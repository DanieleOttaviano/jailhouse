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

/* FPD regions (to do ...) */

/* OCM regions (to do ...) */

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
static xmpu_channel ddr_xmpu_device[NR_XMPU_DDR];
static xmpu_channel fpd_xmpu_device;
static xmpu_channel ocm_xmpu_device;

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

typedef struct master_device{
  u64 id;
  u64 mask;
  u8 ddr_xmpu_channel_mask;
} master_device;

/* Static master device list */
static master_device master_device_list[] = {
    { RPU0_ID,        RPU0_MASK,        (u8)0b000001 },
    { RPU1_ID,        RPU1_MASK,        (u8)0b000001 },
    { GROUP0_ID,      GROUP0_MASK,      (u8)0b000110 },
    { GROUP1_ID,      GROUP1_MASK,      (u8)0b000110 },
    { GROUP2_ID,      GROUP2_MASK,      (u8)0b000110 },
    { APU_ID,         APU_MASK,         (u8)0b000110 },
    { DISPLAYPORT_ID, DISPLAYPORT_MASK, (u8)0b001000 },
    { FPD_DMA_ID,     FPD_DMA_MASK,     (u8)0b001000 },
    { TBU3_ID,        TBU3_MASK,        (u8)0b001000 },
    { TBU4_ID,        TBU4_MASK,        (u8)0b010000 },
    { TBU5_ID,        TBU5_MASK,        (u8)0b100000 }
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
static void set_xmpu_status(u32 xmpu_base, xmpu_status_config *config){
  u32 xmpu_ctrl_register;
  u32 xmpu_poison_register;
  u32 xmpu_lock_register;
  bool poison;
 
  poison = (xmpu_base == XMPU_FPD_BASE_ADDR) ? !(config->poison) : config->poison; // The FPD XMPU uses negative logic on the poison bit

  if(xmpu_base == XMPU_FPD_BASE_ADDR){
    xmpu_ctrl_register	  = (0x00000000 | (config->align << 3) | (1<<2) | (config->def_wr_allowed << 1) | (config->def_rd_allowed));
  }
  else{ // DDR and OCM
    xmpu_ctrl_register	  = (0x00000000 | (config->align << 3) | (0<<2) | (config->def_wr_allowed << 1) | (config->def_rd_allowed));
  }

  xmpu_poison_register	=	(0x00000000 | (poison << 20));
  xmpu_lock_register	  =	(0x00000000 | (config->lock));

  xmpu_write32((void *)(XMPU_CTRL_REGISTER(xmpu_base)), xmpu_ctrl_register);
  xmpu_write32((void *)(XMPU_POISON_REGISTER(xmpu_base)), xmpu_poison_register);
  xmpu_write32((void *)(XMPU_LOCK_REGISTER(xmpu_base)), xmpu_lock_register);
}

// Set the XMPU region_offset registers of the xmpu_base
static void set_xmpu_region(u32 xmpu_base, u32 region_offset, xmpu_region_config *config){
  u32 tmp_addr;
  u32 xmpu_start_register;
  u32 xmpu_end_register;
  u32 xmpu_master_register;
  u32 xmpu_config_register;

  if(xmpu_base == XMPU_FPD_BASE_ADDR){
    tmp_addr = config->addr_start >> 12;
    xmpu_start_register = tmp_addr; 
    tmp_addr = config->addr_end >> 12;
    xmpu_end_register = tmp_addr;	
  } else if(xmpu_base == XMPU_OCM_BASE_ADDR){
    tmp_addr = config->addr_start;
    xmpu_start_register = tmp_addr; 
    tmp_addr = config->addr_end;
    xmpu_end_register = tmp_addr;	 
  } else {
    tmp_addr = config->addr_start >> 20;
    xmpu_start_register = tmp_addr << 8;
    tmp_addr = config->addr_end >> 20;
    xmpu_end_register = tmp_addr << 8;
  }
  xmpu_master_register = (0x00000000 | (config->master_mask << 16) | (config->master_id));
  xmpu_config_register = (0x00000000 | (config->ns_checktype << 4) | (config->region_ns << 3) | (config->wrallowed << 2) | (config->rdallowed << 1) | (config->enable)); 

  xmpu_write32((void *)(XMPU_START_REGISTER(xmpu_base, region_offset)), xmpu_start_register);
  xmpu_write32((void *)(XMPU_END_REGISTER(xmpu_base, region_offset)), xmpu_end_register);
  xmpu_write32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region_offset)), xmpu_master_register);
  xmpu_write32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region_offset)), xmpu_config_register);
}

// Set the XMPU region registers of the xmpu_base to default values
static void set_xmpu_region_default(u32 xmpu_base, u32 region_offset){
  u32 xmpu_start_register   = XMPU_DEFAULT_START;
  u32 xmpu_end_register     = XMPU_DEFAULT_END;
  u32 xmpu_master_register  = XMPU_DEFAULT_MASTER;
  u32 xmpu_config_register  = XMPU_DEFAULT_CONFIG;

  xmpu_write32((void *)(XMPU_START_REGISTER(xmpu_base, region_offset)), xmpu_start_register);
  xmpu_write32((void *)(XMPU_END_REGISTER(xmpu_base, region_offset)), xmpu_end_register);
  xmpu_write32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region_offset)), xmpu_master_register);
  xmpu_write32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region_offset)), xmpu_config_register);
}

// Set the XMPU status registers of the xmpu_base to default values
static void set_xmpu_status_default(u32 xmpu_base){
  u32 xmpu_ctrl_register    = 0;
  u32 xmpu_poison_register  = 0;
  u32 xmpu_lock_register    = 0;

  if (xmpu_base == XMPU_DDR_0_BASE_ADDR || xmpu_base == XMPU_DDR_1_BASE_ADDR || 
      xmpu_base == XMPU_DDR_2_BASE_ADDR || xmpu_base == XMPU_DDR_3_BASE_ADDR || 
      xmpu_base == XMPU_DDR_4_BASE_ADDR || xmpu_base == XMPU_DDR_5_BASE_ADDR)
  {
    xmpu_ctrl_register    = XMPU_DDR_DEFAULT_CTRL;
    xmpu_poison_register  = XMPU_DDR_DEFAULT_POISON;  
  }
  else if(xmpu_base == XMPU_FPD_BASE_ADDR){
    xmpu_ctrl_register    = XMPU_FPD_DEFAULT_CTRL;
    xmpu_poison_register  = XMPU_FPD_DEFAULT_POISON;
  }
  else if(xmpu_base == XMPU_OCM_BASE_ADDR){
    xmpu_ctrl_register    = XMPU_OCM_DEFAULT_CTRL;
    xmpu_poison_register  = XMPU_OCM_DEFAULT_POISON;
  }
  else{
    xmpu_print("ERROR: XMPU base address not valid\n\r");
  }
  
  xmpu_write32((void *)(XMPU_CTRL_REGISTER(xmpu_base)), xmpu_ctrl_register);
  xmpu_write32((void *)(XMPU_POISON_REGISTER(xmpu_base)), xmpu_poison_register);
  xmpu_write32((void *)(XMPU_LOCK_REGISTER(xmpu_base)), xmpu_lock_register); 
}

// Set all the XMPU registers of the xmpu_base to default values
static void set_xmpu_default(u32 xmpu_base){
  u8 i = 0;
  u32 region_offset;

  set_xmpu_status_default(xmpu_base);

  for(i=0 ; i<NR_XMPU_REGIONS; i++){
    region_offset = i * XMPU_REGION_OFFSET;
    set_xmpu_region_default(xmpu_base, region_offset);
  }

}

// Setup region configuration
static void setup_region_configuration(xmpu_region_config *config, 
                              u64 addr_start, u64 addr_end, 
                              u64 master_id, u64 master_mask,
                              bool ns_checktype, bool region_ns,
                              bool wrallowed, bool rdallowed,
                              bool enable, u16 id, bool used){
  config->addr_start =   addr_start;
  config->addr_end =     addr_end;
  config->master_id =    master_id;
  config->master_mask =  master_mask;
  config->ns_checktype = ns_checktype;
  config->region_ns =    region_ns;
  config->rdallowed =    rdallowed;
  config->wrallowed =    wrallowed;
  config->enable =       enable;
  config->id =           id;
  config->used =         used;
}

// Setup region configuration for the root cell (full access)
static void setup_region_configuration_rootcell(xmpu_region_config * config, 
                                      struct master_device *dev) {
  setup_region_configuration(config, 
                    0x0000000000, // start address
                    0xFFFFFFFFFF, // end address
                    dev->id, dev->mask, 
                    0, 1,     // ns_checktype, region_ns
                    1, 1,     // wrallowed, rdallowed
                    1,        // enable
                    root_cell.config->id, 1); // id, used
}

// Helper function to clean xmpu permissions of a specific device for all the regions used by a cell
static void clean_cell_permissions(struct master_device *dev, u8 cell_id) { 
  u8 i = 0;
  u8 valid_reg_n = 0; 
  u8 xmpu_channel_n = 0;
  u32 region_base = 0;
  u32 xmpu_base; 
  xmpu_channel *xmpu_chnl = NULL;
  u8 mask = dev->ddr_xmpu_channel_mask;

  for (xmpu_channel_n = 0; mask; xmpu_channel_n++, mask >>= 1) {
    xmpu_print("mask = 0x%05x, xmpu_channel_n = %d\n\r", mask, xmpu_channel_n);
    if (!(mask & 0x1))
      continue;
    xmpu_base = XMPU_DDR_0_BASE_ADDR + (xmpu_channel_n * XMPU_DDR_OFFSET);
    xmpu_chnl = &ddr_xmpu_device[xmpu_channel_n];

    for(i = 0; i < NR_XMPU_REGIONS; i++) {
      if(xmpu_chnl->region[i].id == cell_id && xmpu_chnl->region[i].used == 1 && xmpu_chnl->region[i].master_id == dev->id && xmpu_chnl->region[i].master_mask == dev->mask) {
        
        valid_reg_n = i;
        region_base = valid_reg_n * XMPU_REGION_OFFSET;

#if defined(CONFIG_XMPU_DEBUG)
        xmpu_print("Cleaning XMPU region %d for cell %d on dev %llu\n\r", valid_reg_n, cell_id, dev->id);
        xmpu_print("Memory region: 0x%08llx - 0x%08llx\n\r", xmpu_chnl->region[valid_reg_n].addr_start, xmpu_chnl->region[valid_reg_n].addr_end);
        xmpu_print("XMPU base address: 0x%08x\n\r", (xmpu_base + region_base));
        xmpu_print("XMPU channel %d (offset: 0x%08x)\n\r", xmpu_channel_n, (xmpu_channel_n * XMPU_DDR_OFFSET));
        xmpu_print("XMPU region  %d (offset: 0x%08x)\n\r", valid_reg_n, region_base);
        xmpu_print("Master ID:    0x%04llx\n\r", dev->id);
        xmpu_print("Master Mask:  0x%04llx\n\r", dev->mask);
#endif // CONFIG_XMPU_DEBUG

        set_xmpu_region_default(xmpu_base, region_base);
        xmpu_chnl->region[valid_reg_n].id = 0;
        xmpu_chnl->region[valid_reg_n].used = 0;
      }
    }
  }
}

// Helper function to set xmpu permissions to cell
static int set_cell_permissions(struct master_device *dev, struct cell *cell) {
  u8 i = 0;
  u8 valid_reg_n = 0;
  u32 region_offset = 0;
  u32 xmpu_base;
  u8 xmpu_channel_n = 0;
  u8 mask = dev->ddr_xmpu_channel_mask;
  u32 addr_start, addr_end;
  xmpu_channel *xmpu_chnl = NULL;
  const struct jailhouse_memory *mem;
  unsigned int n;
  
  for (xmpu_channel_n = 0; mask; xmpu_channel_n++, mask >>= 1) {
    xmpu_print("mask = 0x%05x, xmpu_channel_n = %d\n\r", mask, xmpu_channel_n);
    if (!(mask & 0x1))
      continue;
    xmpu_base = XMPU_DDR_0_BASE_ADDR + (xmpu_channel_n * XMPU_DDR_OFFSET);
    xmpu_chnl = &ddr_xmpu_device[xmpu_channel_n];

    for_each_mem_region(mem, cell->config, n){
      // If not in DDR memory exit
      if (!((mem->virt_start <= DDR_LOW_END - mem->size) || 
        ((mem->virt_start >= DDR_HIGH_START) && (mem->virt_start <= DDR_HIGH_END - mem->size )))) {
        continue;
      }
      addr_start = mem->phys_start;
      addr_end =  mem->phys_start + mem->size - 1;
        
      // Check for free region in the channel
      for(i = 0; i < NR_XMPU_REGIONS; i++){
        if(xmpu_chnl->region[i].used == 0){
          valid_reg_n = i; 
          break;
        }
      }
      if (i == NR_XMPU_REGIONS){
        xmpu_print("ERROR: No XMPU free region, impossible to create the VM\n\r");
        return -1;
      }
      region_offset = valid_reg_n * XMPU_REGION_OFFSET;

#if defined(CONFIG_XMPU_DEBUG)
      xmpu_print("Setting XMPU region %d for cell %d\n\r", valid_reg_n, cell->config->id);
      xmpu_print("Memory region: %d  (0x%08x - 0x%08x)\n\r", n, addr_start, addr_end);
      xmpu_print("XMPU base address: 0x%08x\n\r", xmpu_base);
      xmpu_print("XMPU channel %d (offset: 0x%08x)\n\r", xmpu_channel_n, (xmpu_channel_n * XMPU_DDR_OFFSET));
      xmpu_print("XMPU region  %d (offset: 0x%08x)\n\r", valid_reg_n, region_offset);
      xmpu_print("Master ID:    0x%04llx\n\r", dev->id);
      xmpu_print("Master Mask:  0x%04llx\n\r", dev->mask);
#endif // CONFIG_XMPU_DEBUG

      // the root cell has dedicated configuration (full access) and need only the first free region per channel
      if(cell->config->id == root_cell.config->id){
        setup_region_configuration_rootcell(&xmpu_chnl->region[valid_reg_n], dev);
        set_xmpu_region(xmpu_base, region_offset, &xmpu_chnl->region[valid_reg_n]);
        break;
      }

      setup_region_configuration(&xmpu_chnl->region[valid_reg_n], 
                        addr_start, addr_end, 
                        dev->id, dev->mask, 
                        0, 0, // ns_checktype and region_ns
                        (mem->flags & JAILHOUSE_MEM_WRITE) ? 1 : 0, // wrallowed
                        (mem->flags & JAILHOUSE_MEM_READ) ? 1 : 0,  // rdallowed
                        1,  //enable 
                        cell->config->id, 
                        1); //used
      set_xmpu_region(xmpu_base, region_offset, &xmpu_chnl->region[valid_reg_n]);   
    }
  }

  return 0;
}

static void arm_xmpu_cell_exit(struct cell *cell){
  struct master_device *dev = NULL; 
  unsigned int cpu, fpga_region;

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

    for_each_cpu(cpu, cell->rcpu_set) {
      switch (cpu)
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
	unsigned int cpu, fpga_region;
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
    for_each_cpu(cpu, cell->rcpu_set) {
      switch (cpu)
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
  u32 xmpu_base = 0;
  xmpu_print("Resetting XMPU to default values\n\r");

  // Set to default values
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base=XMPU_DDR_BASE_ADDR;
    set_xmpu_default(xmpu_base + (i*XMPU_DDR_OFFSET));
  }
  xmpu_base=XMPU_FPD_BASE_ADDR;
  set_xmpu_default(xmpu_base);
  xmpu_base=XMPU_OCM_BASE_ADDR;
  set_xmpu_default(xmpu_base);

#if defined(CONFIG_XMPU_DEBUG)
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET); 
    //xmpu_print("DDR channel %d\n\r", i);
    print_xmpu_status_regs(xmpu_base);
  }
  //xmpu_print("FPD channel 0\n\r");
  print_xmpu_status_regs(XMPU_FPD_BASE_ADDR);
  //xmpu_print("OCM channel 0\n\r");
  print_xmpu_status_regs(XMPU_OCM_BASE_ADDR); 
#endif // CONFIG_XMPU_DEBUG
}

// DDR XMPU init
static void xmpu_ddr_init(void){
  u8 i,j = 0;
  u8 xmpu_channel_n = 0;
  u32 xmpu_base = 0;
  
  // Configure all XMPU DDR registers to default values
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET);
    set_xmpu_default(xmpu_base);
  } 

  // Configure XMPU DDR region registers for each device
  for( j=0; j < NR_MASTER_DEVICES; j++){
    xmpu_print("Setting XMPU permissions for master device %d (ID: 0x%04llx, Mask: 0x%04llx)\n\r", j, master_device_list[j].id, master_device_list[j].mask);
    set_cell_permissions(&master_device_list[j], &root_cell);
  }

  // Configure XMPU status registers for all ddr the channels to remove default read/write permissions
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET);  
    xmpu_channel_n = i;   
    
    ddr_xmpu_device[xmpu_channel_n].status.poison =        1;
    ddr_xmpu_device[xmpu_channel_n].status.align =         1; //1Mb
    ddr_xmpu_device[xmpu_channel_n].status.def_wr_allowed =0;
    ddr_xmpu_device[xmpu_channel_n].status.def_rd_allowed =0;
    ddr_xmpu_device[xmpu_channel_n].status.lock =          0;
    set_xmpu_status(xmpu_base, &ddr_xmpu_device[xmpu_channel_n].status);    
  }
}

// FPD XMPU init
static void xmpu_fpd_init(void){
  u32 xmpu_base = XMPU_FPD_BASE_ADDR;

  // Configure all XMPU FPD registers to default values
  set_xmpu_default(xmpu_base);

  // Configure XMPU FPD region registers
  setup_region_configuration_rootcell(&fpd_xmpu_device.region[0], &master_device_list[APU]);
  set_xmpu_region(xmpu_base, R00_OFFSET, &fpd_xmpu_device.region[0]);  
  
  // Configure XMPU FPD status registers to remove default read/write permissions
  fpd_xmpu_device.status.poison =        1;
  fpd_xmpu_device.status.align =         0; //4kb
  fpd_xmpu_device.status.def_wr_allowed =0;
  fpd_xmpu_device.status.def_rd_allowed =0;
  fpd_xmpu_device.status.lock =          0;
  set_xmpu_status(xmpu_base, &fpd_xmpu_device.status);
}

// OCM XMPU init
static void xmpu_ocm_init(void){
  u32 xmpu_base = XMPU_OCM_BASE_ADDR;

  // Configure all XMPU OCM registers to default values
  set_xmpu_default(xmpu_base);
  
  // Configure XMPU OCM region registers
  setup_region_configuration_rootcell(&ocm_xmpu_device.region[0], &master_device_list[APU]);
  set_xmpu_region(xmpu_base, R00_OFFSET, &ocm_xmpu_device.region[0]);
  setup_region_configuration_rootcell(&ocm_xmpu_device.region[1], &master_device_list[RPU0]);
  set_xmpu_region(xmpu_base, R01_OFFSET, &ocm_xmpu_device.region[1]);
  setup_region_configuration_rootcell(&ocm_xmpu_device.region[2], &master_device_list[RPU1]);
  set_xmpu_region(xmpu_base, R02_OFFSET, &ocm_xmpu_device.region[2]);

  // Configure XMPU OCM status registers to remove default read/write permissions
  ocm_xmpu_device.status.poison =        1;
  ocm_xmpu_device.status.align =         0; //4kb
  ocm_xmpu_device.status.def_wr_allowed =0;
  ocm_xmpu_device.status.def_rd_allowed =0;
  ocm_xmpu_device.status.lock =          0;
  set_xmpu_status(xmpu_base, &ocm_xmpu_device.status);
}

// Initialize the XMPU
static int arm_xmpu_init(void){

#if defined(CONFIG_XMPU_DEBUG)
  u8 i = 0;
  u32 xmpu_base = 0;
#endif /* CONFIG_XMPU_DEBUG */

  /* DDR */
  xmpu_ddr_init();
  
  /* FPD */
  xmpu_fpd_init();
  
  /* OCM */
  xmpu_ocm_init();

#if defined(CONFIG_XMPU_DEBUG)
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET); 
    xmpu_print("DDR channel %d\n\r", i);
    print_xmpu(xmpu_base);
  }
  xmpu_print("FPD channel 0\n\r");
  print_xmpu(XMPU_FPD_BASE_ADDR);
  xmpu_print("OCM channel 0\n\r");
  print_xmpu(XMPU_OCM_BASE_ADDR);
#endif /* CONFIG_XMPU_DEBUG */

  return 0;
}

DEFINE_UNIT_MMIO_COUNT_REGIONS_STUB(arm_xmpu);
DEFINE_UNIT(arm_xmpu, "ARM XMPU");
#endif /* CONFIG_XMPU && CONFIG_MACH_ZYNQMP_ZCU102 */ 