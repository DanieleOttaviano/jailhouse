/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor RISC-V live demo execution time.
 * Complete demo is available at: 
 *  https://github.com/DanieleOttaviano/test_omnivisor_host/tree/main/demo 
 *  https://github.com/DanieleOttaviano/test_omnivisor_guest/tree/main/demo
 * 
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
#define DIM (12 * NPAGES)   // 12*1024 = 12288
#define REP 1 //10
#define FREQUENCY 100 // 100 MHz 
#define PERIOD 200000 // 200 ms

#define SHM_BASE       ((volatile uint32_t *)0x46d01000)
#define MEM_ARRAY	   ((uint32_t *)0x70FF0000)
#define SYSTEM_COUNTER ((volatile uint32_t *)0xFF250000)
#define COUNT          128
#define WRITE_PTR_IDX  COUNT
#define READ_PTR_IDX   (COUNT + 1)
#define SHARED_MEM_SIZE 1024
#define FLAG_INDEX (SHARED_MEM_SIZE - 1)


void main(void){
	uint32_t *mem_array = MEM_ARRAY; // Memory array base address
	volatile uint32_t* system_counter = SYSTEM_COUNTER;
	volatile uint32_t *shared_memory = SHM_BASE;

	uint32_t start, end, diff;
	uint32_t readsum = 0;
	uint32_t time_us = 0;
	int i, j, k;

	// Initialize memory array
	for (i = 0; i < DIM; i++) {
		mem_array[i] = i;
	}

	while (1) {
		// Perform memory read workload
		start = *system_counter;
		for (j = 0; j < REP; j++) {
			for (i = 0; i < DIM; i++) {
				readsum += mem_array[i]; // READ
			}
		}
		end = *system_counter;
		
		diff = end - start;
		time_us = diff / FREQUENCY;

		// --- Ring buffer write ---
		uint32_t write_ptr = shared_memory[WRITE_PTR_IDX];
		uint32_t read_ptr = shared_memory[READ_PTR_IDX];

		// Check if buffer is full
		uint32_t next_ptr = (write_ptr + 1) % COUNT;
		if (next_ptr != read_ptr) {
			shared_memory[write_ptr] = time_us; // Write time to shared memory
			shared_memory[WRITE_PTR_IDX] = next_ptr; // Update write pointer
		}

		// Delay until next period
		while (time_us < PERIOD){
			end = *system_counter;
			diff = end - start;
			time_us = diff / FREQUENCY;
		}
	}
}
