#ifndef BITONIC_H
#define BITONIC_H

/*
time: 44
clock cycles: 189350
*/

/*
  Forward declaration of functions
*/
void bitonic_init( void );
void bitonic_main( void );
int bitonic_return( void );
void bitonic_compare( int i, int j, int dir );
void bitonic_merge( int lo, int cnt, int dir );
void bitonic_sort( int lo, int cnt, int dir );

#endif /*BITONIC_H*/