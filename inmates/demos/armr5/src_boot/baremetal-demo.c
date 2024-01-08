#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "sleep.h"
#include "xil_cache.h"

#define DIM_MB 1
#define SIZE_NO_BUFFER 82       // Dimension of the bin without the buffer the buffer
#define SIZE_BIN 1024*DIM_MB    // Dimension of the bin file

static volatile const char buffer[(SIZE_BIN-SIZE_NO_BUFFER)*1024]={0};

int main()
{
  volatile unsigned int* system_counter = (unsigned int*)0xFF250000;
  u32 *shared_memory = (u32 *)0x46d00000;

  // Boot Time
  uint32_t time = *system_counter;
  
  // Initialize the Plaftorm and disable caches
  init_platform();  
  Xil_DCacheDisable();
  Xil_ICacheDisable();

  // Write boot time in shared memory
  shared_memory[0] = time;
 
  //xil_printf("boot_time hex: 0x%08X\n\r", time);  
  //xil_printf("boot_time shm: 0x%08X\n\r", shared_memory[0]);

  // Cleanup the Platform
  cleanup_platform();
  return 0;
}