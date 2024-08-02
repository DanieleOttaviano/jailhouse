/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RISC-V
 *
 * Copyright (c) Universita' di Napoli Federico II, 2024
 *
 * Authors:
 *   Daniele Ottaviano <daniele.ottaviano@unina.it>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <stdint.h>

#define REP_TIME 1
#define FREQUENCY 100  // 100 MHz

#define CALL(NAME, FUN) CONCAT(NAME, FUN)
#define CONCAT(NAME, FUN)  NAME ## _ ## FUN()

#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

#define INCLUDE_FILE(name) _STRINGIFY(name.h)

#include INCLUDE_FILE(BENCHNAME)

void main(void){
	volatile uint32_t* system_counter = (uint32_t*)0xFF250000;
	volatile uint32_t* shared_memory = (uint32_t *)0x46d00000;
 	uint32_t start, end, diff;
	uint32_t time_us = 0;
	int i;

	//SHM Array initialization
	for (i = 0; i < REP_TIME; i++) {
		shared_memory[i] = 0;
	}

	for(i = 0; i < REP_TIME; i++ ){ 
		/* Actual access */
		start = *system_counter;
		// Exec the Benchmark 
		CALL(BENCHNAME, init);
		CALL(BENCHNAME, main);

		end = *system_counter;

		// Calculate time in us    
		diff = end - start;
		time_us = diff / FREQUENCY;

		shared_memory[i] = time_us;
	}

	while(1);
}

