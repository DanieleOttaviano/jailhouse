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


/* XMPU/XPPU configiguration register addresses */
#define XMPU_DDR_0_BASE_ADDR      0xFD000000U
#define XMPU_DDR_1_BASE_ADDR      0xFD010000U
#define XMPU_DDR_2_BASE_ADDR      0xFD020000U
#define XMPU_DDR_3_BASE_ADDR      0xFD030000U
#define XMPU_DDR_4_BASE_ADDR      0xFD040000U
#define XMPU_DDR_5_BASE_ADDR      0xFD050000U
#define XMPU_FPD_BASE_ADDR        0xFD5D0000U
#define XMPU_OCM_BASE_ADDR        0xFFA70000U
#define XPPU_BASE_ADDR            0xFF980000U
#define XPPU_POISON_OFFSET_ADDR   0xFF9CFF00U

/* XMPU status register offsets */
#define XMPU_CTRL_OFFSET          0x00U
#define XMPU_ERR_STATUS1_OFFSET   0x04U
#define XMPU_ERR_STATUS2_OFFSET   0x08U
#define XMPU_POISON_OFFSET        0x0CU
#define XMPU_ISR_OFFSET           0x10U
#define XMPU_IMR_OFFSET           0x14U
#define XMPU_IEN_OFFSET           0x18U
#define XMPU_IDS_OFFSET           0x1CU
#define XMPU_LOCK_OFFSET          0x20U

/* Region offset */
#define R00_OFFSET  0x00U
#define R01_OFFSET  0x10U
#define R02_OFFSET  0x20U
#define R03_OFFSET  0x30U
#define R04_OFFSET  0x40U
#define R05_OFFSET  0x50U
#define R06_OFFSET  0x60U
#define R07_OFFSET  0x70U
#define R08_OFFSET  0x80U
#define R09_OFFSET  0x90U
#define R10_OFFSET  0xA0U
#define R11_OFFSET  0xB0U
#define R12_OFFSET  0xC0U
#define R13_OFFSET  0xD0U
#define R14_OFFSET  0xE0U
#define R15_OFFSET  0xF0U

/* Region field offset */
#define RX_START_OFFSET    0x100U
#define RX_END_OFFSET      0x104U
#define RX_MASTER_OFFSET   0x108U
#define RX_CONFIG_OFFSET   0x10CU

/* XMPU error IDs to identify each error */
#define    XMPU_REG_ACC_ERR_ON_APB            0x1U
#define    XMPU_READ_PERMISSION_VIOLATION     0x2U
#define    XMPU_WRITE_PERMISSION_VIOLATION    0x4U
#define    _VIOLATION_ERR        0x8U

/* XPPU error IDs to identify each error */
#define    XPPU_REG_ACC_ERR_ON_APB            0x1U
#define    XPPU_MID_NOT_FOUND                 0x2U
#define    XPPU_MWRITE_PERMISSON_VIOLATION    0x4U
#define    XPPU_MID_PARITY_ERROR              0x8U
#define    XPPU_MID_ACCESS_VIOLATION          0x20U
#define    XPPU_TRUSTZONE_VIOLATION           0x40U
#define    XPPU_APPER_PARITY_ERROR            0x80U

/*
Function like macro that sums up the base address and register offset
*/
#define XMPU_STATUS_REGISTER(base, offset) ((volatile uint32_t *)((base) + (offset)))

#define XMPU_CTRL_REGISTER(xmpu_base)         XMPU_STATUS_REGISTER(xmpu_base, XMPU_CTRL_OFFSET)
#define XMPU_ERR_STATUS1_REGISTER(xmpu_base)  XMPU_STATUS_REGISTER(xmpu_base, XMPU_ERR_STATUS1_OFFSET)
#define XMPU_ERR_STATUS2_REGISTER(xmpu_base)  XMPU_STATUS_REGISTER(xmpu_base, XMPU_ERR_STATUS2_OFFSET)
#define XMPU_POISON_REGISTER(xmpu_base)       XMPU_STATUS_REGISTER(xmpu_base, XMPU_POISON_OFFSET)
#define XMPU_ISR_REGISTER(xmpu_base)          XMPU_STATUS_REGISTER(xmpu_base, XMPU_ISR_OFFSET)
#define XMPU_IMR_REGISTER(xmpu_base)          XMPU_STATUS_REGISTER(xmpu_base, XMPU_IMR_OFFSET)
#define XMPU_IEN_REGISTER(xmpu_base)          XMPU_STATUS_REGISTER(xmpu_base, XMPU_IEN_OFFSET)
#define XMPU_IDS_REGISTER(xmpu_base)          XMPU_STATUS_REGISTER(xmpu_base, XMPU_IDS_OFFSET)
#define XMPU_LOCK_REGISTER(xmpu_base)         XMPU_STATUS_REGISTER(xmpu_base, XMPU_LOCK_OFFSET)

#define XMPU_REGION_REGISTER(base, region, offset) ((volatile uint32_t *)((base) + (region) + (offset)))

#define XMPU_START_REGISTER(xmpu_base, region)    XMPU_REGION_REGISTER(xmpu_base, region, RX_START_OFFSET)
#define XMPU_END_REGISTER(xmpu_base, region)      XMPU_REGION_REGISTER(xmpu_base, region, RX_END_OFFSET)
#define XMPU_MASTER_REGISTER(xmpu_base, region)   XMPU_REGION_REGISTER(xmpu_base, region, RX_MASTER_OFFSET)
#define XMPU_CONFIG_REGISTER(xmpu_base, region)   XMPU_REGION_REGISTER(xmpu_base, region, RX_CONFIG_OFFSET)


// Print the XMPU status registers
void print_xmpu_status_regs(uint32_t xmpu_base)
{
  xil_printf("CTRL:        0x%08X\n\r", *(XMPU_CTRL_REGISTER(xmpu_base)));
  xil_printf("ERR_STATUS1: 0x%08X\n\r", *(XMPU_ERR_STATUS1_REGISTER(xmpu_base)));
  xil_printf("ERR_STATUS2: 0x%08X\n\r", *(XMPU_ERR_STATUS2_REGISTER(xmpu_base)));
  xil_printf("POISON:      0x%08X\n\r", *(XMPU_POISON_REGISTER(xmpu_base)));
  xil_printf("ISR:         0x%08X\n\r", *(XMPU_ISR_REGISTER(xmpu_base)));
  xil_printf("IMR:         0x%08X\n\r", *(XMPU_IMR_REGISTER(xmpu_base)));
  xil_printf("IEN:         0x%08X\n\r", *(XMPU_IEN_REGISTER(xmpu_base)));
  xil_printf("IDS:         0x%08X\n\r", *(XMPU_IDS_REGISTER(xmpu_base)));
  xil_printf("LOCK:        0x%08X\n\r", *(XMPU_LOCK_REGISTER(xmpu_base)));
  xil_printf("\n\r");
}

// Print the XMPU region registers
void print_xmpu_region_regs(uint32_t xmpu_base, uint32_t region)
{
  xil_printf("R%02d_START:   0x%08X\n\r", region, *(XMPU_START_REGISTER(xmpu_base, region)));
  xil_printf("R%02d_END:     0x%08X\n\r", region, *(XMPU_END_REGISTER(xmpu_base, region)));
  xil_printf("R%02d_MASTER:  0x%08X\n\r", region, *(XMPU_MASTER_REGISTER(xmpu_base, region)));
  xil_printf("R%02d_CONFIG:  0x%08X\n\r", region, *(XMPU_CONFIG_REGISTER(xmpu_base, region)));
  xil_printf("\n\r");
}

// Configure XPMU0 register
void set_xmpu0_rpu(){  
  /* START_REG
  [31:28] -> Reserved
  [27:8] -> Correspond to address bit [39:20]
  [7:0] -> Reserved
  */
  *(XMPU_START_REGISTER(XMPU_DDR_0_BASE_ADDR, R00_OFFSET)) =    0x0003ED00; // Start address 

  /* END_REG
  [31:28] -> Reserved
  [27:8] -> Correspond to address bit [39:20]
  [7:0] -> Reserved
  */
  *(XMPU_END_REGISTER(XMPU_DDR_0_BASE_ADDR, R00_OFFSET)) =      0x0003EE00; // End address

  /* MASTER_REG
  [31:26] -> Reserved
  [25:16] -> Mask
  [15:10] -> Reserved
  [9:0] -> Master ID
  */
  *(XMPU_MASTER_REGISTER(XMPU_DDR_0_BASE_ADDR, R00_OFFSET)) =   0x03E00000; // Reserved 0000 00 | Mask: 11 1110 0000 | Reserved: 0000 00 |  MASTER ID: 0000 00 0000

  /* CONFIG_REG
  [31:5] -> Reserved
  [4] -> Select Security Level of region 0: Secure, 1: Non Secure
  [3] -> Allow write
  [2] -> Allow read
  [1] -> Enable XMPU region
  [0] -> Reserved
  */  
  *(XMPU_CONFIG_REGISTER(XMPU_DDR_0_BASE_ADDR, R00_OFFSET)) =   0x00000007; // Non Secure : 0, Allow write: 1, Allow read: 1, Enable XMPU region: 1.

  /* CTRL_REG
  [31:4] -> Reserved
  [3] -> Align 1MB: 1, Align 4KB: 0
  [2] -> Attr poison: 1, Attr no poison: 0
  [1] -> Default Write Permissions: 1,
  [0] -> Default Read Permissions: 1
  */
  *(XMPU_CTRL_REGISTER(XMPU_DDR_0_BASE_ADDR)) =         0x00000008; // Align 1MB: 1, Attr poison: 0, Write Default: 0, Read Default: 0
  
  /* POISON_REG
  [31:21] -> Reserved
  [20] -> Poison attribute
  [19:0] -> Reserved. The DDR memory controller expects poisoning by attribute. 
  */
  *(XMPU_POISON_REGISTER(XMPU_DDR_0_BASE_ADDR)) =       0x00100000; // Enable address poisoning
}

// Configure XPMU1 register
void set_xmpu1_apu(){ 
  *(XMPU_START_REGISTER(XMPU_DDR_1_BASE_ADDR, R00_OFFSET)) =    0x0003ED00; // Start address 
  *(XMPU_END_REGISTER(XMPU_DDR_1_BASE_ADDR, R00_OFFSET)) =      0x0003EE00; // End address
  *(XMPU_MASTER_REGISTER(XMPU_DDR_1_BASE_ADDR, R00_OFFSET)) =   0x03C00080; // Reserved 0000 00 | Mask: 11 1100 0000 | Reserved: 0000 00 |  MASTER ID: 0010 00 0000
  *(XMPU_CONFIG_REGISTER(XMPU_DDR_1_BASE_ADDR, R00_OFFSET)) =   0x00000001; // Non Secure : 0, Allow write: 0, Allow read: 0, Enable XMPU region: 1.

  *(XMPU_CTRL_REGISTER(XMPU_DDR_1_BASE_ADDR)) =                 0x0000000B; // Align 1MB: 1, Attr poison: 0, Write Default: 1, Read Default: 1
  *(XMPU_POISON_REGISTER(XMPU_DDR_1_BASE_ADDR)) =               0x00100000; // Enable address poisoning
}

// Configure XPMU2 register
void set_xmpu2_apu(){
  *(XMPU_START_REGISTER(XMPU_DDR_2_BASE_ADDR, R00_OFFSET)) =    0x0003ED00; // Start address 
  *(XMPU_END_REGISTER(XMPU_DDR_2_BASE_ADDR, R00_OFFSET)) =      0x0003EE00; // End address
  *(XMPU_MASTER_REGISTER(XMPU_DDR_2_BASE_ADDR, R00_OFFSET)) =   0x03C00080; // Reserved 0000 00 | Mask: 11 1100 0000 | Reserved: 0000 00 |  MASTER ID: 0010 00 0000  
  *(XMPU_CONFIG_REGISTER(XMPU_DDR_2_BASE_ADDR, R00_OFFSET)) =   0x00000001; // Non Secure : 0, Allow write: 0, Allow read: 0, Enable XMPU region: 1.

  *(XMPU_CTRL_REGISTER(XMPU_DDR_2_BASE_ADDR)) =                 0x0000000B; // Align 1MB: 1, Attr poison: 0, Write Default: 1, Read Default: 1
  *(XMPU_POISON_REGISTER(XMPU_DDR_2_BASE_ADDR)) =               0x00100000; // Enable address poisoning
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
  // Program XMPU1 and XMPU2 registers to protect RPU from APU accesses
  set_xmpu1_apu();
  set_xmpu2_apu();

  xil_printf("XMPU0 registers:\n\r");
  print_xmpu_status_regs(XMPU_DDR_0_BASE_ADDR);
  print_xmpu_region_regs(XMPU_DDR_0_BASE_ADDR, R00_OFFSET);

  xil_printf("XMPU1 registers:\n\r");
  print_xmpu_status_regs(XMPU_DDR_1_BASE_ADDR);
  print_xmpu_region_regs(XMPU_DDR_1_BASE_ADDR, R00_OFFSET);

  xil_printf("XMPU2 registers:\n\r");
  print_xmpu_status_regs(XMPU_DDR_2_BASE_ADDR);
  print_xmpu_region_regs(XMPU_DDR_2_BASE_ADDR, R00_OFFSET);

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
