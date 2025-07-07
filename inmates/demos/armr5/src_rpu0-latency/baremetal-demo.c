/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RPU: execution time latency 
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
#define REP 10
#define FREQUENCY COUNTS_PER_USECOND // ~= 8.83 MHz
#define PERIOD 200000 // 200 ms

int main()
{
  u32 *mem_array = (u32 *)0x3EE00000;
  // u32 *shared_memory = (u32 *)0x46d00000;
  XTime start, end, diff;
  u32 readsum = 0;
  u32 time_us = 0;
  int i, j;

  // Initialize the Platform
  init_platform();  
  Xil_DCacheDisable();
  Xil_ICacheDisable();
  
  xil_printf("[RPU-0] Start!\n\r");
  xil_printf("[RPU-0] Task Period: %d us\n\r", PERIOD);
  xil_printf("[RPU-0] Timer Frequency: %d MHz\n\r", FREQUENCY);

  // Mem Array Initialization
  for (i = 0; i < DIM; i++) {
    mem_array[i] = i;
  }

  while (1) {
    /* Actual access */
    XTime_GetTime(&start);
    for (j = 0; j < REP; j++) {
      for (i = 0; i < DIM; i++) {
        readsum += mem_array[i]; // READ
      }
    }
    XTime_GetTime(&end);
    // Calculate time in us
    diff = end - start;
    time_us = diff / FREQUENCY;
    
    // DEBUG
    xil_printf("[RPU-0] time(us): %llu\n\r", time_us);
    
    // wait until the end of the period
    while (time_us < PERIOD){
      XTime_GetTime(&end);
      diff = end - start;
      time_us = diff / FREQUENCY;
    }
  }

  xil_printf("[RPU-0] Successfully ran the application\n\r");

  cleanup_platform();
  return 0;
}
