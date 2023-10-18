
#ifndef XMPU_H
#define XMPU_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define NR_XMPU_REGIONS 16
#define NR_XMPU_DDR     6

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

/*
Function like macro that sums up the base address and register offset
*/
#define XMPU_STATUS_REGISTER(base, offset)          ((volatile uint32_t *)((base) + (offset)))

#define XMPU_CTRL_REGISTER(xmpu_base)               XMPU_STATUS_REGISTER(xmpu_base, XMPU_CTRL_OFFSET)
#define XMPU_ERR_STATUS1_REGISTER(xmpu_base)        XMPU_STATUS_REGISTER(xmpu_base, XMPU_ERR_STATUS1_OFFSET)
#define XMPU_ERR_STATUS2_REGISTER(xmpu_base)        XMPU_STATUS_REGISTER(xmpu_base, XMPU_ERR_STATUS2_OFFSET)
#define XMPU_POISON_REGISTER(xmpu_base)             XMPU_STATUS_REGISTER(xmpu_base, XMPU_POISON_OFFSET)
#define XMPU_ISR_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_ISR_OFFSET)
#define XMPU_IMR_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_IMR_OFFSET)
#define XMPU_IEN_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_IEN_OFFSET)
#define XMPU_IDS_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_IDS_OFFSET)
#define XMPU_LOCK_REGISTER(xmpu_base)               XMPU_STATUS_REGISTER(xmpu_base, XMPU_LOCK_OFFSET)

#define XMPU_REGION_REGISTER(base, region, offset)  ((volatile uint32_t *)((base) + (region) + (offset)))

#define XMPU_START_REGISTER(xmpu_base, region)      XMPU_REGION_REGISTER(xmpu_base, region, RX_START_OFFSET)
#define XMPU_END_REGISTER(xmpu_base, region)        XMPU_REGION_REGISTER(xmpu_base, region, RX_END_OFFSET)
#define XMPU_MASTER_REGISTER(xmpu_base, region)     XMPU_REGION_REGISTER(xmpu_base, region, RX_MASTER_OFFSET)
#define XMPU_CONFIG_REGISTER(xmpu_base, region)     XMPU_REGION_REGISTER(xmpu_base, region, RX_CONFIG_OFFSET)

typedef struct xmpu_status_config{
  bool poison;
  bool align;
  bool def_wr_allowed;
  bool def_rd_allowed;
  bool lock;
}xmpu_status_config;

typedef struct xmpu_region_config{
  uint64_t addr_start;
  uint64_t addr_end;
  uint16_t master_id;
  uint16_t master_mask;
  bool ns_checktype;
  bool region_ns;
  bool wrallowed;
  bool rdallowed;
  bool enable;
}xmpu_region_config;


// Set XMPU registers
void set_xmpu_status(uint32_t xmpu_base, xmpu_status_config *config);
void set_xmpu_region(uint32_t xmpu_base, uint32_t region, xmpu_region_config *config);

//Debug Print
void print_xmpu_status_regs(uint32_t xmpu_base);
void print_xmpu_region_regs(uint32_t xmpu_base, uint32_t region);
void print_xmpu_regs(uint32_t xmpu_base);

#endif