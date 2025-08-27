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

#include <jailhouse/printk.h>
#include <jailhouse/assert.h>
#include <jailhouse/unit.h>
#include <jailhouse/control.h>
#include <asm/sysregs.h>
#include <asm/xmpu-board.h>
#include <asm/xmpu.h>

#if defined(CONFIG_XMPU_ACTIVE) && defined(CONFIG_MACH_ZYNQMP_ZCU102)

#ifdef CONFIG_DEBUG
#define xmpu_print(fmt, ...)			\
	printk("[XMPU] " fmt, ##__VA_ARGS__)
#else
#define xmpu_print(fmt, ...) do { } while (0)
#endif

/* DDR regions */
#define DDR_LOW_START   0x00000000U
#define DDR_LOW_END     0x7FFFFFFFU
#define DDR_HIGH_START  0x800000000U
#define DDR_HIGH_END    0x87FFFFFFFU

/* FPD regions */
#define FPD_START_ADDR      0xF9000000 
#define FPD_END_ADDR        0xFFFFFFFF 
#define FPD_PCIE_LOW_START  0xE0000000
#define FPD_PCIE_LOW_END    0xEFFFFFFF
#define FPD_PCIE_HIGH_START 0x60000000
#define FPD_PCIE_HIGH_END   0x9FFFFFFF

/* OCM regions */
#define OCM_START_ADDR  0xFFFC0000
#define OCM_END_ADDR    0xFFFFFFFF

/* XMPU Alignment */
#define XMPU_ALIGN_1MB  1
#define XMPU_ALIGN_4KB  0

/*
Function like macro that sums up the base address and register offset
*/
#define XMPU_STATUS_REGISTER(base, offset)          ((volatile u32 *)((u64)(base) + (offset)))

#define XMPU_CTRL_REGISTER(xmpu_base)               XMPU_STATUS_REGISTER(xmpu_base, XMPU_CTRL_OFFSET)
#define XMPU_ERR_STATUS1_REGISTER(xmpu_base)        XMPU_STATUS_REGISTER(xmpu_base, XMPU_ERR_STATUS1_OFFSET)
#define XMPU_ERR_STATUS2_REGISTER(xmpu_base)        XMPU_STATUS_REGISTER(xmpu_base, XMPU_ERR_STATUS2_OFFSET)
#define XMPU_POISON_REGISTER(xmpu_base)             XMPU_STATUS_REGISTER(xmpu_base, XMPU_POISON_OFFSET)
#define XMPU_ISR_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_ISR_OFFSET)
#define XMPU_IMR_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_IMR_OFFSET)
#define XMPU_IEN_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_IEN_OFFSET)
#define XMPU_IDS_REGISTER(xmpu_base)                XMPU_STATUS_REGISTER(xmpu_base, XMPU_IDS_OFFSET)
#define XMPU_LOCK_REGISTER(xmpu_base)               XMPU_STATUS_REGISTER(xmpu_base, XMPU_LOCK_OFFSET)

#define XMPU_REGION_REGISTER(base, region, offset)  ((volatile u32 *)((u64)(base) + (region) + (offset)))

#define XMPU_START_REGISTER(xmpu_base, region)      XMPU_REGION_REGISTER(xmpu_base, region, RX_START_OFFSET)
#define XMPU_END_REGISTER(xmpu_base, region)        XMPU_REGION_REGISTER(xmpu_base, region, RX_END_OFFSET)
#define XMPU_MASTER_REGISTER(xmpu_base, region)     XMPU_REGION_REGISTER(xmpu_base, region, RX_MASTER_OFFSET)
#define XMPU_CONFIG_REGISTER(xmpu_base, region)     XMPU_REGION_REGISTER(xmpu_base, region, RX_CONFIG_OFFSET)

/* XMPU board specific devices */
static xmpu_dev xmpu_device[] = {
    { XMPU_DDR_0_BASE_ADDR, {0}, {{0}}, XMPU_DDR },
    { XMPU_DDR_1_BASE_ADDR, {0}, {{0}}, XMPU_DDR },
    { XMPU_DDR_2_BASE_ADDR, {0}, {{0}}, XMPU_DDR },
    { XMPU_DDR_3_BASE_ADDR, {0}, {{0}}, XMPU_DDR },
    { XMPU_DDR_4_BASE_ADDR, {0}, {{0}}, XMPU_DDR },
    { XMPU_DDR_5_BASE_ADDR, {0}, {{0}}, XMPU_DDR },
    { XMPU_FPD_BASE_ADDR,   {0}, {{0}}, XMPU_FPD },
    { XMPU_OCM_BASE_ADDR,   {0}, {{0}}, XMPU_OCM }
};
#define NR_XMPU (sizeof(xmpu_device)/sizeof(xmpu_device[0]))

/* Static master device list */
static master_device master_device_list[] = {
    { RPU0_ID,        RPU0_MASK,        (u8)0b10000001 },
    { RPU1_ID,        RPU1_MASK,        (u8)0b10000001 },
    { GROUP0_ID,      GROUP0_MASK,      (u8)0b00000110 },
    { GROUP1_ID,      GROUP1_MASK,      (u8)0b00000110 },
    { GROUP2_ID,      GROUP2_MASK,      (u8)0b00000110 },
    { APU_ID,         APU_MASK,         (u8)0b11000110 },
    { DISPLAYPORT_ID, DISPLAYPORT_MASK, (u8)0b00001000 },
    { FPD_DMA_ID,     FPD_DMA_MASK,     (u8)0b00001000 },
    { TBU3_ID,        TBU3_MASK,        (u8)0b00001000 },
    { TBU4_ID,        TBU4_MASK,        (u8)0b00010000 },
    { TBU5_ID,        TBU5_MASK,        (u8)0b00100000 }
};
#define NR_MASTER_DEVICES (sizeof(master_device_list)/sizeof(master_device_list[0]))

#if defined(CONFIG_XMPU_DEBUG)
//Print the XMPU status registers
void print_xmpu_status_regs(u32 xmpu_base){
  xmpu_print("CTRL:        0x%08x\n\r", xmpu_read32((void *)(XMPU_CTRL_REGISTER(xmpu_base))));
  xmpu_print("ERR_STATUS1: 0x%08x\n\r", xmpu_read32((void *)(XMPU_ERR_STATUS1_REGISTER(xmpu_base))));
  xmpu_print("ERR_STATUS2: 0x%08x\n\r", xmpu_read32((void *)(XMPU_ERR_STATUS2_REGISTER(xmpu_base))));
  xmpu_print("POISON:      0x%08x\n\r", xmpu_read32((void *)(XMPU_POISON_REGISTER(xmpu_base))));
  xmpu_print("ISR:         0x%08x\n\r", xmpu_read32((void *)(XMPU_ISR_REGISTER(xmpu_base))));
  xmpu_print("IMR:         0x%08x\n\r", xmpu_read32((void *)(XMPU_IMR_REGISTER(xmpu_base))));
  xmpu_print("IEN:         0x%08x\n\r", xmpu_read32((void *)(XMPU_IEN_REGISTER(xmpu_base))));
  xmpu_print("IDS:         0x%08x\n\r", xmpu_read32((void *)(XMPU_IDS_REGISTER(xmpu_base))));
  xmpu_print("LOCK:        0x%08x\n\r", xmpu_read32((void *)(XMPU_LOCK_REGISTER(xmpu_base))));
  xmpu_print("\n\r");
}

// Print the XMPU region registers
void print_xmpu_region_regs(u32 xmpu_base, u32 region_offset){
  xmpu_print("START:   0x%08x\n\r", xmpu_read32((void *)(XMPU_START_REGISTER(xmpu_base, region_offset))));
  xmpu_print("END:     0x%08x\n\r", xmpu_read32((void *)(XMPU_END_REGISTER(xmpu_base, region_offset))));
  xmpu_print("MASTER:  0x%08x\n\r", xmpu_read32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region_offset))));
  xmpu_print("CONFIG:  0x%08x\n\r", xmpu_read32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region_offset))));
  xmpu_print("\n\r");
}

// Print all the XMPU registers
void print_xmpu(u32 xmpu_base){
  int i=0;
  print_xmpu_status_regs(xmpu_base);
  for (i = 0; i < NR_XMPU_REGIONS; i++) {
    xmpu_print("Region %d:\n\r", i);
    print_xmpu_region_regs(xmpu_base, R00_OFFSET + (i * XMPU_REGION_OFFSET));
  }
}
#endif // CONFIG_XMPU_DEBUG

// Set the XMPU status registers of the xmpu_base
static void set_xmpu_status(xmpu_dev * xmpu){
  u32 xmpu_ctrl_register;
  u32 xmpu_poison_register;
  u32 xmpu_lock_register;
  bool poison;

  // The FPD XMPU uses negative logic on the poison bit
  poison = (xmpu->type == XMPU_FPD) ? !(xmpu->status.poison) : xmpu->status.poison;

  if(xmpu->type == XMPU_FPD){
    xmpu_ctrl_register	  = (0x00000000 | (xmpu->status.align << 3) | (1<<2) | (xmpu->status.def_wr_allowed << 1) | (xmpu->status.def_rd_allowed));
  }
  else{ // DDR and OCM
    xmpu_ctrl_register	  = (0x00000000 | (xmpu->status.align << 3) | (0<<2) | (xmpu->status.def_wr_allowed << 1) | (xmpu->status.def_rd_allowed));
  }

  xmpu_poison_register	=	(0x00000000 | (poison << 20));
  xmpu_lock_register	  =	(0x00000000 | (xmpu->status.lock));

  xmpu_write32((void *)(XMPU_CTRL_REGISTER(xmpu->base_addr)), xmpu_ctrl_register);
  xmpu_write32((void *)(XMPU_POISON_REGISTER(xmpu->base_addr)), xmpu_poison_register);
  xmpu_write32((void *)(XMPU_LOCK_REGISTER(xmpu->base_addr)), xmpu_lock_register);
}

// Set the XMPU region to the value of the input xmpu_dev
static void set_xmpu_region(xmpu_dev *xmpu, u32 reg_num){
  u32 tmp_addr;
  u32 xmpu_start_register;
  u32 xmpu_end_register;
  u32 xmpu_master_register;
  u32 xmpu_config_register;
  u32 region_offset = reg_num * XMPU_REGION_OFFSET;

#if defined(CONFIG_XMPU_DEBUG)
  xmpu_print("Setting access to memory region: (0x%08llx - 0x%08llx)\n\r", xmpu->region[reg_num].addr_start, xmpu->region[reg_num].addr_end);
#endif // CONFIG_XMPU_DEBUG

  switch (xmpu->type)
  {
    case XMPU_DDR:
      tmp_addr = xmpu->region[reg_num].addr_start >> 20;
      xmpu_start_register = tmp_addr << 8;
      tmp_addr = xmpu->region[reg_num].addr_end >> 20;
      xmpu_end_register = tmp_addr << 8;
      break;
    case XMPU_FPD:
      tmp_addr = xmpu->region[reg_num].addr_start >> 12;
      xmpu_start_register = tmp_addr; 
      tmp_addr = xmpu->region[reg_num].addr_end >> 12;
      xmpu_end_register = tmp_addr;	
      break;
    case XMPU_OCM:
      tmp_addr = xmpu->region[reg_num].addr_start;
      xmpu_start_register = tmp_addr; 
      tmp_addr = xmpu->region[reg_num].addr_end;
      xmpu_end_register = tmp_addr;	
      break;
    default:
      return; // Invalid XMPU type
  }
  xmpu_master_register = (0x00000000 | (xmpu->region[reg_num].master_mask << 16) | (xmpu->region[reg_num].master_id));
  xmpu_config_register = (0x00000000 | (xmpu->region[reg_num].ns_checktype << 4) | (xmpu->region[reg_num].region_ns << 3) | (xmpu->region[reg_num].wrallowed << 2) | (xmpu->region[reg_num].rdallowed << 1) | (xmpu->region[reg_num].enable));

  xmpu_write32((void *)(XMPU_START_REGISTER(xmpu->base_addr, region_offset)), xmpu_start_register);
  xmpu_write32((void *)(XMPU_END_REGISTER(xmpu->base_addr, region_offset)), xmpu_end_register);
  xmpu_write32((void *)(XMPU_MASTER_REGISTER(xmpu->base_addr, region_offset)), xmpu_master_register);
  xmpu_write32((void *)(XMPU_CONFIG_REGISTER(xmpu->base_addr, region_offset)), xmpu_config_register);
}

// Set the XMPU region registers to default values
static void set_xmpu_region_default(xmpu_dev *xmpu, u32 reg_num){
  u32 xmpu_start_register   = XMPU_DEFAULT_START;
  u32 xmpu_end_register     = XMPU_DEFAULT_END;
  u32 xmpu_master_register  = XMPU_DEFAULT_MASTER;
  u32 xmpu_config_register  = XMPU_DEFAULT_CONFIG;
  u32 region_offset = reg_num * XMPU_REGION_OFFSET;

  xmpu_write32((void *)(XMPU_START_REGISTER(xmpu->base_addr, region_offset)), xmpu_start_register);
  xmpu_write32((void *)(XMPU_END_REGISTER(xmpu->base_addr, region_offset)), xmpu_end_register);
  xmpu_write32((void *)(XMPU_MASTER_REGISTER(xmpu->base_addr, region_offset)), xmpu_master_register);
  xmpu_write32((void *)(XMPU_CONFIG_REGISTER(xmpu->base_addr, region_offset)), xmpu_config_register);
}

// Set the XMPU status registers to default values
static void set_xmpu_status_default(xmpu_dev *xmpu){
  u32 xmpu_ctrl_register    = 0;
  u32 xmpu_poison_register  = 0;
  u32 xmpu_lock_register    = 0;

  switch (xmpu->type) {
    case XMPU_DDR:
      xmpu_ctrl_register    = XMPU_DDR_DEFAULT_CTRL;
      xmpu_poison_register  = XMPU_DDR_DEFAULT_POISON;
      break;
    case XMPU_FPD:
      xmpu_ctrl_register    = XMPU_FPD_DEFAULT_CTRL;
      xmpu_poison_register  = XMPU_FPD_DEFAULT_POISON;
      break;
    case XMPU_OCM:
      xmpu_ctrl_register    = XMPU_OCM_DEFAULT_CTRL;
      xmpu_poison_register  = XMPU_OCM_DEFAULT_POISON;
      break;
    default:
      xmpu_print("ERROR: XMPU base address not valid\n\r");
      break;
  }
  
  xmpu_write32((void *)(XMPU_CTRL_REGISTER(xmpu->base_addr)), xmpu_ctrl_register);
  xmpu_write32((void *)(XMPU_POISON_REGISTER(xmpu->base_addr)), xmpu_poison_register);
  xmpu_write32((void *)(XMPU_LOCK_REGISTER(xmpu->base_addr)), xmpu_lock_register); 
}

// Set all the XMPU registers of the xmpu to default values
static void set_xmpu_default(xmpu_dev * xmpu){
  u8 i = 0;

  set_xmpu_status_default(xmpu);

  for(i=0 ; i<NR_XMPU_REGIONS; i++){
    set_xmpu_region_default(xmpu, i);
  }

}

// Setup status to poison (DDR->1Mb alignment, Others->4Kb alignment)
static void poison_xmpu_status(xmpu_dev *xmpu){
  xmpu->status.poison =        1;
  xmpu->status.align = (xmpu->type == XMPU_DDR) ? XMPU_ALIGN_1MB : XMPU_ALIGN_4KB;
  xmpu->status.def_wr_allowed =0;
  xmpu->status.def_rd_allowed =0;
  xmpu->status.lock =          0;
  set_xmpu_status(xmpu);
}

// Setup region configuration
static void enable_region_configuration(xmpu_region_config *config, 
                              u64 addr_start, u64 addr_end, 
                              u64 master_id, u64 master_mask,
                              bool wrallowed, bool rdallowed,
                              u16 id){
  bool region_ns = false;

  // the root cell has dedicated configuration (full access)
  if(id == root_cell.config->id){
    addr_start = 0x0000000000;
    addr_end   = 0xFFFFFFFFFF;
    region_ns = true;
    wrallowed = true;
    rdallowed = true;
  }
  config->addr_start =   addr_start;
  config->addr_end =     addr_end;
  config->master_id =    master_id;
  config->master_mask =  master_mask;
  config->ns_checktype = false;
  config->region_ns =    region_ns;
  config->wrallowed =    wrallowed;
  config->rdallowed =    rdallowed;
  config->enable =       true;
  config->id =           id;
  config->used =         true;
}

// Helper function to clean xmpu permissions of a specific device for all the regions used by a cell
static void clean_cell_permissions(struct master_device *dev, u8 cell_id) { 
  u8 xmpu_dev_n = 0;
  u8 valid_reg_n = 0; 
  u8 i = 0;
  xmpu_dev *xmpu;
  u8 mask = dev->xmpu_dev_mask;

#if defined(CONFIG_XMPU_DEBUG)
  xmpu_print("\n\rCleaning protection for cell %d\n\r", cell_id);
  xmpu_print("Master ID: 0x%04llx, Mask: 0x%04llx\n\r", dev->id, dev->mask);
#endif // CONFIG_XMPU_DEBUG

  for (xmpu_dev_n = 0; mask; xmpu_dev_n++, mask >>= 1) {
    if (!(mask & 0x1))
      continue;
    xmpu = &xmpu_device[xmpu_dev_n];

#if defined(CONFIG_XMPU_DEBUG)
    xmpu_print("XMPU device channel %d (addr: 0x%08x)\n\r", xmpu_dev_n, xmpu->base_addr);
#endif // CONFIG_XMPU_DEBUG

    for(i = 0; i < NR_XMPU_REGIONS; i++) {
      if(xmpu->region[i].id == cell_id && xmpu->region[i].used == 1 && xmpu->region[i].master_id == dev->id && xmpu->region[i].master_mask == dev->mask) {
        valid_reg_n = i;

#if defined(CONFIG_XMPU_DEBUG)
        xmpu_print("Cleaning XMPU region %d\n\r", valid_reg_n);
#endif // CONFIG_XMPU_DEBUG

        set_xmpu_region_default(xmpu, valid_reg_n);
        xmpu->region[valid_reg_n].id = 0;
        xmpu->region[valid_reg_n].used = 0;
      }
    }
  }
}

static bool mem_in_xmpu_addr_range(xmpu_dev *xmpu, const struct jailhouse_memory *mem) {
  bool in_ddr_low_range = mem->virt_start <= DDR_LOW_END - mem->size;
  bool in_ddr_high_range = (mem->virt_start >= DDR_HIGH_START) && (mem->virt_start + mem->size - 1 <= DDR_HIGH_END);
  bool in_fpd_range = (mem->virt_start >= FPD_START_ADDR) && (mem->virt_start + mem->size - 1 <= FPD_END_ADDR);
  bool in_fpd_pcie_low = (mem->virt_start >= FPD_PCIE_LOW_START) && (mem->virt_start + mem->size - 1 <= FPD_PCIE_LOW_END);
  bool in_fpd_pcie_high = (mem->virt_start >= FPD_PCIE_HIGH_START) && (mem->virt_start + mem->size - 1 <= FPD_PCIE_HIGH_END);
  bool in_ocm_range = (mem->virt_start >= OCM_START_ADDR) && (mem->virt_start + mem->size - 1 <= OCM_END_ADDR);

  if ((xmpu->type == XMPU_DDR && (in_ddr_low_range || in_ddr_high_range)) ||
      (xmpu->type == XMPU_FPD && (in_fpd_range || in_fpd_pcie_low || in_fpd_pcie_high)) ||
      (xmpu->type == XMPU_OCM && in_ocm_range))
    return true;

  return false;
}

// Helper function to set xmpu permissions to cell
static int set_cell_permissions(struct master_device *dev, struct cell *cell) {
  u8 xmpu_dev_n = 0;
  u8 valid_reg_n = 0;
  u8 i = 0;
  u64 addr_start, addr_end;
  bool wrallowed = false;
  bool rdallowed = false;
  xmpu_dev * xmpu;
  u8 mask = dev->xmpu_dev_mask;
  const struct jailhouse_memory *mem;
  u32 n;

#if defined(CONFIG_XMPU_DEBUG)
  xmpu_print("Setting protection for cell %d\n\r", cell->config->id);
  xmpu_print("Master ID: 0x%04llx, Mask: 0x%04llx\n\r", dev->id, dev->mask);
#endif // CONFIG_XMPU_DEBUG

  /* For each XMPU used by the master device 
   * check if the memory regions assigned to the cells are in the right address space 
   * and program the first XMPU register accordingly 
   */
  for (xmpu_dev_n = 0; mask; xmpu_dev_n++, mask >>= 1) {
    if (!(mask & 0x1)) continue;
    xmpu = &xmpu_device[xmpu_dev_n];

#if defined(CONFIG_XMPU_DEBUG)
    xmpu_print("XMPU device channel %d (addr: 0x%08x)\n\r", xmpu_dev_n, xmpu->base_addr);
#endif // CONFIG_XMPU_DEBUG

    for_each_mem_region(mem, cell->config, n){
      // root_cell ignore the addr_space (full access)
      if(mem_in_xmpu_addr_range(xmpu, mem) || cell->config->id == root_cell.config->id) {
        addr_start = mem->phys_start;
        addr_end =  mem->phys_start + mem->size - 1;
        wrallowed = (mem->flags & JAILHOUSE_MEM_WRITE) ? 1 : 0;
        rdallowed = (mem->flags & JAILHOUSE_MEM_READ) ? 1 : 0;

        // Check for free region in the channel
        for(i = 0; i < NR_XMPU_REGIONS; i++){
          if(xmpu->region[i].used == 0){
            valid_reg_n = i;
            break;
          }
        }
        if (i == NR_XMPU_REGIONS){
          xmpu_print("ERROR: No XMPU free region, impossible to create the VM\n\r");
          return -1;
        }

#if defined(CONFIG_XMPU_DEBUG)
        xmpu_print("Setting XMPU region %d\n\r", valid_reg_n);
#endif // CONFIG_XMPU_DEBUG

        enable_region_configuration(&xmpu->region[valid_reg_n], 
                          addr_start, 
                          addr_end, 
                          dev->id, 
                          dev->mask, 
                          wrallowed,
                          rdallowed,
                          cell->config->id);
        set_xmpu_region(xmpu, valid_reg_n);

        // rootcell needs only one region per channel
        if(cell->config->id == root_cell.config->id) break; 
      }
    }
  }

  return 0;
}

static void arm_xmpu_cell_exit(struct cell *cell){
  struct master_device *dev = NULL; 
  unsigned int rcpu, fpga_region;

  xmpu_print("Removing XMPU permissions for cell %d\n\r", cell->config->id);

  if(cell->config->fpga_regions_size != 0){ 
    for_each_region(fpga_region, cell->fpga_region_set){
      switch (fpga_region)
      {
        case 0:
          dev = &master_device_list[TBU3];
          break;
        case 1:
          dev = &master_device_list[TBU4];
          break;
        case 2:
          dev = &master_device_list[TBU5];
          break;
        default:
          xmpu_print("Error: FPGA region not valid\n\r");
          continue;
      }

      clean_cell_permissions(dev, cell->config->id);
      set_cell_permissions(dev, &root_cell);
    }
  }

  if(cell->config->rcpu_set_size != 0){

    for_each_cpu(rcpu, cell->rcpu_set) {
      switch (rcpu)
      {
        case 0:
          dev = &master_device_list[RPU0];
          break;
        case 1:
          dev = &master_device_list[RPU1];
          break;
        default:
          continue; // skip soft-core for now
      }

      clean_cell_permissions(dev, cell->config->id);
      set_cell_permissions(dev, &root_cell);
    }
  } 
}

static int arm_xmpu_cell_init(struct cell *cell){
  struct master_device *dev;
	unsigned int rcpu, fpga_region;
  xmpu_print("Setting XMPU permissions for cell %d\n\r", cell->config->id);
  
  if(cell->config->fpga_regions_size > 0){
    for_each_region(fpga_region, cell->fpga_region_set){
      switch (fpga_region)
      {
        case 0:
          dev = &master_device_list[TBU3];
          break;
        case 1:
          dev = &master_device_list[TBU4];
          break;
        case 2:
          dev = &master_device_list[TBU5];
          break;
        default:
          xmpu_print("Error: FPGA region not valid\n\r");
          return -1;
      }
      // Clean region used by root-cell and set the permissions for the cell
      clean_cell_permissions(dev, root_cell.config->id);
      set_cell_permissions(dev, cell);
    }
  }

  if(cell->config->rcpu_set_size != 0){
    for_each_cpu(rcpu, cell->rcpu_set) {
      switch (rcpu)
      {
        case 0:
          dev = &master_device_list[RPU0];
          break;
        case 1:
          dev = &master_device_list[RPU1];
          break;
        default:
          // TODO: Daniele Ottaviano, fine grained management of soft-core permissions
          continue; // soft-core, not supported yet
      }
      // Clean region used by root-cell and set the permissions for the cell
      clean_cell_permissions(dev, root_cell.config->id);
      set_cell_permissions(dev, cell);
    }
  }

  return 0;
}

// Config the XMPU to its default values
static void arm_xmpu_shutdown(void){
  u8 i = 0;
  xmpu_print("Resetting XMPU to default values\n\r");

  // Set to default values
  for(i=0 ; i<NR_XMPU; i++){
    set_xmpu_default(&xmpu_device[i]);
  }
}

// Initialize the XMPU
static int arm_xmpu_init(void){
  u8 i = 0;

  // Configure all XMPU registers to default values
  for(i=0 ; i<NR_XMPU; i++){
    set_xmpu_default(&xmpu_device[i]);
  } 

  // Configure XMPU region registers for each device
  for(i=0; i < NR_MASTER_DEVICES; i++){
    set_cell_permissions(&master_device_list[i], &root_cell);
  }

  // Configure XMPU status registers for all the channels to remove default read/write permissions
  for(i=0 ;  i<NR_XMPU; i++){
    poison_xmpu_status(&xmpu_device[i]);
  }

  return 0;
}

DEFINE_UNIT_MMIO_COUNT_REGIONS_STUB(arm_xmpu);
DEFINE_UNIT(arm_xmpu, "ARM XMPU");
#endif /* CONFIG_XMPU && CONFIG_MACH_ZYNQMP_ZCU102 */ 