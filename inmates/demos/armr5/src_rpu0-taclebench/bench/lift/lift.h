#ifndef LIFT_H
#define LIFT_H

// sim_cycles 11000000

/*
  Include section
*/
#include "liftlibio.h"
#include "liftlibcontrol.h"

/*
  Forward declaration of functions
*/

void lift_controller();
void lift_init();
void lift_main();
int lift_return();

#endif /*LIFT_H*/