/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "platform.h"
#include "xil_printf.h"
#include "xmpu.h"


int main()
{
  //XMPU configurations
  xmpu_status_config xmpu0_status_config;
  xmpu_region_config xmpu0_region_config;
  
  xmpu_status_config fpd_xmpu_status_config;
  xmpu_region_config fpd_xmpu_region_config;
  
  xmpu_status_config xmpu1_status_config;
  xmpu_region_config xmpu1_region_config;
  
  xmpu_status_config xmpu2_status_config;
  xmpu_region_config xmpu2_region_config;
  
  //Pointer to RPU memory
  unsigned int* ptr_rpu_mem = (unsigned int*)0x3ed01000;
  //Pointer to APU memory
  unsigned int* ptr_apu_mem = (unsigned int*)0x75609000;  
  size_t bytes_to_read = 4; // Read 4 bytes
  int i;

  //Initialize the Plaftorm
  init_platform();
  print("\n\r");
  print("Start!\n\n\r");


  // Configure XMPU0 to protect APU memory from RPU accesses
  xmpu0_region_config.addr_start =    0x3ED00000;
  xmpu0_region_config.addr_end =      0x3EEFFFFF;
  xmpu0_region_config.master_id =     0x0000;
  xmpu0_region_config.master_mask =   0x03E0;
  xmpu0_region_config.ns_checktype =  0;
  xmpu0_region_config.region_ns =     0;
  xmpu0_region_config.wrallowed =     1;
  xmpu0_region_config.rdallowed =     1;
  xmpu0_region_config.enable =        1;  
  set_xmpu_region(XMPU_DDR_0_BASE_ADDR, R00_OFFSET, &xmpu0_region_config);
  xmpu0_status_config.poison =        1;
  xmpu0_status_config.align =         1;
  xmpu0_status_config.def_wr_allowed =0;
  xmpu0_status_config.def_rd_allowed =0;
  xmpu0_status_config.lock =          0;
  set_xmpu_status(XMPU_DDR_0_BASE_ADDR, &xmpu0_status_config);

  // Configure XMPU1 to protect RPU memory from APU accesses
  xmpu1_region_config.addr_start =    0x3ED00000;
  xmpu1_region_config.addr_end =      0x3EEFFFFF;
  xmpu1_region_config.master_id =     0x0080;
  xmpu1_region_config.master_mask =   0x03C0;
  xmpu1_region_config.ns_checktype =  0;
  xmpu1_region_config.region_ns =     0;
  xmpu1_region_config.wrallowed =     0;
  xmpu1_region_config.rdallowed =     0;
  xmpu1_region_config.enable =        1;
  set_xmpu_region(XMPU_DDR_1_BASE_ADDR, R00_OFFSET, &xmpu1_region_config);  
  xmpu1_status_config.poison =        1;
  xmpu1_status_config.align =         1;
  xmpu1_status_config.def_wr_allowed =1;
  xmpu1_status_config.def_rd_allowed =1;
  xmpu1_status_config.lock =          0;
  set_xmpu_status(XMPU_DDR_1_BASE_ADDR, &xmpu1_status_config);


  // Configure XMPU2 to protect RPU memory from APU acceses
  xmpu2_region_config.addr_start =    0x3ED00000;
  xmpu2_region_config.addr_end =      0x3EEFFFFF;
  xmpu2_region_config.master_id =     0x0080;
  xmpu2_region_config.master_mask =   0x03C0;
  xmpu2_region_config.ns_checktype =  0;
  xmpu2_region_config.region_ns =     0;
  xmpu2_region_config.wrallowed =     0;
  xmpu2_region_config.rdallowed =     0;
  xmpu2_region_config.enable =        1;
  set_xmpu_region(XMPU_DDR_2_BASE_ADDR, R00_OFFSET, &xmpu2_region_config);
  xmpu2_status_config.poison =        1;
  xmpu2_status_config.align =         1;
  xmpu2_status_config.def_wr_allowed =1;
  xmpu2_status_config.def_rd_allowed =1;
  xmpu2_status_config.lock =          0;
  set_xmpu_status(XMPU_DDR_2_BASE_ADDR, &xmpu2_status_config);


  // Print XMPU registers
  xil_printf("XMPU0 registers:\n\r");
  print_xmpu_status_regs(XMPU_DDR_0_BASE_ADDR);
  print_xmpu_region_regs(XMPU_DDR_0_BASE_ADDR, R00_OFFSET);

  xil_printf("FPD_XMPU registers:\n\r");
  print_xmpu_status_regs(XMPU_FPD_BASE_ADDR);
  print_xmpu_region_regs(XMPU_FPD_BASE_ADDR, R00_OFFSET);

  xil_printf("XMPU1 registers:\n\r");
  print_xmpu_status_regs(XMPU_DDR_1_BASE_ADDR);
  print_xmpu_region_regs(XMPU_DDR_1_BASE_ADDR, R00_OFFSET);
  xil_printf("XMPU2 registers:\n\r");
  print_xmpu_status_regs(XMPU_DDR_2_BASE_ADDR);
  print_xmpu_region_regs(XMPU_DDR_2_BASE_ADDR, R00_OFFSET);


  // Configure FPD_XMPU to protect XMPUs configuration registers from RPU accesses
  fpd_xmpu_region_config.addr_start =    0xFD000000;
  fpd_xmpu_region_config.addr_end =      0xFF9CFF00;
  fpd_xmpu_region_config.master_id =     0x0000;
  fpd_xmpu_region_config.master_mask =   0x03E0;
  fpd_xmpu_region_config.ns_checktype =  0;
  fpd_xmpu_region_config.region_ns =     0;
  fpd_xmpu_region_config.wrallowed =     0;
  fpd_xmpu_region_config.rdallowed =     0;
  fpd_xmpu_region_config.enable =        1;
  set_xmpu_region(XMPU_FPD_BASE_ADDR, R00_OFFSET, &fpd_xmpu_region_config);
  fpd_xmpu_status_config.poison =        1;
  fpd_xmpu_status_config.align =         0; //4kb
  fpd_xmpu_status_config.def_wr_allowed =1;
  fpd_xmpu_status_config.def_rd_allowed =1;
  fpd_xmpu_status_config.lock =          0;
  set_xmpu_status(XMPU_FPD_BASE_ADDR, &fpd_xmpu_status_config);


  printf("DDR_XMPU0 registers after FPD_XMPU configuration:\n\r");
  print_xmpu_status_regs(XMPU_DDR_0_BASE_ADDR);
  print_xmpu_region_regs(XMPU_DDR_0_BASE_ADDR, R00_OFFSET);
  
  
  // Read RPU memory
  print("Reading RPU memory...\n\r");
  for (i = 0; i < bytes_to_read/4; i++) {
    xil_printf("ptr_rpu_mem[%d] = 0x%08X\t", i, ptr_rpu_mem[i]);
  }
  print("\n\n\r");
  // // Write in RPU memory
  // print("Writing RPU memory...\n\r"); 
  // for (i = 0; i < bytes_to_read/4; i++) {
  //   ptr_rpu_mem[i] = 0xFFFFFFFF;
  //   xil_printf("ptr_rpu_mem[%d] = 0x%08X\t",i, ptr_rpu_mem[i]);
  // }
  // print("\n\n\r");


  // Read APU memory
  print("Reading APU memory...\n\r");
  for (i = 0; i < bytes_to_read/4; i++) {
    xil_printf("ptr_apu_mem[%d] = 0x%08X\t", i, ptr_apu_mem[i]);
  }
  print("\n\n\r");
  // // Write in APU memory
  // print("Writing APU memory...\n\r");
  // for (i = 0; i < bytes_to_read/4; i++) {
  //   ptr_apu_mem[i] = 0xFFFFFFFF;
  //   xil_printf("ptr_apu_mem[%d] = 0x%08X\t",i, ptr_apu_mem[i]);
  // }
  // print("\n\n\r");

  print("Successfully ran application\n\n\r");

  cleanup_platform();
  return 0;
}
