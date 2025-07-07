/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RPU: Corrupt the kernel code by writing to the memory 
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

#define NPAGES 1024
#define DIM 12 * NPAGES // 123*32(size fo int)=4096 bytes= 4KB(size of one page)
#define FREQUENCY COUNTS_PER_USECOND // ~= 8.83 MHz
#define PERIOD 200000 // 200 ms

int main()
{
  u32 *mem_array = (u32 *)0x00210000; // kernel code start addr
  XTime start, end, diff;
  u32 time_us = 0;
  u32 i=0;

  // Initialize the Platform
  init_platform();  
  Xil_DCacheDisable();
  Xil_ICacheDisable();

  xil_printf("[RPU-0] Start corrupting memory ... \n\r");

  // Mem Array Initialization
  for (i = 0; i < DIM; i++) {
    XTime_GetTime(&start);
    xil_printf("[RPU-0] Corrupting physical address 0x%08X\n\r", (unsigned int)mem_array + i * sizeof(u32));
    mem_array[i] = i;
    XTime_GetTime(&end);

    // Calculate time in us
    diff = end - start;
    time_us = diff / FREQUENCY;
    
    // wait until the end of the period
    while (time_us < PERIOD){
      XTime_GetTime(&end);
      diff = end - start;
      time_us = diff / FREQUENCY;
    }
  } 

  xil_printf("[RPU-0] Done!\n\r");
  
  cleanup_platform();
  return 0;
}
