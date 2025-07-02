/* SPDX-License-Identifier: MIT */
/* Copyright 2023-2025 Alexander Zuepke */
/*
 * bench.c
 *
 * Benchmark memory performance
 *
 * Compile:
 *   $ gcc -O2 -DNDEBUG -W -Wall -Wextra -Werror bench.c -o bench
 *
 * Run (as root)
 *   $ ./bench <read|write|modify> [-s <size-in-MiB>] [-c <cpu>] [-p <prio>]
 *
 * azuepke, 2023-02-22: standalone version
 * azuepke, 2023-12-07: explicit prefetches
 * azuepke, 2023-12-12: more prefetches
 * azuepke, 2023-12-19: dump regulation-related PMU counters
 * azuepke, 2023-12-20: argument parsing
 * azuepke, 2023-12-24: incorporate worst-case memory access benchmarks
 * azuepke, 2023-12-25: test automation and CSV files
 * azuepke, 2024-03-28: RISC-V version
 * azuepke, 2025-02-18: pointer chasing
 * azuepke, 2025-02-19: sizes in KiB
 * azuepke, 2025-02-25: configurable PMU counters
 * azuepke, 2025-02-26: PMU event sources
 * azuepke, 2025-04-01: access whole memory region in auto-wcet benchmark
 * azuepke, 2025-04-03: step offset
 * dottavia, 2025-07-01: ported to run on small core on the ZCU+ platform (Cortex-R5)
 * 
 */

#include <stdio.h>
#include <assert.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "sleep.h"
#include "xil_cache.h"

/* program name */
#define PROGNAME "bench"

/* build ID, passed as string to compiler via -DBUILDID=<...> */
#ifndef BUILDID
#define BUILDID "local"
#endif

////////////////////////////////////////////////////////////////////////////////

/* CPU frequency in Hz, defined in the Xilinx BSP */
#define CPU_HZ XPAR_CPU_CORTEXR5_0_CPU_CLK_FREQ_HZ

/* half range for the timer counter */
#define HALF_TIMER_RANGE 0x80000000U

/* hardcoded size of a cacheline */
#define CACHELINE_SIZE 32 

/* default mapping size in MiB */
#define DEFAULT_MB 32

/* default test to run */
#define DEFAULT_TEST "modify" /* read, write, modify */ 

/* test mapping */
static char *map_addr = (char *)0x3ad00000;
static char *map_addr_end = (char *)(0x3ad00000 + (DEFAULT_MB * 1024 * 1024));
static size_t map_size = DEFAULT_MB * 1024 * 1024; /* in bytes */

/* global options */
static int option_step = 1; /* 0: linear, 1: step, 2: auto */
static int option_step_all = 0;
/* ==0: infinite, >0: limited number of loops */
// static int option_num_loops = 0;
/* print delay milliseconds, default 1s */
static int option_print_delay_ms = 1000;
static int option_all = 0;

/* if option_step = 1 */
static int step_size = 65536; // CACHELINE_SIZE;
static int step_offset = 0;	


////////////////////////////////////////////////////////////////////////////////

/* conversion from MB/s to MiB/s */
static inline float to_mib(float val)
{
	return val * (1000 * 1000) / (1024 * 1024);
}

////////////////////////////////////////////////////////////////////////////////

static inline u32 next_highest_power_of_2(unsigned int val)
{
	val--;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	val++;

	return val;
}

static inline u32 ilog2(u32 val)
{
	u32 max_bits = (sizeof(val) * 8) - 1;
	return max_bits - __builtin_clzl(val);
}

////////////////////////////////////////////////////////////////////////////////

/* LFSR repeating after 2^n-1 steps */

/*
 * Roy Ward, Timothy C.A. Molteno: "Table of Linear Feedback Shift Registers",
 * Electronics Technical Report No. 2012-1, University of Otago, NZ
 * https://www.physics.otago.ac.nz/reports/electronics/ETR2012-1.pdf
 */
static const u32 lfsr_tap_bits[] = {
	0x00000000,
	0x00000001,
	0x00000003,
	0x00000006,
	0x0000000c,
	0x00000014,
	0x00000030,
	0x00000060,
	0x000000b8,
	0x00000110,
	0x00000240,
	0x00000500,
	0x00000e08,
	0x00001c80,
	0x00003802,
	0x00006000,
	0x0000d008,
	0x00012000,
	0x00020400,
	0x00072000,
	0x00090000,
	0x00140000,
	0x00300000,
	0x00420000,
	0x00e10000,
	0x00d80000,
	0x01200000,
	0x03880000,
	0x07200000,
	0x09000000,
	0x14000000,
	0x32800000,
	0x48000000,
	0xa3000000,
};

/* LFSR step function, do not need with zero value */
static inline u32 lfsr(unsigned int x, unsigned int bits)
{
	u32 tap_bits;
	u32 y;
	u32 i;

	assert(x > 0);
	assert(bits <= 32);

	tap_bits = lfsr_tap_bits[bits];

	y = 0;
	for (i = 0; i < bits; i++) {
		if ((tap_bits & (1u << i)) != 0) {
			y ^= (x >> i) & 1;
		}
	}

	x = (x << 1) | y;
	x &= ((((1u << (bits - 1)) -1) << 1) | 1u);
	return x;
}

////////////////////////////////////////////////////////////////////////////////

struct cacheline {
	struct cacheline *next;
	u32 scratch;
	char padding[CACHELINE_SIZE - 2*sizeof(long)];
};

/* prepare a random pointer-chasing pattern based on an LFSR
 *
 * The LFSR generates unique values in the range 1..2^n-1
 * and repeats after 2^n-1 steps.
 * We start at array index zero and prepare pointers to the next value.
 * We let the last 2^n entry point to index 0 to close the loop.
 *
 * This works well for any power-of-two sizes.
 * For smaller sizes, we use an LFSR with the next higher power-of-two value.
 * If an generated index is beyond the array size, we continue generating values
 * until we're back in the valid range.
 */
static void prepare_chase_random(void *start, size_t size)
{
	struct cacheline *ptr = start;
	u32 lfsr_state;
	u32 lfsr_end;
	u32 bits;
	u32 next;
	u32 end;
	u32 pos;
	u32 i;

	end = size / CACHELINE_SIZE;
	lfsr_end = next_highest_power_of_2(end);
	bits = ilog2(lfsr_end);
	lfsr_state = -1;

	pos = 0;
	for (i = 0; i < lfsr_end-1; i++) {
		lfsr_state = lfsr(lfsr_state, bits);
		next = lfsr_state;
		if (next >= end) {
			continue;
		}
		ptr[pos].next = &ptr[next];
		pos = next;
	}
	ptr[pos].next = &ptr[0];
}

////////////////////////////////////////////////////////////////////////////////

/* flush cacheline, without any fences */
static inline void flush_cacheline(void *addr)
{
	Xil_DCacheFlushLine((INTPTR)addr); // flush cacheline
}

static inline void memory_barrier(void)
{
	__asm__ volatile ("dmb sy" : : : "memory"); 
}

static inline void flush_cacheline_all(void)
{
	char *ptr = map_addr;
	const char *end = map_addr_end;

	for (; ptr < end; ptr += CACHELINE_SIZE) {
		flush_cacheline(ptr);
	}
	memory_barrier();
}

////////////////////////////////////////////////////////////////////////////////

/* generate a set of non-inlined benchmark loops */
#define BENCH_CACHELINE(name)	\
	/* go over whole memory, cacheline by cacheline */	\
	static void name ## _linear(void)	\
	{	\
		char *ptr = map_addr;	\
		char *end = map_addr_end;	\
		for (; ptr < end; ptr += CACHELINE_SIZE) {	\
			name(ptr);	\
		}	\
	}	\
	\
	/* go over memory once, with steps, leaving out unrelated cachelines */	\
	static void name ## _step(size_t step, size_t step_offset)	\
	{	\
		char *ptr = map_addr + step_offset;	\
		char *end = map_addr_end;	\
		for (; ptr < end; ptr += step) {	\
			name(ptr);	\
		}	\
	}	\
	\
	/* go over memory multiple times, with steps, change start offset each iteration */	\
	static void name ## _step_all(size_t step, size_t step_offset)	\
	{	\
		char *end = map_addr_end;	\
		size_t ptr_offset;	\
		(void)step_offset;	\
		for (ptr_offset = 0; ptr_offset < step; ptr_offset += CACHELINE_SIZE) {	\
			char *ptr = map_addr + ptr_offset;	\
			for (; ptr < end; ptr += step) {	\
				name(ptr);	\
			}	\
		}	\
	}

#define BENCH_NAMES(name)	\
	.bench_linear = name ## _linear,	\
	.bench_step = name ## _step,	\
	.bench_step_all = name ## _step_all

////////////////////////////////////////////////////////////////////////////////

/* read a cache line using a load of one word */
static inline void read_cacheline(void *addr)
{
	u32 *p = addr;
	u32 tmp;
	/* the compiler barriers prevent reordering */
	__asm__ volatile ("" : : : "memory");
	tmp = p[0];
	/* consume tmp */
	__asm__ volatile ("" : : "r"(tmp) : "memory");
}

BENCH_CACHELINE(read_cacheline)

////////////////////////////////////////////////////////////////////////////////

/* write to a complete cache line using stores */
static inline void write_cacheline(void *addr)
{
	__uint32_t *p = addr;
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
}

BENCH_CACHELINE(write_cacheline)


////////////////////////////////////////////////////////////////////////////////

/* modify a single word in a cache line, effectively a read-modify-write */
static inline void modify_cacheline(void *addr)
{
	u32 *p = addr;

	/* the compiler barriers prevent reordering */
	__asm__ volatile ("" : : : "memory");
	p[0] = (u32) addr;
	__asm__ volatile ("" : : : "memory");
}

BENCH_CACHELINE(modify_cacheline)

////////////////////////////////////////////////////////////////////////////////

static void chase_random_prep(void)
{
	prepare_chase_random(map_addr, map_size);
}

static void chase_random_read(void)
{
	struct cacheline *start = (struct cacheline *)map_addr;
	struct cacheline *ptr = start;
	do {
		ptr = ptr->next;
	} while (ptr != start);
}

static void chase_random_write(void)
{
	struct cacheline *start = (struct cacheline *)map_addr;
	struct cacheline *ptr = start;
	do {
		ptr->scratch = 42;
		ptr = ptr->next;
	} while (ptr != start);
}

////////////////////////////////////////////////////////////////////////////////

/* test cases */
static struct test {
	const char *name;
	void (*bench_prep)(void);
	void (*bench_linear)(void);
	void (*bench_step)(size_t, size_t);
	void (*bench_step_all)(size_t, size_t);
	const char *desc;
} tests[] = {
	{	.name = "read", BENCH_NAMES(read_cacheline), .desc = "read cacheline",	},
	{	.name = "write", BENCH_NAMES(write_cacheline), .desc = "write full cacheline (without reading)",	},
	{	.name = "modify", BENCH_NAMES(modify_cacheline), .desc = "modify cacheline (both read and write)",	},
	{	.name = "chase_random_r", .bench_prep = chase_random_prep, .bench_linear = chase_random_read, .desc = "random pointer chasing in reading mode",	},
	{	.name = "chase_random_w", .bench_prep = chase_random_prep, .bench_linear = chase_random_write, .desc = "random pointer chasing in writing (modify) mode",	},
	{	.name = NULL, .desc = NULL,	},
};

static void bench_linear(const char *name, void (*bench)(void))
{
    XTime ts_start, ts_end, ts_now;
    u64 bytes_accessed;
	u64 delta_ns;
    u32 delta_ticks, delay_ticks;
    u32 runs = 0;
    // int loops = 0;
    double bw;

	delay_ticks = ((u64)option_print_delay_ms * COUNTS_PER_SECOND) / 1000U;

    printf("[RPU-1] linear %s bandwidth over %u KiB (%u MiB) block\n",
           name, map_size / 1024, (map_size + 1024 * 1024 - 1) / (1024 * 1024));

    flush_cacheline_all();

    XTime_GetTime(&ts_start);
    ts_end = ts_start + delay_ticks;

    while (1) {
        bench();
        runs++;

        XTime_GetTime(&ts_now);

		if((u32)(ts_now - ts_end) < HALF_TIMER_RANGE) {
            bytes_accessed = (u64)runs * map_size;

            delta_ticks = ts_now - ts_start; 
			delta_ns = ((u64)delta_ticks * 1000000000ULL) / COUNTS_PER_SECOND;

            bw = 1000.0 * (double)bytes_accessed / (double)delta_ns;

            printf("[RPU-1] %9.1f MiB/s, %9.1f MB/s\n", to_mib(bw), bw);

            // loops++;
            // if (loops == option_num_loops) {
            //     break;
            // }

            XTime_GetTime(&ts_start);
            ts_end = ts_start + delay_ticks;
            runs = 0;
        }
    }
}

static void bench_step(const char *name, void (*bench)(size_t, size_t), size_t step, size_t step_offset, int step_all)
{
    XTime ts_now, ts_start, ts_end;
    u32 runs = 0;
    // int loops = 0;
    u64 bytes_accessed;
    u32 delta_ticks;
    u64 delta_ns;
    double bw;

    unsigned delay_ticks = ((u64)option_print_delay_ms * COUNTS_PER_SECOND) / 1000U;

    printf("[RPU-1] step1 %s bandwidth over %u KiB (%u MiB) block, step %u, offset %u\n",
           name, (unsigned)(map_size / 1024), (unsigned)((map_size + 1024*1024 - 1) / (1024*1024)),
           (unsigned)step, (unsigned)step_offset); 

    flush_cacheline_all();

    XTime_GetTime(&ts_start);
    ts_end = ts_start + delay_ticks;

    while (1) {
        bench(step, step_offset);
        runs++;

        XTime_GetTime(&ts_now);

        // Wrap-around safe check if ts_now passed ts_end
        if ((u32)(ts_now - ts_end) < HALF_TIMER_RANGE) {
            if (step_all) {
                bytes_accessed = (u64)runs * map_size;
            } else {
                bytes_accessed = (u64)runs * map_size * CACHELINE_SIZE / step;
            }

            delta_ticks = (u32)(ts_now - ts_start);
            delta_ns = ((u64)delta_ticks * 1000000000ULL) / COUNTS_PER_SECOND;

            bw = 1000.0f * (double)bytes_accessed / (double)delta_ns;

            printf("[RPU-1] %9.1f MiB/s, %9.1f MB/s\n", to_mib(bw), bw);

            // loops++;
            // if (loops == option_num_loops) {
            //     break;
            // }

            XTime_GetTime(&ts_start);
            ts_end = ts_start + delay_ticks;
            runs = 0;
        }
    }
}

static void bench_auto(const char *name, void (*bench)(size_t, size_t), int step_all)
{
    XTime ts_start, ts_now, ts_end;
    u64 bytes_accessed;
    u64 delta_ns;
    u32 delta_ticks, delay_ticks;
    u32 runs = 0;
    size_t step, min_step;
    float min_step_bw;
    float bw;

    // Calculate how long to measure each step (in timer ticks)
    delay_ticks = ((u64)option_print_delay_ms * COUNTS_PER_SECOND) / 1000U;

    printf("[RPU-1] worst-case %s bandwidth over %u KiB (%u MiB) block\n",
           name, map_size / 1024, (map_size + 1024*1024-1) / 1024 / 1024);

	flush_cacheline_all();

    // Init
    min_step = (size_t)-1;
    min_step_bw = 1e30f;

    step = CACHELINE_SIZE;

    while (1) {
        XTime_GetTime(&ts_start);
        ts_end = ts_start + delay_ticks;
        runs = 0;

        while (1) {
            bench(step, 0);
            runs++;

            XTime_GetTime(&ts_now);

            // Wrap-safe check: has ts_now passed ts_end?
            if ((u32)(ts_now - ts_end) < 0x80000000U) {
                break;
            }
        }

        // Bandwidth computation
        if (step_all) {
            bytes_accessed = (u64)runs * map_size;
        } else {
            bytes_accessed = (u64)runs * map_size * CACHELINE_SIZE / step;
        }

        delta_ticks = ts_now - ts_start;
        delta_ns = ((u64)delta_ticks * 1000000000ULL) / COUNTS_PER_SECOND;
        bw = 1000.0 * (double)bytes_accessed / (double)delta_ns;

        printf("[RPU-1] step %9u: %9.1f MiB/s, %9.1f MB/s\n", step, to_mib(bw), bw);

        if (bw < min_step_bw) {
            min_step_bw = bw;
            min_step = step;
        }

        step *= 2;
        if (step >= map_size / 8) {
            break;
        }
    }

    printf("[RPU-1] slowest step size: %u\n", min_step);
}

int main()
{
	struct test * t = tests;
	t->name = DEFAULT_TEST;	

	// Initialize the Platform
	init_platform();  
	Xil_DCacheEnable();
	Xil_ICacheEnable();
	// Xil_DCacheDisable();
	// Xil_ICacheDisable();

	if (option_step == 0) {
		if (option_all != 0) {
			for (t = tests; t->name != NULL; t++) {
				if (t->bench_linear != NULL) {
					if (t->bench_prep != NULL) {
						t->bench_prep();
					}
					bench_linear(t->name, t->bench_linear);
				}
			}
		} else {
			assert(t->name != NULL);
			if (t->bench_linear != NULL) {
				if (t->bench_prep != NULL) {
					t->bench_prep();
				}
				bench_linear(t->name, t->bench_linear);
			}
		}
	} else if (option_step == 1) {
		if (option_all != 0) {
			for (t = tests; t->name != NULL; t++) {
				if (t->bench_step != NULL) {
					if (t->bench_prep != NULL) {
						t->bench_prep();
					}
					bench_step(t->name,
					           option_step_all ? t->bench_step_all : t->bench_step,
					           step_size, step_offset, option_step_all);
				}
			}
		} else {
			assert(t->name != NULL);
			if (t->bench_step != NULL) {
				if (t->bench_prep != NULL) {
					t->bench_prep();
				}
				bench_step(t->name,
				           option_step_all ? t->bench_step_all : t->bench_step,
				           step_size, step_offset, option_step_all);
			}
		}
	} else if (option_step == 2) {
		if (option_all != 0) {
			for (t = tests; t->name != NULL; t++) {
				if (t->bench_step != NULL) {
					if (t->bench_prep != NULL) {
						t->bench_prep();
					}
					bench_auto(t->name,
					           option_step_all ? t->bench_step_all : t->bench_step,
					           option_step_all);
				}
			}
		} else {
			assert(t->name != NULL);
			if (t->bench_step != NULL) {
				if (t->bench_prep != NULL) {
					t->bench_prep();
				}
				bench_auto(t->name,
				           option_step_all ? t->bench_step_all : t->bench_step,
				           option_step_all);
			}
		}
	}

	cleanup_platform();
	return 0;
}
