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
#include "platform.h"
#include "xil_printf.h"


// Base address of the XMPU0
#define DDR_XMPUx_CFG 0x00FD000000

// XMPU OFFSETT
#define XMPU0 0x00000
#define XMPU1 0x10000
#define XMPU2 0x20000
#define XMPU3 0x30000
#define XMPU4 0x40000
#define XMPU5 0x50000

// Offsets for the registers
#define CTRL        0x0000
#define ERR_STATUS1 0x0004
#define ERR_STATUS2 0x0008
#define POISON      0x000C
#define ISR         0x0010
#define IMR         0x0014
#define IEN         0x0018
#define IDS         0x001C
#define LOCK        0x0020

#define RX_START    0x0100
#define RX_END      0x0104
#define RX_MASTER   0x0108
#define RX_CONFIG   0x010C

#define R00         0x0000
#define R01         0x0010
#define R02         0x0020
#define R03         0x0030
#define R04         0x0040
#define R05         0x0050
#define R06         0x0060
#define R07         0x0070
#define R08         0x0080
#define R09         0x0090
#define R10         0x00A0
#define R11         0x00B0
#define R12         0x00C0
#define R13         0x00D0
#define R14         0x00E0
#define R15         0x00F0

// Calculate the actual addresses of the registers
volatile uint32_t *XMPU0_CTRL_REG =        (volatile uint32_t *)(DDR_XMPUx_CFG + CTRL         + XMPU0);
volatile uint32_t *XMPU0_ERR_STATUS1_REG = (volatile uint32_t *)(DDR_XMPUx_CFG + ERR_STATUS1  + XMPU0);
volatile uint32_t *XMPU0_ERR_STATUS2_REG = (volatile uint32_t *)(DDR_XMPUx_CFG + ERR_STATUS2  + XMPU0);
volatile uint32_t *XMPU0_POISON_REG =      (volatile uint32_t *)(DDR_XMPUx_CFG + POISON       + XMPU0);
volatile uint32_t *XMPU0_ISR_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + ISR          + XMPU0);
volatile uint32_t *XMPU0_IMR_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IMR          + XMPU0);
volatile uint32_t *XMPU0_IEN_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IEN          + XMPU0);
volatile uint32_t *XMPU0_IDS_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IDS          + XMPU0);
volatile uint32_t *XMPU0_LOCK_REG =        (volatile uint32_t *)(DDR_XMPUx_CFG + LOCK         + XMPU0);

volatile uint32_t *XMPU0_R00_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R00 + XMPU0);
volatile uint32_t *XMPU0_R00_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R00 + XMPU0);
volatile uint32_t *XMPU0_R00_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R00 + XMPU0);
volatile uint32_t *XMPU0_R00_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R00 + XMPU0);

volatile uint32_t *XMPU0_R01_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R01 + XMPU0);
volatile uint32_t *XMPU0_R01_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R01 + XMPU0);
volatile uint32_t *XMPU0_R01_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R01 + XMPU0);
volatile uint32_t *XMPU0_R01_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R01 + XMPU0);

volatile uint32_t *XMPU0_R02_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R02 + XMPU0);
volatile uint32_t *XMPU0_R02_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R02 + XMPU0);
volatile uint32_t *XMPU0_R02_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R02 + XMPU0);
volatile uint32_t *XMPU0_R02_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R02 + XMPU0);

volatile uint32_t *XMPU0_R03_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R03 + XMPU0);
volatile uint32_t *XMPU0_R03_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R03 + XMPU0);
volatile uint32_t *XMPU0_R03_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R03 + XMPU0);
volatile uint32_t *XMPU0_R03_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R03 + XMPU0);

volatile uint32_t *XMPU0_R04_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R04 + XMPU0);
volatile uint32_t *XMPU0_R04_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R04 + XMPU0);
volatile uint32_t *XMPU0_R04_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R04 + XMPU0);
volatile uint32_t *XMPU0_R04_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R04 + XMPU0);

volatile uint32_t *XMPU0_R05_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R05 + XMPU0);
volatile uint32_t *XMPU0_R05_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R05 + XMPU0);
volatile uint32_t *XMPU0_R05_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R05 + XMPU0);
volatile uint32_t *XMPU0_R05_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R05 + XMPU0);

volatile uint32_t *XMPU0_R06_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R06 + XMPU0);
volatile uint32_t *XMPU0_R06_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R06 + XMPU0);
volatile uint32_t *XMPU0_R06_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R06 + XMPU0);
volatile uint32_t *XMPU0_R06_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R06 + XMPU0);

volatile uint32_t *XMPU0_R07_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R07 + XMPU0);
volatile uint32_t *XMPU0_R07_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R07 + XMPU0);
volatile uint32_t *XMPU0_R07_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R07 + XMPU0);
volatile uint32_t *XMPU0_R07_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R07 + XMPU0);

volatile uint32_t *XMPU0_R08_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R08 + XMPU0);
volatile uint32_t *XMPU0_R08_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R08 + XMPU0);
volatile uint32_t *XMPU0_R08_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R08 + XMPU0);
volatile uint32_t *XMPU0_R08_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R08 + XMPU0);

volatile uint32_t *XMPU0_R09_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R09 + XMPU0);
volatile uint32_t *XMPU0_R09_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R09 + XMPU0);
volatile uint32_t *XMPU0_R09_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R09 + XMPU0);
volatile uint32_t *XMPU0_R09_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R09 + XMPU0);

volatile uint32_t *XMPU0_R10_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R10 + XMPU0);
volatile uint32_t *XMPU0_R10_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R10 + XMPU0);
volatile uint32_t *XMPU0_R10_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R10 + XMPU0);
volatile uint32_t *XMPU0_R10_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R10 + XMPU0);

volatile uint32_t *XMPU0_R11_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R11 + XMPU0);
volatile uint32_t *XMPU0_R11_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R11 + XMPU0);
volatile uint32_t *XMPU0_R11_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R11 + XMPU0);
volatile uint32_t *XMPU0_R11_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R11 + XMPU0);

volatile uint32_t *XMPU0_R12_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R12 + XMPU0);
volatile uint32_t *XMPU0_R12_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R12 + XMPU0);
volatile uint32_t *XMPU0_R12_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R12 + XMPU0);
volatile uint32_t *XMPU0_R12_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R12 + XMPU0);

volatile uint32_t *XMPU0_R13_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R13 + XMPU0);
volatile uint32_t *XMPU0_R13_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R13 + XMPU0);
volatile uint32_t *XMPU0_R13_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R13 + XMPU0);
volatile uint32_t *XMPU0_R13_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R13 + XMPU0);

volatile uint32_t *XMPU0_R14_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R14 + XMPU0);
volatile uint32_t *XMPU0_R14_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R14 + XMPU0);
volatile uint32_t *XMPU0_R14_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R14 + XMPU0);
volatile uint32_t *XMPU0_R14_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R14 + XMPU0);

volatile uint32_t *XMPU0_R15_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R15 + XMPU0);
volatile uint32_t *XMPU0_R15_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R15 + XMPU0);
volatile uint32_t *XMPU0_R15_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R15 + XMPU0);
volatile uint32_t *XMPU0_R15_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R15 + XMPU0);

// XMPU1
volatile uint32_t *XMPU1_CTRL_REG =        (volatile uint32_t *)(DDR_XMPUx_CFG + CTRL         + XMPU1);
volatile uint32_t *XMPU1_ERR_STATUS1_REG = (volatile uint32_t *)(DDR_XMPUx_CFG + ERR_STATUS1  + XMPU1);
volatile uint32_t *XMPU1_ERR_STATUS2_REG = (volatile uint32_t *)(DDR_XMPUx_CFG + ERR_STATUS2  + XMPU1);
volatile uint32_t *XMPU1_POISON_REG =      (volatile uint32_t *)(DDR_XMPUx_CFG + POISON       + XMPU1);
volatile uint32_t *XMPU1_ISR_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + ISR          + XMPU1);
volatile uint32_t *XMPU1_IMR_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IMR          + XMPU1);
volatile uint32_t *XMPU1_IEN_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IEN          + XMPU1);
volatile uint32_t *XMPU1_IDS_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IDS          + XMPU1);
volatile uint32_t *XMPU1_LOCK_REG =        (volatile uint32_t *)(DDR_XMPUx_CFG + LOCK         + XMPU1);

volatile uint32_t *XMPU1_R00_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R00 + XMPU1);
volatile uint32_t *XMPU1_R00_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R00 + XMPU1);
volatile uint32_t *XMPU1_R00_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R00 + XMPU1);
volatile uint32_t *XMPU1_R00_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R00 + XMPU1);

// XMPU2
volatile uint32_t *XMPU2_CTRL_REG =        (volatile uint32_t *)(DDR_XMPUx_CFG + CTRL         + XMPU2);
volatile uint32_t *XMPU2_ERR_STATUS1_REG = (volatile uint32_t *)(DDR_XMPUx_CFG + ERR_STATUS1  + XMPU2);
volatile uint32_t *XMPU2_ERR_STATUS2_REG = (volatile uint32_t *)(DDR_XMPUx_CFG + ERR_STATUS2  + XMPU2);
volatile uint32_t *XMPU2_POISON_REG =      (volatile uint32_t *)(DDR_XMPUx_CFG + POISON       + XMPU2);
volatile uint32_t *XMPU2_ISR_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + ISR          + XMPU2);
volatile uint32_t *XMPU2_IMR_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IMR          + XMPU2);
volatile uint32_t *XMPU2_IEN_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IEN          + XMPU2);
volatile uint32_t *XMPU2_IDS_REG =         (volatile uint32_t *)(DDR_XMPUx_CFG + IDS          + XMPU2);
volatile uint32_t *XMPU2_LOCK_REG =        (volatile uint32_t *)(DDR_XMPUx_CFG + LOCK         + XMPU2);

volatile uint32_t *XMPU2_R00_START_REG =   (volatile uint32_t *)(DDR_XMPUx_CFG + RX_START  + R00 + XMPU2);
volatile uint32_t *XMPU2_R00_END_REG =     (volatile uint32_t *)(DDR_XMPUx_CFG + RX_END    + R00 + XMPU2);
volatile uint32_t *XMPU2_R00_MASTER_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_MASTER + R00 + XMPU2);
volatile uint32_t *XMPU2_R00_CONFIG_REG =  (volatile uint32_t *)(DDR_XMPUx_CFG + RX_CONFIG + R00 + XMPU2);


// Print the XMPU0 registers
void print_xmpu0_regs(){
  xil_printf("XMPU0 Registers:\n\r");
  xil_printf("CTRL:        0x%08X\n\r", *XMPU0_CTRL_REG);
  xil_printf("ERR_STATUS1: 0x%08X\n\r", *XMPU0_ERR_STATUS1_REG);
  xil_printf("ERR_STATUS2: 0x%08X\n\r", *XMPU0_ERR_STATUS2_REG);
  xil_printf("POISON:      0x%08X\n\r", *XMPU0_POISON_REG);
  xil_printf("ISR:         0x%08X\n\r", *XMPU0_ISR_REG);
  xil_printf("IMR:         0x%08X\n\r", *XMPU0_IMR_REG);
  xil_printf("IEN:         0x%08X\n\r", *XMPU0_IEN_REG);
  xil_printf("IDS:         0x%08X\n\r", *XMPU0_IDS_REG);
  xil_printf("LOCK:        0x%08X\n\r", *XMPU0_LOCK_REG);
  xil_printf("\n\r");
  xil_printf("R00_START:   0x%08X\n\r", *XMPU0_R00_START_REG);
  xil_printf("R00_END:     0x%08X\n\r", *XMPU0_R00_END_REG);
  xil_printf("R00_MASTER:  0x%08X\n\r", *XMPU0_R00_MASTER_REG);
  xil_printf("R00_CONFIG:  0x%08X\n\r", *XMPU0_R00_CONFIG_REG);
  xil_printf("\n\r");
  //xil_printf("R01_START:   0x%08X\n\r", *XMPU0_R01_START_REG);
  //xil_printf("R01_END:     0x%08X\n\r", *XMPU0_R01_END_REG);
  //xil_printf("R01_MASTER:  0x%08X\n\r", *XMPU0_R01_MASTER_REG);
  //xil_printf("R01_CONFIG:  0x%08X\n\r", *XMPU0_R01_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R02_START:   0x%08X\n\r", *XMPU0_R02_START_REG);
  //xil_printf("R02_END:     0x%08X\n\r", *XMPU0_R02_END_REG);
  //xil_printf("R02_MASTER:  0x%08X\n\r", *XMPU0_R02_MASTER_REG);
  //xil_printf("R02_CONFIG:  0x%08X\n\r", *XMPU0_R02_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R03_START:   0x%08X\n\r", *XMPU0_R03_START_REG);
  //xil_printf("R03_END:     0x%08X\n\r", *XMPU0_R03_END_REG);
  //xil_printf("R03_MASTER:  0x%08X\n\r", *XMPU0_R03_MASTER_REG);
  //xil_printf("R03_CONFIG:  0x%08X\n\r", *XMPU0_R03_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R04_START:   0x%08X\n\r", *XMPU0_R04_START_REG);
  //xil_printf("R04_END:     0x%08X\n\r", *XMPU0_R04_END_REG);
  //xil_printf("R04_MASTER:  0x%08X\n\r", *XMPU0_R04_MASTER_REG);
  //xil_printf("R04_CONFIG:  0x%08X\n\r", *XMPU0_R04_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R05_START:   0x%08X\n\r", *XMPU0_R05_START_REG);
  //xil_printf("R05_END:     0x%08X\n\r", *XMPU0_R05_END_REG);
  //xil_printf("R05_MASTER:  0x%08X\n\r", *XMPU0_R05_MASTER_REG);
  //xil_printf("R05_CONFIG:  0x%08X\n\r", *XMPU0_R05_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R06_START:   0x%08X\n\r", *XMPU0_R06_START_REG);
  //xil_printf("R06_END:     0x%08X\n\r", *XMPU0_R06_END_REG);
  //xil_printf("R06_MASTER:  0x%08X\n\r", *XMPU0_R06_MASTER_REG);
  //xil_printf("R06_CONFIG:  0x%08X\n\r", *XMPU0_R06_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R07_START:   0x%08X\n\r", *XMPU0_R07_START_REG);
  //xil_printf("R07_END:     0x%08X\n\r", *XMPU0_R07_END_REG);
  //xil_printf("R07_MASTER:  0x%08X\n\r", *XMPU0_R07_MASTER_REG);
  //xil_printf("R07_CONFIG:  0x%08X\n\r", *XMPU0_R07_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R08_START:   0x%08X\n\r", *XMPU0_R08_START_REG);
  //xil_printf("R08_END:     0x%08X\n\r", *XMPU0_R08_END_REG);
  //xil_printf("R08_MASTER:  0x%08X\n\r", *XMPU0_R08_MASTER_REG);
  //xil_printf("R08_CONFIG:  0x%08X\n\r", *XMPU0_R08_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R09_START:   0x%08X\n\r", *XMPU0_R09_START_REG);
  //xil_printf("R09_END:     0x%08X\n\r", *XMPU0_R09_END_REG);
  //xil_printf("R09_MASTER:  0x%08X\n\r", *XMPU0_R09_MASTER_REG);
  //xil_printf("R09_CONFIG:  0x%08X\n\r", *XMPU0_R09_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R10_START:   0x%08X\n\r", *XMPU0_R10_START_REG);
  //xil_printf("R10_END:     0x%08X\n\r", *XMPU0_R10_END_REG);
  //xil_printf("R10_MASTER:  0x%08X\n\r", *XMPU0_R10_MASTER_REG);
  //xil_printf("R10_CONFIG:  0x%08X\n\r", *XMPU0_R10_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R11_START:   0x%08X\n\r", *XMPU0_R11_START_REG);
  //xil_printf("R11_END:     0x%08X\n\r", *XMPU0_R11_END_REG);
  //xil_printf("R11_MASTER:  0x%08X\n\r", *XMPU0_R11_MASTER_REG);
  //xil_printf("R11_CONFIG:  0x%08X\n\r", *XMPU0_R11_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R12_START:   0x%08X\n\r", *XMPU0_R12_START_REG);
  //xil_printf("R12_END:     0x%08X\n\r", *XMPU0_R12_END_REG);
  //xil_printf("R12_MASTER:  0x%08X\n\r", *XMPU0_R12_MASTER_REG);
  //xil_printf("R12_CONFIG:  0x%08X\n\r", *XMPU0_R12_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R13_START:   0x%08X\n\r", *XMPU0_R13_START_REG);
  //xil_printf("R13_END:     0x%08X\n\r", *XMPU0_R13_END_REG);
  //xil_printf("R13_MASTER:  0x%08X\n\r", *XMPU0_R13_MASTER_REG);
  //xil_printf("R13_CONFIG:  0x%08X\n\r", *XMPU0_R13_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R14_START:   0x%08X\n\r", *XMPU0_R14_START_REG);
  //xil_printf("R14_END:     0x%08X\n\r", *XMPU0_R14_END_REG);
  //xil_printf("R14_MASTER:  0x%08X\n\r", *XMPU0_R14_MASTER_REG);
  //xil_printf("R14_CONFIG:  0x%08X\n\r", *XMPU0_R14_CONFIG_REG);
  //xil_printf("\n\r");
  //xil_printf("R15_START:   0x%08X\n\r", *XMPU0_R15_START_REG);
  //xil_printf("R15_END:     0x%08X\n\r", *XMPU0_R15_END_REG);
  //xil_printf("R15_MASTER:  0x%08X\n\r", *XMPU0_R15_MASTER_REG);
  //xil_printf("R15_CONFIG:  0x%08X\n\r", *XMPU0_R15_CONFIG_REG);
  //xil_printf("\n\r");
}

// Configure XPMU0 register
void set_xmpu0_rpu(){  
  /* START_REG
  [31:28] -> Reserved
  [27:8] -> Correspond to address bit [39:20]
  [7:0] -> Reserved
  */
  *XMPU0_R00_START_REG =    0x0003ED00; // Start address 

  /* END_REG
  [31:28] -> Reserved
  [27:8] -> Correspond to address bit [39:20]
  [7:0] -> Reserved
  */
  *XMPU0_R00_END_REG =      0x0003EE00; // End address 

  /* MASTER_REG
  [31:26] -> Reserved
  [25:16] -> Mask
  [15:10] -> Reserved
  [9:0] -> Master ID
  */
  *XMPU0_R00_MASTER_REG =   0x03E00000; // Reserved 0000 00 | Mask: 11 1110 0000 | Reserved: 0000 00 |  MASTER ID: 0000 00 0000

  /* CONFIG_REG
  [31:5] -> Reserved
  [4] -> Select Security Level of region 0: Secure, 1: Non Secure
  [3] -> Allow write
  [2] -> Allow read
  [1] -> Enable XMPU region
  [0] -> Reserved
  */  
  *XMPU0_R00_CONFIG_REG =   0x00000007; // Non Secure : 0, Allow write: 1, Allow read: 1, Enable XMPU region: 1.



  /* CTRL_REG
  [31:4] -> Reserved
  [3] -> Align 1MB: 1, Align 4KB: 0
  [2] -> Attr poison: 1, Attr no poison: 0
  [1] -> Default Write Permissions: 1,
  [0] -> Default Read Permissions: 1
  */
  *XMPU0_CTRL_REG =         0x00000008; // Align 1MB: 1, Attr poison: 0, Write Default: 0, Read Default: 0
  
  //*ERR_STATUS1_REG =  0x00000000;
  //*ERR_STATUS2_REG =  0x00000000; 

  /* POISON_REG
  [31:21] -> Reserved
  [20] -> Poison attribute
  [19:0] -> Reserved. The DDR memory controller expects poisoning by attribute. 
  */
  *XMPU0_POISON_REG =       0x00100000; // Enable address poisoning

  //*ISR_REG =          0x00000000;
  //*IMR_REG =          0x00000000;
  //*IEN_REG =          0x00000000;
  //*IDS_REG =          0x00000000;
  //*LOCK_REG =         0x00000000; 
  

}


// Print the XMPU1 registers
void print_xmpu1_regs(){
  xil_printf("XMPU1 Registers:\n\r");
  xil_printf("CTRL:        0x%08X\n\r", *XMPU1_CTRL_REG);
  xil_printf("ERR_STATUS1: 0x%08X\n\r", *XMPU1_ERR_STATUS1_REG);
  xil_printf("ERR_STATUS2: 0x%08X\n\r", *XMPU1_ERR_STATUS2_REG);
  xil_printf("POISON:      0x%08X\n\r", *XMPU1_POISON_REG);
  xil_printf("ISR:         0x%08X\n\r", *XMPU1_ISR_REG);
  xil_printf("IMR:         0x%08X\n\r", *XMPU1_IMR_REG);
  xil_printf("IEN:         0x%08X\n\r", *XMPU1_IEN_REG);
  xil_printf("IDS:         0x%08X\n\r", *XMPU1_IDS_REG);
  xil_printf("LOCK:        0x%08X\n\r", *XMPU1_LOCK_REG);
  xil_printf("\n\r");
  xil_printf("R00_START:   0x%08X\n\r", *XMPU1_R00_START_REG);
  xil_printf("R00_END:     0x%08X\n\r", *XMPU1_R00_END_REG);
  xil_printf("R00_MASTER:  0x%08X\n\r", *XMPU1_R00_MASTER_REG);
  xil_printf("R00_CONFIG:  0x%08X\n\r", *XMPU1_R00_CONFIG_REG);
  xil_printf("\n\r");
}

// Configure XPMU1 register
void set_xmpu1_apu(){ 
  *XMPU1_R00_START_REG =    0x0003ED00; // Start address 
  *XMPU1_R00_END_REG =      0x0003EE00; // End address 
  *XMPU1_R00_MASTER_REG =   0x03C00080; // Reserved 0000 00 | Mask: 11 1100 0000 | Reserved: 0000 00 |  MASTER ID: 0010 00 0000  
  *XMPU1_R00_CONFIG_REG =   0x00000001; // Non Secure : 0, Allow write: 1, Allow read: 1, Enable XMPU region: 1.

  *XMPU1_CTRL_REG =         0x0000000B; // Align 1MB: 1, Attr poison: 0, Write Default: 1, Read Default: 1 
  *XMPU1_POISON_REG =       0x00100000; // Enable address poisoning
}


// Print the XMPU2 registers
void print_xmpu2_regs(){
  xil_printf("XMPU2 Registers:\n\r");
  xil_printf("CTRL:        0x%08X\n\r", *XMPU2_CTRL_REG);
  xil_printf("ERR_STATUS1: 0x%08X\n\r", *XMPU2_ERR_STATUS1_REG);
  xil_printf("ERR_STATUS2: 0x%08X\n\r", *XMPU2_ERR_STATUS2_REG);
  xil_printf("POISON:      0x%08X\n\r", *XMPU2_POISON_REG);
  xil_printf("ISR:         0x%08X\n\r", *XMPU2_ISR_REG);
  xil_printf("IMR:         0x%08X\n\r", *XMPU2_IMR_REG);
  xil_printf("IEN:         0x%08X\n\r", *XMPU2_IEN_REG);
  xil_printf("IDS:         0x%08X\n\r", *XMPU2_IDS_REG);
  xil_printf("LOCK:        0x%08X\n\r", *XMPU2_LOCK_REG);
  xil_printf("\n\r");
  xil_printf("R00_START:   0x%08X\n\r", *XMPU2_R00_START_REG);
  xil_printf("R00_END:     0x%08X\n\r", *XMPU2_R00_END_REG);
  xil_printf("R00_MASTER:  0x%08X\n\r", *XMPU2_R00_MASTER_REG);
  xil_printf("R00_CONFIG:  0x%08X\n\r", *XMPU2_R00_CONFIG_REG);
  xil_printf("\n\r");
}

// Configure XPMU2 register
void set_xmpu2_apu(){
  *XMPU2_R00_START_REG =    0x0003ED00; // Start address 
  *XMPU2_R00_END_REG =      0x0003EE00; // End address 
  *XMPU2_R00_MASTER_REG =   0x03C00080; // Reserved 0000 00 | Mask: 11 1100 0000 | Reserved: 0000 00 |  MASTER ID: 0010 00 0000  
  *XMPU2_R00_CONFIG_REG =   0x00000001; // Non Secure : 0, Allow write: 1, Allow read: 1, Enable XMPU region: 1.

  *XMPU2_CTRL_REG =         0x0000000B; // Align 1MB: 1, Attr poison: 0, Write Default: 1, Read Default: 1 
  *XMPU2_POISON_REG =       0x00100000; // Enable address poisoning
}




int main()
{
  //Pointer to RPU memory
  unsigned int* ptr_rpu_mem = (unsigned int*)0x3ed01000;
  unsigned int* ptr_apu_mem = (unsigned int*)0x75609000;
  //unsigned char* ptr_kernel = (unsigned char*)0x01560000;
  size_t bytes_to_read = 32; // Read 32 bytes
  
  init_platform();
  print("\n\r");
  print("Start!\n\n\r");

  // Program XMPU0 registers to protect APU from RPU accesses
  set_xmpu0_rpu();

  // Print XMPU0 registers
  print_xmpu0_regs();

  // Program XMPU1 and XMPU2 registers to protect RPU from APU accesses
  set_xmpu1_apu();
  set_xmpu2_apu();

  // Print XMPU1 registers
  print_xmpu1_regs();

  // Print XMPU2 registers
  print_xmpu2_regs();

  // Read RPU memory
  print("Reading RPU memory...\n\r");
  for (int i = 0; i < bytes_to_read/4; i++) {
    xil_printf("ptr_rpu_mem[%d] = 0x%08X\t", i, ptr_rpu_mem[i]);
  }
  print("\n\n\r");
  // Write in RPU memory
  print("Writing RPU memory...\n\r"); 
  for (int i = 0; i < bytes_to_read/4; i++) {
    ptr_rpu_mem[i] = 0xFFFFFFFF;
    xil_printf("ptr_rpu_mem[%d] = 0x%08X\t",i, ptr_rpu_mem[i]);
  }
  print("\n\n\r");


  // Read APU memory
  print("Reading APU memory...\n\r");
  for (int i = 0; i < bytes_to_read/4; i++) {
    xil_printf("ptr_apu_mem[%d] = 0x%08X\t", i, ptr_apu_mem[i]);
  }
  print("\n\n\r");
  // Write in APU memory
  print("Writing APU memory...\n\r");
  for (int i = 0; i < bytes_to_read/4; i++) {
    ptr_apu_mem[i] = 0xFFFFFFFF;
    xil_printf("ptr_apu_mem[%d] = 0x%08X\t",i, ptr_apu_mem[i]);
  }
  print("\n\n\r");



  print("Successfully ran application\n\n\r");

  cleanup_platform();
  return 0;
}
