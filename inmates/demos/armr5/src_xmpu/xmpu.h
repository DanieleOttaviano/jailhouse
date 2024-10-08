/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor XMPU demo RPU
 *
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef XMPU_H
#define XMPU_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <xbasic_types.h>

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed long long s64;
typedef unsigned long long u64;


#define NR_XMPU_REGIONS 16
#define NR_XMPU_DDR     6

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

/* XMPU region field offset */
#define RX_START_OFFSET    0x100U
#define RX_END_OFFSET      0x104U
#define RX_MASTER_OFFSET   0x108U
#define RX_CONFIG_OFFSET   0x10CU

/* Region offset */
#define XMPU_REGION_OFFSET        0x10U
#define R00_OFFSET  0x00U
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
  // Jailhouse specific
  u16 id;
  bool used;
}xmpu_region_config;

typedef struct xmpu_channel{
  xmpu_status_config status;
  xmpu_region_config region[NR_XMPU_REGIONS];
}xmpu_channel;

// Set XMPU registers
void set_xmpu_status(u32 xmpu_base, xmpu_status_config *config);
void set_xmpu_region(u32 xmpu_base, u32 region, xmpu_region_config *config);
void set_xmpu_region_default(u32 xmpu_base, u32 region_offset);
void set_xmpu_status_default(u32 xmpu_base);
void set_xmpu_default(u32 xmpu_base);

//Debug Print
void print_xmpu_status_regs(u32 xmpu_base);
void print_xmpu_region_regs(u32 xmpu_base, u32 region);
void print_xmpu(u32 xmpu_base);

#endif