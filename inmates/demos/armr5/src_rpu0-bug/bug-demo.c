/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RPU: write the memory belonging to RPU1
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

int main()
{
  u32 *mem_array = (u32 *)0x3ad00000;
  u32 i=0;
  // Initialize the Platform
  init_platform();  
  Xil_DCacheDisable();
  Xil_ICacheDisable();

  xil_printf("[RPU] Start corrupting memory ... \n\r");

  // Mem Array Initialization
  for (i = 0; i < DIM; i++) {
    mem_array[i] = 0x00000000;
  } 

  xil_printf("[RPU] Done!\n\r");
  

  cleanup_platform();
  return 0;
}
