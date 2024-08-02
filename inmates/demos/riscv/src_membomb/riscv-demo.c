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

#define REP_TIME 100000 // 10
#define FREQUENCY 100  // 100 MHz
#define PERIOD 30 // 30 us

static uint32_t memory_size = 4 * 1024 * 1024; 			// 4Mb
static uint8_t* base_address = (uint8_t *)0x78FF0000;

void main(void){
	volatile uint32_t* system_counter = (uint32_t*)0xFF250000;
	volatile uint32_t* shared_memory =  (uint32_t *)0x46d00000;
	uint32_t start, end, diff;
  	uint32_t time_us = 0;
  	int i, j, k;
	
	while(1){
		for(int i = 0 ; i < memory_size ; i=i+16){

			start=*system_counter;
			base_address[i] = 0xFF;
			end=*system_counter;

			// Calculate time in us
			diff = end - start;
			time_us = diff / FREQUENCY;

			// wait until the end of the period
			while (time_us < PERIOD){
				end=*system_counter;
				diff = end - start;
				time_us = diff / FREQUENCY;
			}
		}
	}
}
