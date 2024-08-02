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
 * Omnivisor XMPU demo RPU
 *
 * Copyright (c) Universita' di Napoli Federico II, 2024
 *
 * Authors:
 *   Daniele Ottaviano <daniele.ottaviano@unina.it>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "platform.h"
#include "xil_printf.h"
#include "xmpu.h"
#include <xtime_l.h>
#define PERIOD 1000000 // 1 s
#define FREQUENCY 8  // 8 MHz

int main()
{
  u8 i = 0;
  u32 xmpu_base = 0;
  XTime start, end, diff;
  u32 time_us = 0;

  printf("RPU starting...\n\r");


  while(1){ 
    XTime_GetTime(&start);

    // DEBUG XMPU
    for(i=0 ; i<NR_XMPU_DDR; i++){
      if(i == 1 || i == 2){
        xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET); 
        printf("DDR channel %d\n\r", i);
        print_xmpu_status_regs(xmpu_base);
      }
    }
    // printf("FPD channel 0\n\r");
    // print_xmpu_status_regs(XMPU_FPD_BASE_ADDR);

    XTime_GetTime(&end);
    diff = end - start;
    time_us = diff / FREQUENCY;

    while (time_us < PERIOD){
      XTime_GetTime(&end);
      diff = end - start;
      time_us = diff / FREQUENCY;
    }
  }
  print("Successfully ran application\n\n\r");

  cleanup_platform();
  return 0;
}
