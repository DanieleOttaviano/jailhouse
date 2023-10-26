
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "sleep.h"

#define NPAGES 1024
#define DIM 128 * NPAGES // 123*32(size fo int)=4096 bytes= 4KB(size of one page)
#define REP 100
#define REP_TIME 100
#define FREQUENCY 80 // 80 MHz

int main()
{
  u32 *mem_array = (u32 *)0x3EE00000;
  XTime start, end, diff;
  u32 readsum = 0;
  // u32 avglat = 0;  
  u32 time_ns = 0;
  int i, j, k;

  // Initialize the Platform
  init_platform();
  
  xil_printf("Start!\n\r");

  // Initialization
  for (i = 0; i < DIM; i++) {
    mem_array[i] = i;
    // xil_printf("mem_array[%d]: %d\n\r", i, mem_array[i]);
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

    diff = end - start;
    time_ns = diff / FREQUENCY;
    // avglat = diff / DIM / REP; // Calculate average latency per access over all repetitions

    // xil_printf("diff: %llu\n\r", diff);
    xil_printf("time(ns): %llu\n\r", time_ns);
    // xil_printf("avglat: %llu\n\r", avglat);
  }


  xil_printf("Successfully ran the application\n\r");

  cleanup_platform();
  return 0;
}
