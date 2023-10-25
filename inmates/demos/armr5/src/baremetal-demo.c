#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"

#define SIZE_NO_BUFFER 73
#define SIZE_BUFFER 1024*10 // Must be higher than SIZE_NO_BUFFER

static volatile const char buffer[(SIZE_BUFFER-SIZE_NO_BUFFER)*1024]={0};

int main()
{
  volatile unsigned int* system_counter = (unsigned int*)0xFF250000;
  uint32_t time = *system_counter;
  
  //Initialize the Plaftorm
  init_platform();

  sleep_R5(1);

  xil_printf("end_time: 0x%08X\n\r", time);  

  cleanup_platform();
  return 0;
}