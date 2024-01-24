#ifndef DIJKSTRA_H
#define DIJKSTRA_H

//sim_cycles TOO LONG

#include "input.h"

/*
  Definitions of symbolic constants
*/
#define NONE 9999
#define OUT_OF_MEMORY -1
#define QUEUE_SIZE 1000

/*
  Type declarations
*/
struct _NODE {
  int dist;
  int prev;
};

struct _QITEM {
  int node;
  int dist;
  int prev;
  struct _QITEM *next;
};

/*
  Forward declaration of functions
*/
void dijkstra_init( void );
int dijkstra_return( void );
int dijkstra_enqueue( int node, int dist, int prev );
void dijkstra_dequeue( int *node, int *dist, int *prev );
int dijkstra_qcount( void );
int dijkstra_find( int chStart, int chEnd );
void dijkstra_main( void );


#endif  /* DIJKSTRA_H */
