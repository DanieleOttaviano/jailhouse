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

#define NPAGES 1024
#define DIM 12 * NPAGES // 123*32(size fo int)=4096 bytes= 4KB(size of one page)
#define REP 10
#define REP_TIME 100
#define FREQUENCY 8  // 8 MHz
#define PERIOD 200000 // 200 ms

void test_memory_protection(){
  //Pointer to RPU memory
  unsigned int* ptr_rpu_mem = (unsigned int*)0x3ed01000;
  //Pointer to APU memory
  unsigned int* ptr_apu_mem = (unsigned int*)0x75609000;  
  //Pointer to XMPU memory
  unsigned int* ptr_xmpu_mem = (unsigned int*)0xFD000000;
  size_t bytes_to_read = 4 * 5; // 4 bytes = 1 word
  int i;

  // Read RPU memory 
  print("Reading RPU memory...\n\r");
  for (i = 0; i < bytes_to_read/4; i++) {
    xil_printf("ptr_rpu_mem[%d] = 0x%08X\t", i, ptr_rpu_mem[i]);
  }
  print("\n\n\r");
  // // Write in RPU memory
  // print("Writing RPU memory...\n\r"); 
  // for (i = 0; i < bytes_to_read/4; i++) {
  //   ptr_rpu_mem[i] = 0xFFFFFFFF;
  //   xil_printf("ptr_rpu_mem[%d] = 0x%08X\t",i, ptr_rpu_mem[i]);
  // }
  // print("\n\n\r");

  print("Reading FPD_XMPU memory...\n\r");
  for (i = 0; i < bytes_to_read/4; i++) {
    xil_printf("ptr_xmpu_mem[%d] = 0x%08X\t", i, ptr_xmpu_mem[i]);
  }
  print("\n\n\r");
  
  // Read APU memory
  print("Reading APU memory...\n\r");
  for (i = 0; i < bytes_to_read/4; i++) {
    xil_printf("ptr_apu_mem[%d] = 0x%08X\t", i, ptr_apu_mem[i]);
  }
  print("\n\n\r");
  // // Write in APU memory
  // print("Writing APU memory...\n\r");
  // for (i = 0; i < bytes_to_read/4; i++) {
  //   ptr_apu_mem[i] = 0xFFFFFFFF;
  //   xil_printf("ptr_apu_mem[%d] = 0x%08X\t",i, ptr_apu_mem[i]);
  // }
  // print("\n\n\r");

}


int main()
{
  u32 *mem_array = (u32 *)0x3EE00000;
  u32 *shared_memory = (u32 *)0x46d00000;
  XTime start, end, diff;
  u32 readsum = 0;
  u32 time_us = 0;
  int i, j, k;

  // Initialize the Platform
  init_platform();  
  Xil_DCacheDisable();
  Xil_ICacheDisable();
  
  // DEBUG
  //xil_printf("Start!\n\r");
  //xil_printf("The Test will last %d s\n\r", (PERIOD*REP_TIME)/1000000);
  //xil_printf("Repetitions: %d\n\r", REP_TIME);
  //xil_printf("Task Period: %d us\n\r", PERIOD);
  //xil_printf("Timer Frequency: %d MHz\n\r", FREQUENCY);

  // DEBUG
  // test_memory_protection();
  // cleanup_platform();
  // return 0;

  // Mem Array Initialization
  for (i = 0; i < DIM; i++) {
    mem_array[i] = i;
  }
  //SHM Array initialization
  for (i = 0; i < REP_TIME; i++) {
    shared_memory[i] = 0;
  }

 for(k = 0; k < REP_TIME; k++ ){ 
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
    
    shared_memory[k] = time_us;
    
    // DEBUG
    // xil_printf("%llu\n\r", time_us);
    
    // wait until the end of the period
    while (time_us < PERIOD){
      XTime_GetTime(&end);
      diff = end - start;
      time_us = diff / FREQUENCY;
    }
  }

  // DEBUG
  // xil_printf("Successfully ran the application\n\r");

  cleanup_platform();
  return 0;
}
