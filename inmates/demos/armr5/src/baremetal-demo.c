#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "sleep.h"

#define NPAGES 1024
#define DIM 128 * NPAGES // 123*32(size fo int)=4096 bytes= 4KB(size of one page)
#define REP 10
#define REP_TIME 100
#define FREQUENCY 8  // 8 MHz
#define PERIOD 100000 // 100 ms

int main()
{
  u32 *mem_array = (u32 *)0x3EE00000;
  XTime start, end, diff;
  u32 readsum = 0;
  u32 time_us = 0;
  int i, j, k;

  // Initialize the Platform
  init_platform();  
  xil_printf("Start!\n\r");

  // Mem Array Initialization
  for (i = 0; i < DIM; i++) {
    mem_array[i] = i;
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

    // xil_printf("time(us): ");
    xil_printf("%llu\n\r", time_us);
    
    // wait until the end of the period
    while (time_us < PERIOD){
      XTime_GetTime(&end);
      diff = end - start;
      time_us = diff / FREQUENCY;
    }
  }


  xil_printf("Successfully ran the application\n\r");

  cleanup_platform();
  return 0;
}
