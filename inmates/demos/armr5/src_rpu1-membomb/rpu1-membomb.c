/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RPU: memory bandwidth bomb. Writes in the memory assigned to RPU0 and RISCV PICO32
 *
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "sleep.h"
#include "xil_cache.h"

#define REP_TIME 100000	// 10s
#define FREQUENCY COUNTS_PER_USECOND  // ~8.83 Mhz
#define PERIOD 	30 	// 30 us

static u32* base_address = (u32*)0x3ED00000;	// RPU0 memory
static u32* base_address_pico32 = (u32*)0x70000000; // Pico32 memory
static u32 memory_size = 4 * 1024 * 1024; 		// 4Mb


// void bit_flip(){
// 	XTime start, end, diff;
// 	u32 time_us = 0;
// 	u32 offset = 0;
// 	u32 bit_position = 0;
	
// 	xil_printf("[RPU1] base address: 0x%x\n\r", base_address);
// 	xil_printf("[RPU1] memory size: %d\n\r", memory_size);

// 	for(int i = 0; i < REP_TIME; i++ ){

// 		XTime_GetTime(&start);

// 		// Write all the memory
// 		// base_address[i] = 0xFF;

// 		// Generate a random offset within the memory region
// 		offset = (u32)start % memory_size;
// 		// Generate a random bit position within the selected byte
// 		bit_position = (u32)start % 8;
// 		// Toggle the randomly chosen bit at the specified offset
// 		base_address[offset] ^= (1 << bit_position);
// 		xil_printf("accessing offset: %d, bit position:%d\r\n",offset, bit_position);

// 		XTime_GetTime(&end);
// 		// Calculate time in us
// 		diff = end - start;
// 		time_us = diff / FREQUENCY;

// 		//xil_printf("time(us):%llu\n\r", time_us);
// 		// wait until the end of the period
// 		while (time_us < PERIOD){
// 			XTime_GetTime(&end);
// 			diff = end - start;
// 			time_us = diff / FREQUENCY;
// 		}
// 	}
//}

void membomb(){
	XTime start, end, diff;
	u32 time_us = 0;
	while(1){
    	for(int i = 0 ; i < memory_size/sizeof(u32) ; i++){

			XTime_GetTime(&start);
    		
			base_address[i] = 0xFFFFFFFF;
			base_address_pico32[i] = 0xFFFFFFFF;
			//asm_clean_inval_dc_line_mva_poc((u32)&base_address[i]);
			//Xil_DCacheStoreLine((u32)&base_address[i]);
			
			// DEBUG
			//xil_printf("base_address[%d] = %d\n\r", i, base_address[i]);			
			XTime_GetTime(&end);

			// Calculate time in us
			diff = end - start;
			time_us = diff / FREQUENCY;

			// xil_printf("time(us):%llu\n\r", time_us);
			// wait until the end of the period
			while (time_us < PERIOD){
				XTime_GetTime(&end);
				diff = end - start;
				time_us = diff / FREQUENCY;
			}
    	}
	}
}

int main()
{
	// Initialize the Platform
	init_platform();	
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	
	xil_printf("[RPU-1] Start RPU1!\n\r");
	membomb();
	xil_printf("[RPU-1] Successfully ran the application\n\r");

	cleanup_platform();
	return 0;
}
