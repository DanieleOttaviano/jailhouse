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

// Set the XMPU status registers of the xmpu_base
static void set_xmpu_status(u32 xmpu_base, xmpu_status_config *config){
  u32 xmpu_ctrl_register;
  u32 xmpu_poison_register;
  u32 xmpu_lock_register;
  bool poison;
 
  poison = (xmpu_base == XMPU_FPD_BASE_ADDR) ? !(config->poison) : config->poison; // to do add explaination ...

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
  u32 xmpu_start_register   = 0x00000000;
  u32 xmpu_end_register     = 0x00000000;
  u32 xmpu_master_register  = 0x00000000;
  u32 xmpu_config_register  = 0x00000008; 

  xmpu_write32((void *)(XMPU_START_REGISTER(xmpu_base, region_offset)), xmpu_start_register);
  xmpu_write32((void *)(XMPU_END_REGISTER(xmpu_base, region_offset)), xmpu_end_register);
  xmpu_write32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region_offset)), xmpu_master_register);
  xmpu_write32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region_offset)), xmpu_config_register);
}

// Set the XMPU status registers of the xmpu_base to default values
static void set_xmpu_status_default(u32 xmpu_base){
  u32 xmpu_ctrl_register    = 0x00000000;
  u32 xmpu_poison_register  = 0x00000000;
  u32 xmpu_lock_register    = 0x00000000;

  if (xmpu_base == XMPU_DDR_0_BASE_ADDR || xmpu_base == XMPU_DDR_1_BASE_ADDR || 
      xmpu_base == XMPU_DDR_2_BASE_ADDR || xmpu_base == XMPU_DDR_3_BASE_ADDR || 
      xmpu_base == XMPU_DDR_4_BASE_ADDR || xmpu_base == XMPU_DDR_5_BASE_ADDR)
  {
    xmpu_ctrl_register    = 0x0000000b;
    xmpu_poison_register  = 0x00000000;
  }
  else if(xmpu_base == XMPU_FPD_BASE_ADDR){
    xmpu_ctrl_register    = 0x00000007;
    xmpu_poison_register  = 0x000fd4f0;
  }
  else if(xmpu_base == XMPU_OCM_BASE_ADDR){
    xmpu_ctrl_register    = 0x00000003;
    xmpu_poison_register  = 0x00000000;
  }
  else{
    //xmpu_print("ERROR: XMPU base address not valid\n\r");
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

//Print the XMPU status registers
void print_xmpu_status_regs(u32 xmpu_base){

  //xmpu_print("CTRL:        0x%08x\n\r", xmpu_read32((void *)(XMPU_CTRL_REGISTER(xmpu_base))));
  //xmpu_print("ERR_STATUS1: 0x%08x\n\r", xmpu_read32((void *)(XMPU_ERR_STATUS1_REGISTER(xmpu_base))));
  //xmpu_print("ERR_STATUS2: 0x%08x\n\r", xmpu_read32((void *)(XMPU_ERR_STATUS2_REGISTER(xmpu_base))));
  //xmpu_print("POISON:      0x%08x\n\r", xmpu_read32((void *)(XMPU_POISON_REGISTER(xmpu_base))));
  //xmpu_print("ISR:         0x%08x\n\r", xmpu_read32((void *)(XMPU_ISR_REGISTER(xmpu_base))));
  //xmpu_print("IMR:         0x%08x\n\r", xmpu_read32((void *)(XMPU_IMR_REGISTER(xmpu_base))));
  //xmpu_print("IEN:         0x%08x\n\r", xmpu_read32((void *)(XMPU_IEN_REGISTER(xmpu_base))));
  //xmpu_print("IDS:         0x%08x\n\r", xmpu_read32((void *)(XMPU_IDS_REGISTER(xmpu_base))));
  //xmpu_print("LOCK:        0x%08x\n\r", xmpu_read32((void *)(XMPU_LOCK_REGISTER(xmpu_base))));
  //xmpu_print("\n\r");
}

// Print the XMPU region registers
void print_xmpu_region_regs(u32 xmpu_base, u32 region){

  //xmpu_print("START:   0x%08x\n\r", xmpu_read32((void *)(XMPU_START_REGISTER(xmpu_base, region))));
  //xmpu_print("END:     0x%08x\n\r", xmpu_read32((void *)(XMPU_END_REGISTER(xmpu_base, region))));
  //xmpu_print("MASTER:  0x%08x\n\r", xmpu_read32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region))));
  //xmpu_print("CONFIG:  0x%08x\n\r", xmpu_read32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region))));
  //xmpu_print("\n\r");
}

// Print all the XMPU registers
void print_xmpu(u32 xmpu_base){
  int i=0;
  print_xmpu_status_regs(xmpu_base);
  for (i = 0; i < NR_XMPU_REGIONS; i++) {
    print_xmpu_region_regs(xmpu_base, i);
  }
}

// Helper function to set default xmpu permissions of a cell
static void set_default_cell_permissions(xmpu_channel *xmpu_chnl, u32 xmpu_base, struct cell *cell) { 
  u8 i = 0;
  u8 valid_reg_n = 0;
  u32 region_base = 0;


  for(i = 0; i<NR_XMPU_REGIONS; i++){
    if(xmpu_chnl->region[i].id == cell->config->id){ 
      // Clean regions used by the cell
      valid_reg_n = i;
      region_base = valid_reg_n * XMPU_REGION_OFFSET;

      //DEBUG
      // xmpu_print("Cleaning memory region: 0x%08llx - 0x%08llx\n\r", xmpu_chnl->region[valid_reg_n].addr_start, xmpu_chnl->region[valid_reg_n].addr_end);
      // xmpu_print("XMPU base address: 0x%08x\n\r", (xmpu_base + region_base));
      // xmpu_print("XMPU region: %d\n\r", valid_reg_n);

      set_xmpu_region_default(xmpu_base, region_base);
      xmpu_chnl->region[valid_reg_n].id = 0;
      xmpu_chnl->region[valid_reg_n].used = 0;
    }
  }
}


// Cell exit: config XMPU
static void arm_xmpu_cell_exit(struct cell *cell){
  u8 xmpu_channel_n = 0;
  u32 xmpu_base = 0;
  xmpu_channel *xmpu_chnl;
  unsigned int cpu;
  unsigned int region;

  //DEBUG  
  xmpu_print("Shutting down XMPU for cell %d\n\r", cell->config->id);

  // For each FPGA region in the cell configuration Set the XMPU region registers to default values
  if(cell->config->fpga_regions_size != 0){ 
    for_each_region(region, cell->fpga_region_set){

      if(region > 2){
        //DEBUG
        xmpu_print("Error: FPGA region not valid\n\r");
        continue;
      }
      //DEBUG
      xmpu_print("FPGA region %d\n\r", region); 
      xmpu_channel_n = region + 3; 
      xmpu_base = XMPU_DDR_3_BASE_ADDR + (region * XMPU_REGION_OFFSET);
 
      // DEBUG
      xmpu_print("Setting FPGA access on channel %d to default\n\r", xmpu_channel_n);

      xmpu_chnl = &ddr_xmpu_device[xmpu_channel_n];
      // set default permissions
      set_default_cell_permissions(xmpu_chnl, xmpu_base, cell);
    }
  }
  else{
    //DEBUG
    xmpu_print("No FPGA regions in this cell\n\r"); 
  }

  // For each rCPU in the cell configuration Set the XMPU region registers to default values
  if(cell->config->rcpu_set_size != 0){
    // todo ... Take from cell configuration and do it for all the subordinates (DDR, FPD, OCM)
    xmpu_base = XMPU_DDR_BASE_ADDR;

    for_each_cpu(cpu, cell->rcpu_set) {
      if(cpu == 0){           // RPU 0
        xmpu_channel_n = 0;
      }
      else if(cpu == 1){      // RPU 1
        xmpu_channel_n = 0;
      }
      else{
        // xmpu_print("Error: rCPU doesn't exist\r\n");
        // soft-core
        continue;
      }
      // DEBUG
      xmpu_print("Setting rCPU %d access on channel %d to default\n\r", cpu, xmpu_channel_n);    
      xmpu_chnl = &ddr_xmpu_device[xmpu_channel_n];

      // Set default permissions
      set_default_cell_permissions(xmpu_chnl, xmpu_base, cell);
    }
  } 
  else{
    //DEBUG
    xmpu_print("No rCPUs in this cell\n\r"); 
  }
}

// Helper function to set xmpu permissions to cell
static int set_cell_permissions(u8 xmpu_channel_n, u32 xmpu_base, u64 cell_master_id, u64 cell_master_mask, struct cell *cell) {
  u8 i = 0;
  u8 valid_reg_n = 0;
  u32 region_base = 0;
  u32 addr_start, addr_end;
  xmpu_channel *xmpu_chnl;
  const struct jailhouse_memory *mem;
  unsigned int n;

  for_each_mem_region(mem, cell->config, n){
    // If in DDR memory
    if((mem->virt_start <= DDR_LOW_END - mem->size) || 
      ((mem->virt_start >= DDR_HIGH_START) && (mem->virt_start <= DDR_HIGH_END - mem->size ))){ 
      xmpu_chnl = &ddr_xmpu_device[xmpu_channel_n];
    }
    else{
      continue;
    }

    addr_start = mem->phys_start;
    addr_end =  mem->phys_start + mem->size - 1;
    
    // Check for free region in the channel
    for(i = 0; i<NR_XMPU_REGIONS; i++){
      if(xmpu_chnl->region[i].used == 0){
        valid_reg_n = i; 
        break;
      }
    }
    if (i == NR_XMPU_REGIONS){
      xmpu_print("ERROR: No XMPU free region, impossible to create the VM\n\r");
      return -1;
    }
    region_base = valid_reg_n * XMPU_REGION_OFFSET;

    //DEBUG
    // xmpu_print("Allowed memory region: 0x%08x - 0x%08x\n\r", addr_start, addr_end);
    // xmpu_print("XMPU DDR Channel: %d\n\r", xmpu_channel_n);
    // xmpu_print("XMPU base address: 0x%08x\n\r", (xmpu_base + region_base));
    // xmpu_print("XMPU region: %d\n\r", valid_reg_n);
    // xmpu_print("Master ID:    0x%04llx\n\r", cell_master_id);
    // xmpu_print("Master Mask:  0x%04llx\n\r", cell_master_mask);
    
    // Configure region
    xmpu_chnl->region[valid_reg_n].addr_start =     addr_start;
    xmpu_chnl->region[valid_reg_n].addr_end =       addr_end;
    xmpu_chnl->region[valid_reg_n].master_id =      cell_master_id; 
    xmpu_chnl->region[valid_reg_n].master_mask =    cell_master_mask;
    xmpu_chnl->region[valid_reg_n].ns_checktype =   0;
    xmpu_chnl->region[valid_reg_n].region_ns =      0;
    xmpu_chnl->region[valid_reg_n].wrallowed =      (mem->flags & JAILHOUSE_MEM_WRITE) ? 1 : 0;
    xmpu_chnl->region[valid_reg_n].rdallowed =      (mem->flags & JAILHOUSE_MEM_READ) ? 1 : 0;
    xmpu_chnl->region[valid_reg_n].enable =         1;
    xmpu_chnl->region[valid_reg_n].id =             cell->config->id;
    xmpu_chnl->region[valid_reg_n].used =           1;
    set_xmpu_region(xmpu_base, region_base, &xmpu_chnl->region[valid_reg_n]);   
  }

  return 0;
}


// Cell init: config XMPU
static int arm_xmpu_cell_init(struct cell *cell){
  u32 xmpu_base = 0;
  u8 xmpu_channel_n = 0;
  u64 cell_master_id = 0;
  u64 cell_master_mask = 0;
	unsigned int cpu;
	unsigned int region;

  //xmpu_print("Initializing XMPU for cell %d\n\r", cell->config->id);
  
  // If FPGA in cell, give the requested accesses
  // to do ... specify the FPGA regions and give the permession to the corresponding maser ID
  if(cell->config->fpga_regions_size > 0){
    // S_AXI_HP0_FPD (HP0) (1010, AXI ID[5:0]) DOESN'T WORK ...
    for_each_region(region, cell->fpga_region_set){
      // DEBUG
      xmpu_print("FPGA region %d\n\r", region); 
      xmpu_channel_n = region + 3; 
      xmpu_base = XMPU_DDR_3_BASE_ADDR + (region * XMPU_DDR_OFFSET);

      if(region == 0){
        // All that is coming from TBU 3 
        cell_master_id = 0x2000;    // 0010 00(XX XXXX XXXX)  to do: take from cell configuration
        cell_master_mask = 0xFC00;  // 1111 11(00 0000 0000)  to do: take from cell configuration       

        // S_AXI_HP0_FPD (HP0) -> 1010, AXI ID [5:0] from PL
        // cell_master_id = 0x0280;    // 0000 00(10 1000 0000)  to do: take from cell configuration
        // cell_master_mask = 0x03C0;  // 0000 00(11 1100 0000)  to do: take from cell configuration   
      }
      else if(region == 1){
        // All that is coming from TBU 4
        cell_master_id = 0x4000;    // 0100 00(XX XXXX XXXX)  to do: take from cell configuration
        cell_master_mask = 0xFC00;  // 1111 11(00 0000 0000)  to do: take from cell configuration       

        // S_AXI_HP1_FPD (HP1) -> 1011, AXI ID [5:0] from PL
        // cell_master_id = 0x02C0;    // 0000 00(10 1100 0000)  to do: take from cell configuration
        // cell_master_mask = 0x03C0;  // 0000 00(11 1100 0000)  to do: take from cell configuration   
 
        // S_AXI_HP2_FPD (HP2) -> 1100, AXI ID [5:0] from PL
        // cell_master_id = 0x0300;    // 0000 00(11 0000 0000)  to do: take from cell configuration
        // cell_master_mask = 0x03C0;  // 0000 00(11 1100 0000)  to do: take from cell configuration    
      }
      else if(region == 2){
        // All that is coming from TBU 5
        cell_master_id = 0x8000;    // 1000 00(XX XXXX XXXX)  to do: take from cell configuration
        cell_master_mask = 0xFC00;  // 1111 11(00 0000 0000)  to do: take from cell configuration       

        // S_AXI_HP3_FPD (HP3) -> 1101, AXI ID [5:0] from PL
        // cell_master_id = 0x0340;    // 0000 00(11 0100 0000)  to do: take from cell configuration
        // cell_master_mask = 0x03C0;  // 0000 00(11 1100 0000)  to do: take from cell configuration    
      }
      else{
        xmpu_print("Error: FPGA region doesn't exist\r\n");
        break;
      }

      xmpu_print("FPGA region permissions settings ...\n\r");
      set_cell_permissions(xmpu_channel_n, xmpu_base, cell_master_id, cell_master_mask, cell);
    }
  }
  else{
    xmpu_print("No FPGA regions in this cell\n\r");
  }

  // If there is at least one rCPU in the configuration give the requested accesses
  // For each rCPU in the cell configuration Set the XMPU region registers
  if(cell->config->rcpu_set_size != 0){
    // To do: take from cell configuration
    for_each_cpu(cpu, cell->rcpu_set) {
		  if(cpu == 0){           // RPU 0
        xmpu_channel_n = 0;
        xmpu_base = XMPU_DDR_BASE_ADDR;
        // RPU (0000, 00, AXI ID[3:0])
        cell_master_id = 0x0000;    // 0000 00(00 0000 0000)  to do: take from cell configuration
        cell_master_mask = 0x03F0;  // 0000 00(11 1110 0000)  to do: take from cell configuration 
      }
		  else if(cpu == 1){      // RPU 1
        xmpu_channel_n = 0;
        xmpu_base = XMPU_DDR_BASE_ADDR;
        // RPU (0000, 01, AXI ID[3:0])
        cell_master_id = 0x0010;    // 0000 00(00 0001 0000)  to do: take from cell configuration
        cell_master_mask = 0x03F0;  // 0000 00(11 1111 0000)  to do: take from cell configuration 
      }
		  else{
			  // xmpu_print("Error: rCPU doesn't exist\r\n");
        // soft-core
        continue;
      }
      // DEBUG
	  	xmpu_print("rCPU %d permissions settings ...\n\r", cpu);
      set_cell_permissions(xmpu_channel_n, xmpu_base, cell_master_id, cell_master_mask, cell);
    }
  }
  else{
    xmpu_print("No rCPUs in this cell\n\r");
  }

  return 0;
}

// Config the XMPU to its default values
static void arm_xmpu_shutdown(void){
  u8 i = 0;
  u32 xmpu_base = 0;
  //xmpu_print("Shutting down XMPU\n\r");

  // Set to default values
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base=XMPU_DDR_BASE_ADDR;
    set_xmpu_default(xmpu_base + (i*XMPU_DDR_OFFSET));
  }
  xmpu_base=XMPU_FPD_BASE_ADDR;
  set_xmpu_default(xmpu_base);
  
  // DEBUG
  // for(i=0 ; i<NR_XMPU_DDR; i++){
  //   xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET); 
  //   //xmpu_print("DDR channel %d\n\r", i);
  //   print_xmpu_status_regs(xmpu_base);
  // }
  // //xmpu_print("FPD channel 0\n\r");
  // print_xmpu_status_regs(XMPU_FPD_BASE_ADDR);
  // //xmpu_print("OCM channel 0\n\r");
  // print_xmpu_status_regs(XMPU_OCM_BASE_ADDR); 
}

// DDR XMPU init
static void xmpu_ddr_init(void){
  u8 i,j = 0;
  u8 xmpu_channel_n = 0;
  u32 xmpu_base = 0;
  
  // Configure all XMPU ddr registers to default values
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET);
    set_xmpu_default(xmpu_base);
  } 

  // Configure XMPU region registers for all ddr the channels
  // TODO: Daniele Ottaviano
  // Give RPUs access to the DDR memory (these are part of the rootcell at the beginning) ... 
  for(j=0; j<NR_XMPU_DDR; j++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (j*XMPU_DDR_OFFSET);  
    xmpu_channel_n = j;               
    // Configure all the XMPU regions to be usable (used = 0) by the rootcell (id = 0)
    for(i = 0; i<NR_XMPU_REGIONS; i++){
      ddr_xmpu_device[xmpu_channel_n].region[i].used = 0;
      ddr_xmpu_device[xmpu_channel_n].region[i].id = 0;
    }
    // Configure region channels
    if( xmpu_channel_n==1 || xmpu_channel_n==2) {
      // SATA, GPU, DAP AXI CoreSight, PCIe
      // ( 0011, 0x, xxxx  )
      ddr_xmpu_device[xmpu_channel_n].region[0].addr_start =    0x0000000000;
      ddr_xmpu_device[xmpu_channel_n].region[0].addr_end =      0xFFFFF00000;
      ddr_xmpu_device[xmpu_channel_n].region[0].master_id =     0x00C0; //0000 0000 1100 0000
      ddr_xmpu_device[xmpu_channel_n].region[0].master_mask =   0x03E0; //0000 0011 1110 0000
      ddr_xmpu_device[xmpu_channel_n].region[0].ns_checktype =  0;
      ddr_xmpu_device[xmpu_channel_n].region[0].region_ns =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].wrallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].rdallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].enable =        1;
      ddr_xmpu_device[xmpu_channel_n].region[0].used =          1;
      ddr_xmpu_device[xmpu_channel_n].region[0].id =            0;
      set_xmpu_region(xmpu_base, R00_OFFSET, &ddr_xmpu_device[xmpu_channel_n].region[0]);

      // PMU processor, CSU processor, CSU-DMA, USB0, USB1, DAP APB control,
      // LPD DMA, SD0, SD1, NAND, QSPI, GEM0, GEM1, GEM2, GEM3 
      // ( 0001, xx, xxxx )
      ddr_xmpu_device[xmpu_channel_n].region[1].addr_start =    0x0000000000;
      ddr_xmpu_device[xmpu_channel_n].region[1].addr_end =      0xFFFFF00000;
      ddr_xmpu_device[xmpu_channel_n].region[1].master_id =     0x0040; // 0000 0000 0100 0000
      ddr_xmpu_device[xmpu_channel_n].region[1].master_mask =   0x03C0; // 0000 0011 1100 0000
      ddr_xmpu_device[xmpu_channel_n].region[1].ns_checktype =  0;
      ddr_xmpu_device[xmpu_channel_n].region[1].region_ns =     1;
      ddr_xmpu_device[xmpu_channel_n].region[1].wrallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[1].rdallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[1].enable =        1;
      ddr_xmpu_device[xmpu_channel_n].region[1].used =          1;
      ddr_xmpu_device[xmpu_channel_n].region[1].id =            0;
      set_xmpu_region(xmpu_base, R01_OFFSET, &ddr_xmpu_device[xmpu_channel_n].region[1]);

      // CCI-400, SMMU TCU 
      // (0000, 00, 0000)
      ddr_xmpu_device[xmpu_channel_n].region[2].addr_start =    0x0000000000;
      ddr_xmpu_device[xmpu_channel_n].region[2].addr_end =      0xFFFFF00000;
      ddr_xmpu_device[xmpu_channel_n].region[2].master_id =     0x0000; // 0000 0000 0000 0000 
      ddr_xmpu_device[xmpu_channel_n].region[2].master_mask =   0x03FF; // 0000 0011 1111 1111
      ddr_xmpu_device[xmpu_channel_n].region[2].ns_checktype =  0;
      ddr_xmpu_device[xmpu_channel_n].region[2].region_ns =     1;
      ddr_xmpu_device[xmpu_channel_n].region[2].wrallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[2].rdallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[2].enable =        1;
      ddr_xmpu_device[xmpu_channel_n].region[2].used =          1;
      ddr_xmpu_device[xmpu_channel_n].region[2].id =            0;
      set_xmpu_region(xmpu_base, R02_OFFSET, &ddr_xmpu_device[xmpu_channel_n].region[2]);

      // APU
      // ( 0010, AXI ID [5:0] )
      ddr_xmpu_device[xmpu_channel_n].region[3].addr_start =    0x0000000000;
      ddr_xmpu_device[xmpu_channel_n].region[3].addr_end =      0xFFFFF00000;
      ddr_xmpu_device[xmpu_channel_n].region[3].master_id =     0x0080; // 0000 0000 1000 0000
      ddr_xmpu_device[xmpu_channel_n].region[3].master_mask =   0x03C0; // 0000 0011 1100 0000
      ddr_xmpu_device[xmpu_channel_n].region[3].ns_checktype =  0;
      ddr_xmpu_device[xmpu_channel_n].region[3].region_ns =     1;
      ddr_xmpu_device[xmpu_channel_n].region[3].wrallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[3].rdallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[3].enable =        1;
      ddr_xmpu_device[xmpu_channel_n].region[3].used =          1;
      ddr_xmpu_device[xmpu_channel_n].region[3].id =            0;
      set_xmpu_region(xmpu_base, R03_OFFSET, &ddr_xmpu_device[xmpu_channel_n].region[3]);
    }
    else if( xmpu_channel_n==3 ){
      // DisplayPort ( 0011, 10, 0xxx DMA{0:5} )
      ddr_xmpu_device[xmpu_channel_n].region[0].addr_start =    0x0000000000;
      ddr_xmpu_device[xmpu_channel_n].region[0].addr_end =      0xFFFFF00000;
      ddr_xmpu_device[xmpu_channel_n].region[0].master_id =     0x00E0; //0000 0000 1110 0000
      ddr_xmpu_device[xmpu_channel_n].region[0].master_mask =   0x03F8; //0000 0011 1111 1000
      ddr_xmpu_device[xmpu_channel_n].region[0].ns_checktype =  0;
      ddr_xmpu_device[xmpu_channel_n].region[0].region_ns =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].wrallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].rdallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].enable =        1;
      ddr_xmpu_device[xmpu_channel_n].region[0].used =          1;
      ddr_xmpu_device[xmpu_channel_n].region[0].id =            0;
      set_xmpu_region(xmpu_base, R00_OFFSET, &ddr_xmpu_device[xmpu_channel_n].region[0]);        
    }
    else if( xmpu_channel_n==5 ){
      // FPD DMA ( 0011, 10, 1xxx CH{0:7} )
      ddr_xmpu_device[xmpu_channel_n].region[0].addr_start =    0x0000000000;
      ddr_xmpu_device[xmpu_channel_n].region[0].addr_end =      0xFFFFF00000;
      ddr_xmpu_device[xmpu_channel_n].region[0].master_id =     0x00E8; //0000 0000 1110 1000
      ddr_xmpu_device[xmpu_channel_n].region[0].master_mask =   0x03F8; //0000 0011 1111 1000
      ddr_xmpu_device[xmpu_channel_n].region[0].ns_checktype =  0;
      ddr_xmpu_device[xmpu_channel_n].region[0].region_ns =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].wrallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].rdallowed =     1;
      ddr_xmpu_device[xmpu_channel_n].region[0].enable =        1;
      ddr_xmpu_device[xmpu_channel_n].region[0].used =          1;
      ddr_xmpu_device[xmpu_channel_n].region[0].id =            0;
      set_xmpu_region(xmpu_base, R00_OFFSET, &ddr_xmpu_device[xmpu_channel_n].region[0]);    
    } 
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
  u8 xmpu_channel_n = 0;
  u32 xmpu_base = XMPU_FPD_BASE_ADDR;

  // Configure all XMPU FPD registers to default values
  set_xmpu_default(xmpu_base);

  // Configure XMPU FPD region registers
  // APU ( 0010, AXI ID [5:0] )
  fpd_xmpu_device.region[xmpu_channel_n].addr_start =    0x0000000000;
  fpd_xmpu_device.region[xmpu_channel_n].addr_end =      0xFFFFFFF000;
  fpd_xmpu_device.region[xmpu_channel_n].master_id =     0x0080;//0000 0000 1000 0000
  fpd_xmpu_device.region[xmpu_channel_n].master_mask =   0x03C0;//0000 0011 1100 0000
  fpd_xmpu_device.region[xmpu_channel_n].ns_checktype =  0;
  fpd_xmpu_device.region[xmpu_channel_n].region_ns =     1;
  fpd_xmpu_device.region[xmpu_channel_n].wrallowed =     1;
  fpd_xmpu_device.region[xmpu_channel_n].rdallowed =     1;
  fpd_xmpu_device.region[xmpu_channel_n].enable =        1;
  fpd_xmpu_device.region[xmpu_channel_n].used =          1;
  fpd_xmpu_device.region[xmpu_channel_n].id =            0;
  set_xmpu_region(xmpu_base, R00_OFFSET, &fpd_xmpu_device.region[xmpu_channel_n]);  
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
  u8 xmpu_channel_n = 0;
  u32 xmpu_base = XMPU_OCM_BASE_ADDR;

  // Configure all XMPU OCM registers to default values
  set_xmpu_default(xmpu_base);
  // Configure XMPU OCM region registers
  // APU ( 0010, AXI ID [5:0] )
  ocm_xmpu_device.region[xmpu_channel_n].addr_start =    0x00000000;
  ocm_xmpu_device.region[xmpu_channel_n].addr_end =      0xFFFFFFFF;
  ocm_xmpu_device.region[xmpu_channel_n].master_id =     0x0080;//0000 0000 1000 0000
  ocm_xmpu_device.region[xmpu_channel_n].master_mask =   0x03C0;//0000 0011 1100 0000
  ocm_xmpu_device.region[xmpu_channel_n].ns_checktype =  0;
  ocm_xmpu_device.region[xmpu_channel_n].region_ns =     1;
  ocm_xmpu_device.region[xmpu_channel_n].wrallowed =     1;
  ocm_xmpu_device.region[xmpu_channel_n].rdallowed =     1;
  ocm_xmpu_device.region[xmpu_channel_n].enable =        1;
  ocm_xmpu_device.region[xmpu_channel_n].used =          1;
  ocm_xmpu_device.region[xmpu_channel_n].id =            0;
  set_xmpu_region(xmpu_base, R00_OFFSET, &ocm_xmpu_device.region[xmpu_channel_n]);  
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
  // u8 i,j = 0;
  // u8 xmpu_channel_n = 0;
  // u32 xmpu_base = 0;
  // xmpu_print("Initializing XMPU devices\n\r");

  /* DDR */
  xmpu_ddr_init();
  
  /* FPD */
  xmpu_fpd_init();
  
  /* OCM */
  xmpu_ocm_init();

  // DEBUG
  // for(i=0 ; i<NR_XMPU_DDR; i++){
  //   xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET); 
  //   //xmpu_print("DDR channel %d\n\r", i);
  //   print_xmpu_status_regs(xmpu_base);
  // }
  // //xmpu_print("FPD channel 0\n\r");
  // print_xmpu_status_regs(XMPU_FPD_BASE_ADDR);
  // //xmpu_print("OCM channel 0\n\r");
  // print_xmpu_status_regs(XMPU_OCM_BASE_ADDR); 
  
  return 0;
}

DEFINE_UNIT_MMIO_COUNT_REGIONS_STUB(arm_xmpu);
DEFINE_UNIT(arm_xmpu, "ARM XMPU");
#endif /* CONFIG_XMPU && CONFIG_MACH_ZYNQMP_ZCU102 */ 