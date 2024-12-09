#ifndef POWERWINDOW_H
#define POWERWINDOW_H

/*
time: 4547
clock cycles: 19559088
mem 128KiB
*/

#include "powerwindow_HeaderFiles/powerwindow.h"
#include "powerwindow_HeaderFiles/powerwindow_PW_Control_PSG_Front.h"
#include "powerwindow_HeaderFiles/powerwindow_PW_Control_PSG_BackL.h"
#include "powerwindow_HeaderFiles/powerwindow_PW_Control_PSG_BackR.h"
#include "powerwindow_HeaderFiles/powerwindow_PW_Control_DRV.h"
#include "powerwindow_HeaderFiles/powerwindow_debounce.h"
#include "powerwindow_HeaderFiles/powerwindow_controlexclusion.h"       /* Control Model's header file */
#include "powerwindow_HeaderFiles/powerwindow_model_reference_types.h"
#include "powerwindow_HeaderFiles/powerwindow_powerwindow_control.h"  /* PW passenger control Model's header file */
#include "powerwindow_HeaderFiles/powerwindow_rtwtypes.h"
#include "powerwindow_HeaderFiles/powerwindow_model_reference_types.h"
/*
  Forward declaration of functions
*/


void powerwindow_Booleaninputarray_initialize( powerwindow_boolean_T *,
    powerwindow_boolean_T * );
void powerwindow_Uint8inputarray_initialize( powerwindow_uint8_T *,
    powerwindow_uint8_T * );
void powerwindow_init();
void powerwindow_main();
int powerwindow_return();


#endif /*POWERWINDOW_H*/
