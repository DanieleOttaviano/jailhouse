#ifndef BITCOUNT_H
#define BITCOUNT_H

/*
time: 64
clock cycles: 273717
*/

#include "bitops.h"
#include <stddef.h>
/*
   First declaration of the functions
*/
int bitcount_bit_shifter( long int x );
unsigned long bitcount_random( void );
void bitcount_main();
int bitcount_return();
void bitcount_init();
void *memcpy(void *dest, const void *src, size_t n);

#endif /*BITCOUNT_H*/