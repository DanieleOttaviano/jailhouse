/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RISC-V
 *
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <stdint.h>

#define NPAGES 1024
#define DIM 12 * NPAGES // 123*32(size fo int)=4096 bytes= 4KB(size of one page)
#define REP 1 // 10 
#define REP_TIME 100
#define FREQUENCY 100  // 100 MHz
#define PERIOD 200000 // 200 ms
#define SHARED_MEM_SIZE 1024
#define FLAG_INDEX (SHARED_MEM_SIZE - 1)

void main(void){
	volatile uint32_t* system_counter = (uint32_t*)0xFF250000;
	volatile uint32_t *shared_memory = (uint32_t *)0x46d00000;
	uint32_t *mem_array = (uint32_t *)0x70FF0000;

	uint32_t start, end, diff;
	uint32_t readsum = 0;
	uint32_t time_us = 0;
	int i, j, k;

	// Initialize memory array
	for (i = 0; i < DIM; i++) {
		mem_array[i] = i;
	}
	// Zero shared memory
	for (i = 0; i < SHARED_MEM_SIZE; i++) {
		shared_memory[i] = 0;
	}

	while (1) {
		for (k = 0; k < REP_TIME; k++) {
			// Wait until Linux sets flag to 0
			while (shared_memory[FLAG_INDEX] != 0);

			start = *system_counter;
			for (j = 0; j < REP; j++) {
				for (i = 0; i < DIM; i++) {
					readsum += mem_array[i]; // READ
				}
			}
			end = *system_counter;
			diff = end - start;
			time_us = diff / FREQUENCY;

			// Write to shared memory
			shared_memory[k] = time_us;

			// Set flag to indicate data ready
			shared_memory[FLAG_INDEX] = 1;

			// Delay until next period
			while (time_us < PERIOD){
				end = *system_counter;
				diff = end - start;
				time_us = diff / FREQUENCY;
			}
		}
	}
}
