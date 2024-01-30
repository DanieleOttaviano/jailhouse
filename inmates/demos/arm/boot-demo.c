#include <inmate.h>

#define DIM_MB 90
#define SIZE_NO_BUFFER 39       // Dimension of the bin without the buffer the buffer
#define SIZE_BIN 1024*DIM_MB    // Dimension of the bin file

static volatile char buffer[(SIZE_BIN-SIZE_NO_BUFFER)*1024]={0};

void inmate_main(void)
{
	volatile u32* system_counter = (u32*)0xFF250000;
	volatile u32* shared_memory  = (u32*)0x46d00000;

	map_range((void *)system_counter, 4, MAP_UNCACHED);
	map_range((void *)shared_memory, 4, MAP_UNCACHED);

	// Boot Time
	u32 time = *system_counter;
  
	// Write boot time in shared memory
	shared_memory[0] = time;

	buffer[0] = 'A';
	buffer[(SIZE_BIN-SIZE_NO_BUFFER)*1024-1] = 'Z';

	while(1);
}