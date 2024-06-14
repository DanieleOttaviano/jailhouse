/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Memory Bomb userspace supporting tool.
 *
 * Copyright (C) Boston University, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Renato Mancuso (BU) <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include <jailhouse/mem-bomb.h>

struct control {
	unsigned int cmd;
	unsigned int size;
	unsigned int cpu;
	unsigned int time;
	unsigned int memory;
	unsigned int type;
	unsigned int dram_col;
	unsigned int dram_row;
};

static unsigned int get_mem_size(char * mem_str)
{
	const char suff [] = {'k', 'm', 'g', 'p'};
	/* Check the last character of the string and compute
	 * multiplier */
	unsigned int i;
	int len = strlen(mem_str);
	unsigned int shift = 0;

	if (len < 2)
		return atoi(mem_str);

	for (i = 0; i < sizeof(suff); ++i) {
		if (mem_str[len-1] == suff[i]) {
			shift = 10 * (i+1);
			mem_str[len-1] = 0;
			break;
		}
	}

	return atoi(mem_str) << shift;
}

static void usage(char *name)
{
	printf("Usage: %s -v [-c cpu] [-s size] [-t -m time, memory] [-r] [-w] [-e]\n"
			"\t-v\t verbose\n"
			"\t-c\t cpu (from 1, CPU0 is for root cell)\n"
			"\t-r\t only read\n"
			"\t-w\t only writes\n"
			"\t-s\t read and/or write size\n"
			"\t-e\t enable\n"
			"\t\t default: all cpu, r+w, 64\n",
			name);
}

int main(int argc, char **argv)
{
	int opt;
	int cpu;
	unsigned int size, time, memory;
	unsigned int cmd;
	unsigned long off;
	int sleep_sec = 0;
	int mfd;
	void *mem;
	volatile struct control *ctrl;

	size = 64;
	time = memory = 0;
	cpu = -1;
	cmd = CMD_DO_READS | CMD_DO_WRITES;
	while ((opt = getopt(argc, argv, "vrwc:s:t:m:e")) != -1) {
		switch (opt) {
			case 'v':
				cmd |= CMD_VERBOSE;
				sleep_sec = 1;
				break;
			case 'r':
				cmd &= ~CMD_DO_WRITES;
				break;
			case 'w':
				cmd &= ~CMD_DO_READS;
				break;
			case 'c':
				cpu = atoi(optarg);
				if (cpu < 0) {
					fprintf(stderr, "CPU < 0\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 's':
				size = get_mem_size(optarg);
				break;
			case 't':
				time = atoi(optarg);
				break;
			case 'm':
				memory = atoi(optarg);
				break;
			case 'e':
				cmd |= CMD_ENABLE;
				break;
			default:
				usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	mfd = open("/dev/mem", O_RDWR);

	if(mfd < 0) {
		perror("Unable to open /dev/mem.");
		exit(EXIT_FAILURE);
	}

	mem = mmap(0, COMM_TOTAL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			mfd, COMM_PHYS_BASE);

	if (mem == MAP_FAILED) {
		perror("Cannot map.");
		exit(EXIT_FAILURE);
	}

	if (close(mfd) < 0) {
		perror("Closing mem.");
		exit(EXIT_FAILURE);
	}

	if (time != 0 && mem != 0) {
		cmd |= CMD_MEMGUARD;
	}

	if (cpu > 0) {
		printf("CMD: 0x%x, CPU = %d, Size = %u, BT = %u, BM = %u\n",
				cmd, cpu, size, time, memory);
		/* selected CPU only */
		off = (cpu - 1) * COMM_SINGLE_SIZE;
		ctrl = (volatile struct control *)(mem + off);

		if (time != 0 && mem != 0) {
			ctrl->time = time;
			ctrl->memory = memory;
			ctrl->type = 0;
		}

		ctrl->size = size;
		ctrl->cmd = cmd;
		ctrl->cpu = cpu;

		goto unmap;
	}

	/* All CPUs */
	for (cpu = 0; cpu < NUM_CPU; cpu++) {
		/* root cell on CPU 0 might be not affected by this
		 * but we map a sufficiently large space of
		 * COMM_TOTAL_SIZE = NUM_CPU * COMM_SINGLE_SIZE
		 */
		off = cpu * COMM_SINGLE_SIZE;
		ctrl = (volatile struct control *)(mem + off);

		printf("CMD: 0x%x, CPU = %d, Size = %u, BT = %u, BM = %u\n",
				cmd, cpu, size, time, memory);

		if (time != 0 && mem != 0) {
			ctrl->time = time;
			ctrl->memory = memory;
			ctrl->type = 0;
		}

		ctrl->size = size;
		ctrl->cmd = cmd;
		ctrl->cpu = cpu + 1;
		sleep(sleep_sec);
	}

unmap:
	if (munmap(mem, COMM_TOTAL_SIZE) < 0) {
		perror("Unmap.");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
