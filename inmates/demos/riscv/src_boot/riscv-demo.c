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

#define DIM_MB 90
#define SIZE_NO_BUFFER 82       // Dimension of the bin without the buffer the buffer
#define SIZE_BIN 1024*DIM_MB    // Dimension of the bin file

static volatile const char buffer[(SIZE_BIN-SIZE_NO_BUFFER)*1024]={0};

void main(void){
	volatile uint32_t* system_counter = (uint32_t*)0xFF250000;
	volatile uint32_t *shared_memory = (uint32_t *)0x46d00000;

	// Boot Time
	uint32_t time = *system_counter;

	// Write boot time in shared memory
  	shared_memory[0] = time;
	
	while(1);
}

