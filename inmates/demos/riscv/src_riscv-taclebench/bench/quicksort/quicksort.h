#ifndef __QUICKSORT_H
#define __QUICKSORT_H

struct quicksort_3DVertexStruct {
  unsigned int x, y, z;
  double distance;
};

void quicksort_init( void );
int quicksort_return( void );
void quicksort_main( void );

#endif
