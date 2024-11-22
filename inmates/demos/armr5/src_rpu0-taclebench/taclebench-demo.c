/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RPU
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

#define REP_TIME 1
#define FREQUENCY 8  // 8 MHz

#define CALL(NAME, FUN) CONCAT(NAME, FUN)
#define CONCAT(NAME, FUN)  NAME ## _ ## FUN()

#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

#define INCLUDE_FILE(name) _STRINGIFY(name.h)

#include INCLUDE_FILE(BENCHNAME)

int main()
{
  u32 *shared_memory = (u32 *)0x46d00000;
  XTime start, end, diff;
  u32 time_us = 0;
  int i;

  // Initialize the Platform
  init_platform();  
  Xil_DCacheDisable();
  Xil_ICacheDisable();
  // xil_printf(""STRINGIFY(BENCHNAME)" Benchmark Test Start!\n\r");
  xil_printf("Start!\n\r"); 

  //SHM Array initialization
  for (i = 0; i < REP_TIME; i++) {
    shared_memory[i] = 0;
  }

 for(i = 0; i < REP_TIME; i++ ){ 
    /* Actual access */
    XTime_GetTime(&start);
    // Exec the Benchmark 
    CALL(BENCHNAME, init);
    CALL(BENCHNAME, main);

    XTime_GetTime(&end);

    // Calculate time in us    
    diff = end - start;
    time_us = diff / FREQUENCY;
    
    shared_memory[i] = time_us;
    xil_printf("time(us): ");
    xil_printf("%llu\n\r", time_us); 
  }

  xil_printf("Successfully ran the application\n\r");

  cleanup_platform();
  return 0;
}