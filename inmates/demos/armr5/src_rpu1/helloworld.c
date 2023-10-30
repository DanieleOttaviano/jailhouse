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
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "sleep.h"


#define REP_TIME 100000	// 10s
#define FREQUENCY 8  	// 8 MHz
#define PERIOD 1000 	// 1000 us

int main()
{
	  u8* base_address = (u8*)0x3ED00000;	// RPU0 memory
	  u32 memory_size = 1024 * 1024; 		// 1Mb
	  XTime start, end, diff;
	  u32 time_us = 0;
	  u32 offset = 0;
	  u32 bit_position = 0;
	  int i;


	  // Initialize the Platform
	  init_platform();
	  //xil_printf("Start RPU1!\n\r");


	 for(i = 0; i < REP_TIME; i++ ){

	    XTime_GetTime(&start);

	    // Generate a random offset within the memory region
	    offset = (u32)start % memory_size;
	    // Generate a random bit position within the selected byte
	    bit_position = (u32)start % 8;
	    // Toggle the randomly chosen bit at the specified offset
	    base_address[offset] ^= (1 << bit_position);

	    //xil_printf("accessing offset: %d, bit position:%d\r\n",offset, bit_position);

	    XTime_GetTime(&end);
	    // Calculate time in us
	    diff = end - start;
	    time_us = diff / FREQUENCY;

	    //xil_printf("time(us):%llu\n\r", time_us);
	    // wait until the end of the period
	    while (time_us < PERIOD){
	      XTime_GetTime(&end);
	      diff = end - start;
	      time_us = diff / FREQUENCY;
	    }
	  }


	  xil_printf("[RPU1] Successfully ran the application\n\r");

	  cleanup_platform();
	  return 0;
}
