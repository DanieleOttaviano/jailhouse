/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Zynqmp XMPUs management for asymmetric cores spatial protection
 *
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_ASM_XMPU_H
#define _JAILHOUSE_ASM_XMPU_H

#include <jailhouse/types.h>
#include <jailhouse/string.h>

#if defined(CONFIG_XMPU_ACTIVE) && defined(CONFIG_MACH_ZYNQMP_ZCU102)

/* XMPU configiguration register addresses */
#define XMPU_DDR_OFFSET           0x00010000U
#define XMPU_DDR_BASE_ADDR        0xFD000000U
#define XMPU_DDR_0_BASE_ADDR      XMPU_DDR_BASE_ADDR
#define XMPU_DDR_1_BASE_ADDR      (XMPU_DDR_0_BASE_ADDR + XMPU_DDR_OFFSET)//0xFD010000U
#define XMPU_DDR_2_BASE_ADDR      (XMPU_DDR_1_BASE_ADDR + XMPU_DDR_OFFSET)//0xFD020000U
#define XMPU_DDR_3_BASE_ADDR      (XMPU_DDR_2_BASE_ADDR + XMPU_DDR_OFFSET)//0xFD030000U
#define XMPU_DDR_4_BASE_ADDR      (XMPU_DDR_3_BASE_ADDR + XMPU_DDR_OFFSET)//0xFD040000U
#define XMPU_DDR_5_BASE_ADDR      (XMPU_DDR_4_BASE_ADDR + XMPU_DDR_OFFSET)//0xFD050000U
#define XMPU_FPD_BASE_ADDR        0xFD5D0000U
#define XMPU_OCM_BASE_ADDR        0xFFA70000U
#define NR_XMPU_DDR     6

/* XPPU configiguration register addresses */
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

/* XMPU Default status register values*/
#define XMPU_DDR_DEFAULT_CTRL    0x0000000bU
#define XMPU_DDR_DEFAULT_POISON  0x00000000U
#define XMPU_FPD_DEFAULT_CTRL    0x00000007U
#define XMPU_FPD_DEFAULT_POISON  0x000fd4f0U
#define XMPU_OCM_DEFAULT_CTRL    0x00000003U
#define XMPU_OCM_DEFAULT_POISON  0x00000000U

/* XMPU region field offset */
#define RX_START_OFFSET    0x100U
#define RX_END_OFFSET      0x104U
#define RX_MASTER_OFFSET   0x108U
#define RX_CONFIG_OFFSET   0x10CU

/* XMPU Default region field values*/
#define XMPU_DEFAULT_START  0x00000000U
#define XMPU_DEFAULT_END    0x00000000U
#define XMPU_DEFAULT_MASTER 0x00000000U
#define XMPU_DEFAULT_CONFIG 0x00000008U

/* Region offset */
#define XMPU_REGION_OFFSET        0x10U
#define R00_OFFSET                0x00U
#define R01_OFFSET  (R00_OFFSET + XMPU_REGION_OFFSET)//0x10U       
#define R02_OFFSET  (R01_OFFSET + XMPU_REGION_OFFSET)//0x20U
#define R03_OFFSET  (R02_OFFSET + XMPU_REGION_OFFSET)//0x30U
#define R04_OFFSET  (R03_OFFSET + XMPU_REGION_OFFSET)//0x40U
#define R05_OFFSET  (R04_OFFSET + XMPU_REGION_OFFSET)//0x50U
#define R06_OFFSET  (R05_OFFSET + XMPU_REGION_OFFSET)//0x60U
#define R07_OFFSET  (R06_OFFSET + XMPU_REGION_OFFSET)//0x70U
#define R08_OFFSET  (R07_OFFSET + XMPU_REGION_OFFSET)//0x80U
#define R09_OFFSET  (R08_OFFSET + XMPU_REGION_OFFSET)//0x90U
#define R10_OFFSET  (R09_OFFSET + XMPU_REGION_OFFSET)//0xA0U
#define R11_OFFSET  (R10_OFFSET + XMPU_REGION_OFFSET)//0xB0U
#define R12_OFFSET  (R11_OFFSET + XMPU_REGION_OFFSET)//0xC0U
#define R13_OFFSET  (R12_OFFSET + XMPU_REGION_OFFSET)//0xD0U
#define R14_OFFSET  (R13_OFFSET + XMPU_REGION_OFFSET)//0xE0U
#define R15_OFFSET  (R14_OFFSET + XMPU_REGION_OFFSET)//0xF0U
#define NR_XMPU_REGIONS 16

/* Master Devices */
// RPU0 ( 0000, 00, AXI ID[3:0] )
#define RPU0      0
#define RPU0_ID   0x0000
#define RPU0_MASK 0x03F0
// RPU1 ( 0000, 01, AXI ID[3:0] )
#define RPU1      1
#define RPU1_ID   0x0010
#define RPU1_MASK 0x03F0
// SATA, GPU, DAP AXI CoreSight, PCIe ( 0011, 0x, xxxx )
#define GROUP0      2
#define GROUP0_ID   0x00C0 
#define GROUP0_MASK 0x03E0
// PMU processor, CSU processor, CSU-DMA, USB0, USB1, DAP APB control,
// LPD DMA, SD0, SD1, NAND, QSPI, GEM0, GEM1, GEM2, GEM3 ( 0001, xx, xxxx )
#define GROUP1      3
#define GROUP1_ID   0x0040
#define GROUP1_MASK 0x03C0
// CCI-400, SMMU TCU ( 0000, 00, 0000 )
#define GROUP2      4
#define GROUP2_ID   0x0000
#define GROUP2_MASK 0x03FF
// APU ( 0010, AXI ID [5:0] )
#define APU         5
#define APU_ID      0x0080
#define APU_MASK    0x03C0
// DysplayPort ( 0011, 10, 0xxx DMA{0:5} )
#define DISPLAYPORT      6
#define DISPLAYPORT_ID   0x00E0
#define DISPLAYPORT_MASK 0x03F8
// FPD DMA ( 0011, 10, 1xxx CH{0:7} )
#define FPD_DMA      7
#define FPD_DMA_ID   0x00E8
#define FPD_DMA_MASK 0x03F8
// TBU 3 ( 0010 00XX XXXX XXXX )
#define TBU3        8
#define TBU3_ID     0x2000
#define TBU3_MASK   0xFC00
// TBU 4 ( 0100 00XX XXXX XXXX )
#define TBU4        9
#define TBU4_ID     0x4000
#define TBU4_MASK   0xFC00
// TBU 5 ( 1000 00XX XXXX XXXX )
#define TBU5        10
#define TBU5_ID     0x8000
#define TBU5_MASK   0xFC00

typedef struct xmpu_status_config{
  bool poison;
  bool align;
  bool def_wr_allowed;
  bool def_rd_allowed;
  bool lock;
}xmpu_status_config;

typedef struct xmpu_region_config{
  u64 addr_start;
  u64 addr_end;
  u64 master_id;
  u64 master_mask;
  bool ns_checktype;
  bool region_ns;
  bool wrallowed;
  bool rdallowed;
  bool enable;
  // Jailhouse-Omnivisor specific configurations
  u16 id;
  bool used;
}xmpu_region_config;

/* XMPU device type enum for clarity */
typedef enum {
    XMPU_DDR,
    XMPU_FPD,
    XMPU_OCM,
    XMPU_UNKNOWN
}xmpu_dev_type;

typedef struct xmpu_dev{
  u32 base_addr; 
  xmpu_status_config status;
  xmpu_region_config region[NR_XMPU_REGIONS];
  xmpu_dev_type type;
}xmpu_dev;

typedef struct master_device{
  u64 id;
  u64 mask;
  u8  xmpu_dev_mask;
}master_device;

//Debug Print
void print_xmpu_status_regs(u32 xmpu_base);
void print_xmpu_region_regs(u32 xmpu_base, u32 region);
void print_xmpu(u32 xmpu_base);

#endif /* CONFIG_XMPU_ACTIVE && CONFIG_MACH_ZYNQMP_ZCU102 */ 

#endif /* _JAILHOUSE_ASM_XMPU_H  */
