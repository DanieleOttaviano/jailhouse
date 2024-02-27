#include <stdint.h>

#define FREQUENCY 100  // 100 MHz
#define I_FREQUENCY 1/FREQUENCY  // 100 MHz

#define CALL(NAME, FUN) CONCAT(NAME, FUN)
#define CONCAT(NAME, FUN)  NAME ## _ ## FUN()

#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

#define INCLUDE_FILE(name) _STRINGIFY(name.h)

#include INCLUDE_FILE(BENCHNAME)

void main(void) {

	volatile uint64_t* system_counter = (uint64_t*)0xFF250000;
	volatile uint64_t* shared_memory = (uint64_t *)0x46d00000;
 	uint64_t start, end, diff;

	shared_memory[0] = 0;
	shared_memory[1] = 0;

	/* Actual access */
	start = *system_counter;

	// Exec the Benchmark 
	CALL(BENCHNAME, init);
	CALL(BENCHNAME, main);

	end = *system_counter;

	// Calculate time in us    
	diff = end - start;

	// time_us = diff * I_FREQUENCY;

	shared_memory[0] = diff;
	shared_memory[1] = FREQUENCY;

	while(1);
}

