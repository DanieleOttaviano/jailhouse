/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Memory Bomb and Util-stress bare-metal Jailhouse inmate
 *
 * Copyright (C) Boston University, MA, USA, 2020
 * Copyright (C) University of Illinois at Urbana-Champaign, IL, USA, 2020
 * Copyright (C) Technical University of Munich, 2020 - 2021
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *  Rohan Tabish <rtabish@illinois.com;rohantabish@gmail.com>
 *  Andrea Bastoni <andrea.bastoni@tum.de>
 *
 * Generate worst-case access pattern to DRAM to:
 * 1) bypass caches
 * 2) always generate row misses in memory (bottleneck memory controller).
 *
 * DRAM geometry can be specified explicitly (command line -- see commented
 * out hints) or automatically detected.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <inmate.h>
#include <gic.h>
#include <asm/sysregs.h>
#include <jailhouse/memguard-common.h>
#include <jailhouse/mem-bomb.h>

#define print(fmt, ...)							\
	printk("[BOMB %d] " fmt, id, ##__VA_ARGS__)

/* Functions to interact with CPU cycle counter:
 * these uses the virtual counter timer_get_ticks() uses the physical one
 */
#define magic_timing_begin(cycles)\
	do {								\
		asm volatile("mrs %0, CNTVCT_EL0": "=r"(*(cycles)) );	\
	} while(0)

#define magic_timing_end(cycles)					\
	do {								\
		unsigned long tempCycleLo;				\
		asm volatile("mrs %0, CNTVCT_EL0":"=r"(tempCycleLo) );  \
		*(cycles) = tempCycleLo - *(cycles);			\
	} while(0)

static volatile unsigned char* buffer;
u64 crc;
u32 id;

/* Structure of the command & control interface  */
struct control {
	u32 command;
	u32 size;
	u32 cpu;
	u32 time;
	u32 memory;
	u32 type;
	u32 dram_col;
	u32 dram_row;
};

/* Perform write-only iterations over the memory buffer */
void do_reads(volatile struct control * ctrl);

/* Perform write-only iterations over the memory buffer */
void do_writes(volatile struct control * ctrl);

/* Perform write-only iterations over the memory buffer */
void do_reads_writes(volatile struct control * ctrl);


/* Perform read-only iterations over the memory buffer */
void do_reads(volatile struct control * ctrl)
{
	int i = 0;
	int size = ctrl->size;
	u64 start, end, total;
	unsigned long count = 0;

	crc = 0;

	if (ctrl->command & CMD_VERBOSE)
		print("Started READ accesses with size %d.\n", size);

	total = 0;
	while (ctrl->command & CMD_ENABLE) {
		start = timer_get_ticks();
		for (i = 0; i < size; i += LINE_SIZE) {
			crc += buffer[i];
		}
		end = timer_get_ticks();
		count++;
		total += end - start;
	}

	if (ctrl->command & CMD_VERBOSE) {
		print("Done with READ accesses. Check = 0x%08llx\n", crc);
		print("Avg Time (cyc): %llu\n", total/count);
	}

}

/* Perform write-only iterations over the memory buffer */
void do_writes(volatile struct control * ctrl)
{
	int i = 0;
	int size = ctrl->size;
	u64 start, end, total;
	unsigned long count = 0;

	crc = 0;

	if (ctrl->command & CMD_VERBOSE)
		print("Started WRITE accesses with size %d.\n", size);

	total = 0;
	while (ctrl->command & CMD_ENABLE) {
		start = timer_get_ticks();
		for (i = 0; i < size; i += LINE_SIZE) {
			buffer[i] += i;
		}
		end = timer_get_ticks();
		count++;
		total += end - start;
	}

	if (ctrl->command & CMD_VERBOSE) {
		print("Done with WRITE accesses.\n");
		print("Avg Time (cyc): %llu\n", total/count);
	}

}

/* Perform write-only iterations over the memory buffer */
void do_reads_writes(volatile struct control * ctrl)
{
	int i = 0;
	int size = ctrl->size;
	u64 start, end, total;
	unsigned long count = 0;

	crc = 0;

	if (ctrl->command & CMD_VERBOSE)
		print("Started READ+WRITE accesses with size %d.\n", size);

	/* The top half will be written, the bottom half will be read */
	size = size / 2;

	total = 0;
	while (ctrl->command & CMD_ENABLE) {
		start = timer_get_ticks();
		for (i = 0; i < size; i += LINE_SIZE) {
			buffer[i] += buffer[i+size];
		}
		end = timer_get_ticks();
		count++;
		total += end - start;
	}

	if (ctrl->command & CMD_VERBOSE) {
		print("Done with READ+WRITE accesses.\n");
		print("Avg Time (cyc): %llu\n", total/count);
	}

}

static u64 avg_bw_single(
	unsigned int iter,
	unsigned int tcol_idx,
	unsigned int next_col_idx,
	unsigned int next_row_idx)
{
	unsigned int tot_col = (1 << tcol_idx);
	unsigned int next_col = (1 << next_col_idx);
	unsigned int next_row = (1 << next_row_idx);
	unsigned int col_offset;

	register unsigned int j;
	volatile unsigned char *ptr;
	volatile unsigned char *end;

	u64 start, finish, diff, avg_bw;
	u64 count;
	unsigned long freq = timer_get_frequency();

	end = buffer + MEM_SIZE;

	/* The inner loop in each iteration should write on different rows */
	avg_bw = 0;
	for (j = 0; j < iter; ++j) {
		col_offset = 0;
		count = 0;

		start = timer_get_ticks();
		while (col_offset < tot_col) {
			ptr = buffer + col_offset;
			while (ptr < end) {
				*ptr = (unsigned char)j;
				ptr += next_row;
				count++;
			}
			col_offset += next_col;
		}
		finish = timer_get_ticks();

		diff = finish - start;
		/* diff/freq -> B/s; use MB/s instead */
		diff *= 1000000;
		avg_bw += (count * 64 * freq) / diff;
	}

	/* "2" : one read + one write for each memory access (in WC) */
	avg_bw = 2 * (avg_bw / iter);
	return avg_bw;
}

/* Meaningful range of col/row bits */
#define MAX_COL_BIT	12
#define MIN_COL_BIT	7
#define MAX_ROW_BIT	25
#define MIN_ROW_BIT	10

static void do_util_stress(volatile struct control * ctrl)
{
	/* misuse size to multiplex the number of iterations */
	unsigned int iter = ctrl->size;

	unsigned int minc = MIN_COL_BIT;
	unsigned int maxc = MAX_COL_BIT;
	unsigned int minr = MIN_ROW_BIT;
	unsigned int maxr = MAX_ROW_BIT;

	u64 avg_bw;

	if (ctrl->command & CMD_VERBOSE) {
		print("Started Util Stress\n");
	}

	if (!(ctrl->command & CMD_UTILSTRESS_TEST)) {
		/* not testing mode, only one iteration */
		minc = ctrl->dram_col;
		maxc = minc + 1;
		minr = ctrl->dram_row;
		maxr = minr + 1;
	}

	for (unsigned int col = minc; col < maxc; col++) {
		for (unsigned row = minr; row < maxr; row++) {
			avg_bw = avg_bw_single(iter, col, LOG2_LINE_SIZE, row);
			// TODO: integrate into hypervised and use a spinlock
			printk("%u,%u,%u,%llu\n", ctrl->cpu, col, row, avg_bw);
		}
	}
}

/* Print some info about the memory setup in the inmate */
static void print_mem_info(void)
{
	/* Read and print value of the SCTRL register */
	unsigned long sctlr, tcr;
	arm_read_sysreg(SCTLR, sctlr);
	arm_read_sysreg(TRANSL_CONT_REG, tcr);

	//print("SCTLR_EL1 = 0x%08lx\n", sctlr);
	//print("TCR_EL1 = 0x%08lx\n", tcr);
}

static void test_translation(unsigned long addr)
{
	unsigned long par;
	asm volatile("at s1e1r, %0" : : "r"(addr));

	arm_read_sysreg(PAR_EL1, par);

	//print("Translated 0x%08lx -> 0x%08lx\n", addr, par);
}

#define NS_100_US	100000000
#define CY_100_US	119987841

#define GETTS() \
	({      \
	 unsigned long __val;    \
	 __asm__ volatile ("isb; mrs %0, PMCCNTR_EL0" : "=r" (__val) : : "memory");      \
	 __val;  \
	 })

#define GETVCT()\
	({ 	\
	 unsigned long __val;    \
	 __asm__ volatile ("isb; mrs %0, CNTVCT_EL0" : "=r" (__val) : : "memory");      \
	 __val;  \
	 })

static void calibrate_time(void)
{
	/* NOTE: PMCCNTR_ELO: 100000000 ns ~=  119987841 cy */
	unsigned long freq = timer_get_frequency();
	volatile unsigned long ts0, ts1;
	volatile unsigned long v0, v1;
	u64 start, end, diff_ns;

	start = timer_get_ticks();
	ts0 = GETTS();
	v0 = GETVCT();
#if 0
	do {
		ts1 = GETTS();
	} while ((ts1 - ts0) < CY_100_US);
#else
	volatile unsigned int inc = 0;
	while (inc++ < CY_100_US);
#endif
	v1 = GETVCT();
	ts1 = GETTS();
	end = timer_get_ticks();

	diff_ns = timer_ticks_to_ns(end - start);

	printk("# Calibration\n");
	printk("T: %llu, NS: %llu, F: %lu\n", end - start, diff_ns, freq);
	printk("C: %lu C0 %lu, C1 %lu\n", ts1 - ts0, ts0, ts1);
	printk("V: %lu V0 %lu, V1 %lu\n", v1 - v0, v0, v1);
}

struct memguard_params params;

void inmate_main(void)
{
	volatile struct control *ctrl = 0;

	map_range((void*)CONFIG_INMATE_BASE + 0x10000, MAIN_SIZE - 0x10000, MAP_CACHED);
	map_range((void*)MEM_VIRT_START, MEM_SIZE, MAP_CACHED);
	map_range((void*)COMM_VIRT_ADDR, COMM_SINGLE_SIZE, MAP_CACHED);
	instruction_barrier();

	buffer = (volatile unsigned char*)((unsigned long)MEM_VIRT_START);
	ctrl = (volatile struct control *)((unsigned long)COMM_VIRT_ADDR);
	memset((void*)ctrl, 0, sizeof(*ctrl));

	printk("Memory Bomb Started.\n");
	print_mem_info();
	test_translation((unsigned long)buffer);

	calibrate_time();

	/* Main loop */
	while(1) {
		/* Idle while the enable bit is cleared */
		while(!(ctrl->command & CMD_ENABLE));

		/* Read the ID again in the case we issued only one command */
		id = ctrl->cpu;

		if (ctrl->command & CMD_MEMGUARD) {
			params.budget_time = ctrl->time;
			params.budget_memory = ctrl->memory;
			params.event_type = ctrl->type;
			params.flags = 0;
			print("MG: %llu, %u, 0x%x\n", params.budget_time,
					params.budget_memory,
					params.event_type);
			jailhouse_call_arg1(JAILHOUSE_HC_MEMGUARD_SET,
					    (unsigned long)&params);
		}

		if ((ctrl->command & CMD_DO_READS) &&
		    (ctrl->command & CMD_DO_WRITES)) {
			do_reads_writes(ctrl);
		} else if (ctrl->command & CMD_DO_READS) {
			do_reads(ctrl);
		} else if (ctrl->command & CMD_DO_WRITES) {
			do_writes(ctrl);
		} else if (ctrl->command & CMD_UTILSTRESS) {
			do_util_stress(ctrl);
		} else {
			print("Invalid command (0x%08x)\n",
			      ctrl->command);

			/* Clear the enable bit so we do not print and
			 * endless list of errors. */
			ctrl->command &= ~CMD_ENABLE;
		}
	}

	halt();
}
