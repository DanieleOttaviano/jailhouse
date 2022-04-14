/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Wrapper to trigger "util-stress" mode of the memory bombs.
 *
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
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

#define DEFAULT_ITER	1000

struct control {
	unsigned int cmd;
	/* NOTE: overloaded to pass # iterations to the mem-bomb */
	unsigned int size;
	unsigned int cpu;
	unsigned int time;
	unsigned int memory;
	unsigned int type;
	unsigned int dram_col;
	unsigned int dram_row;
};

static void usage(char *name)
{
	printf("Usage: %s -v [-i iterations] [-n num_cpu] -c col -r row [-t] [-e]\n"
			"\t-v\t verbose\n"
			"\t-n\t num_cpu: generate WC load from num_cpu (from CPU1), default all\n"
			"\t-i\t iterations (default 1000)\n"
			"\t-c\t col id\n"
			"\t-r\t row id\n"
			"\t-t\t test multiple combination of col/row ids\n"
			"\t-e\t enable\n",
			name);
}

#define OPT_ARGS "vn:i:l:c:r:et"
int main(int argc, char **argv)
{
	int opt;
	int col;
	int row;
	int iter;
	int num_cpu, cpu;
	unsigned int cmd;
	unsigned long off;
	int sleep_sec = 0;
	int mfd;
	void *mem;
	volatile struct control *ctrl;

	num_cpu = -1;
	col = row = -1;
	cmd = CMD_UTILSTRESS;
	iter = DEFAULT_ITER;
	while ((opt = getopt(argc, argv, OPT_ARGS)) != -1) {
		switch (opt) {
			case 'v':
				cmd |= CMD_VERBOSE;
				sleep_sec = 1;
				break;
			case 'n':
				num_cpu = atoi(optarg);
				if (num_cpu < 0) {
					fprintf(stderr, "num_cpu < 0, ignored\n");
					num_cpu = -1;
				}
				break;
			case 'i':
				iter = atoi(optarg);
				if (iter < 0) {
					fprintf(stderr, "invalid iter, using default\n");
					iter = DEFAULT_ITER;
				}
				break;
			case 'c':
				col = atoi(optarg);
				if (col < 0) {
					fprintf(stderr, "Invalid col id\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'r':
				row = atoi(optarg);
				if (row < 0) {
					fprintf(stderr, "Invalid row id\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 't':
				cmd |= CMD_UTILSTRESS_TEST;
				break;
			case 'e':
				cmd |= CMD_ENABLE;
				break;
			default:
				usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if ((col == -1 || row == -1) && !(cmd & CMD_UTILSTRESS_TEST)) {
		fprintf(stderr, "Provide both column and row id\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
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

	for (cpu = 0; cpu < NUM_CPU; cpu++) {
		/* root cell on CPU 0 might be not affected by this
		 * but we map a sufficiently large space of
		 * COMM_TOTAL_SIZE = NUM_CPU * COMM_SINGLE_SIZE
		 */
		off = cpu * COMM_SINGLE_SIZE;
		ctrl = (volatile struct control *)(mem + off);

		printf("CPU: %d, col: %d, row: %d, iter: %d %c\n",
				cpu, col, row, iter,
				(cmd & CMD_ENABLE) ? 'e' : 'd');

		ctrl->size = iter;
		ctrl->cmd = cmd;
		ctrl->cpu = cpu + 1;
		ctrl->dram_col = col;
		ctrl->dram_row = row;

		sleep(sleep_sec);
	}

	if (munmap(mem, COMM_TOTAL_SIZE) < 0) {
		perror("Unmap.");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
