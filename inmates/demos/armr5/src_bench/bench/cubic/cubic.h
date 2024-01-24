#ifndef CUBIC_H
#define CUBIC_H

#include "wcclibm.h"
#include "snipmath.h"


/*
  Forward declaration of functions
*/

void cubic_solveCubic( float a, float b, float c, float d,
                 int *solutions, float *x );
void cubic_main( void );
void cubic_init( void );
int cubic_return( void );

#endif /*CUBIC_H*/