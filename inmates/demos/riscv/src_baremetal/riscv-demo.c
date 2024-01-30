
#include <stdint.h>

volatile static uint32_t * const SHM = (uint32_t *)0x78FF0000;

void main(void){
	volatile uint32_t* system_counter = (uint32_t*)0xFF250000;
	volatile uint32_t *shared_memory = (uint32_t *)0x46d00000;

	// Boot Time
	uint32_t time = *system_counter;

	// Write boot time in shared memory
  	shared_memory[0] = time;

	//DEBUG SHM
	SHM[0] = 0xDEADBEEF;
	SHM[1] = time;
	
	while(1){
		time = *system_counter;
		shared_memory[0] = time;
	}
}

