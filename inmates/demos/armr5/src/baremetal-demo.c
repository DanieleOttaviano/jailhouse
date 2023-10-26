
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"


int main()
{
 //Initialize the Plaftorm
  init_platform();
  print("Start!\n\r");

  print("Successfully ran application\n\r");

  cleanup_platform();
  return 0;
}
