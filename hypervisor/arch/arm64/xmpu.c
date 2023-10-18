
#include <jailhouse/printk.h>
#include <jailhouse/assert.h>
#include <asm/sysregs.h>
#include <asm/xmpu-board.h>
#include <asm/xmpu.h>

#ifdef CONFIG_DEBUG
#define xmpu_print(fmt, ...)			\
	printk("[XMPU] " fmt, ##__VA_ARGS__)
#else
#define xmpu_print(fmt, ...) do { } while (0)
#endif

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


void set_xmpu_status(u32 xmpu_base, xmpu_status_config *config){
  u32 xmpu_ctrl_register;
  u32 xmpu_poison_register;
  u32 xmpu_lock_register;
  bool poison;
 
  poison = (xmpu_base == XMPU_FPD_BASE_ADDR) ? !(config->poison) : config->poison; // to do add explaination ...

  if(xmpu_base == XMPU_FPD_BASE_ADDR){
    xmpu_ctrl_register	  = (0x00000000 | (config->align << 3) | (1<<2) | (config->def_wr_allowed << 1) | (config->def_rd_allowed));
  }
  else{
    xmpu_ctrl_register	  = (0x00000000 | (config->align << 3) | (0<<2) | (config->def_wr_allowed << 1) | (config->def_rd_allowed));
  }

  xmpu_poison_register	=	(0x00000000 | (poison << 20));
  xmpu_lock_register	  =	(0x00000000 | (config->lock));

  xmpu_write32((void *)(XMPU_CTRL_REGISTER(xmpu_base)), xmpu_ctrl_register);
  xmpu_write32((void *)(XMPU_POISON_REGISTER(xmpu_base)), xmpu_poison_register);
  xmpu_write32((void *)(XMPU_LOCK_REGISTER(xmpu_base)), xmpu_lock_register);
}

void set_xmpu_region(u32 xmpu_base, u32 region_offset, xmpu_region_config *config){
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


void set_xmpu_default(u32 xmpu_base){
  u32 xmpu_start_register   = 0x00000000;
  u32 xmpu_end_register     = 0x00000000;
  u32 xmpu_master_register  = 0x00000000;
  u32 xmpu_config_register  = 0x00000008; 
  u32 xmpu_ctrl_register    = 0x0000000b;
  u32 xmpu_poison_register  = 0x00000000;

  if(xmpu_base == XMPU_FPD_BASE_ADDR){
    xmpu_ctrl_register    = 0x00000007;
    xmpu_poison_register  = 0x000fd4f0;
  }
  
  xmpu_write32((void *)(XMPU_CTRL_REGISTER(xmpu_base)), xmpu_ctrl_register);
  xmpu_write32((void *)(XMPU_POISON_REGISTER(xmpu_base)), xmpu_poison_register);
  
  u32 region_offset = R00_OFFSET;
  xmpu_write32((void *)(XMPU_START_REGISTER(xmpu_base, region_offset)), xmpu_start_register);
  xmpu_write32((void *)(XMPU_END_REGISTER(xmpu_base, region_offset)), xmpu_end_register);
  xmpu_write32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region_offset)), xmpu_master_register);
  xmpu_write32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region_offset)), xmpu_config_register);

}

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
void print_xmpu_region_regs(u32 xmpu_base, u32 region){

  xmpu_print("START:   0x%08x\n\r", xmpu_read32((void *)(XMPU_START_REGISTER(xmpu_base, region))));
  xmpu_print("END:     0x%08x\n\r", xmpu_read32((void *)(XMPU_END_REGISTER(xmpu_base, region))));
  xmpu_print("MASTER:  0x%08x\n\r", xmpu_read32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region))));
  xmpu_print("CONFIG:  0x%08x\n\r", xmpu_read32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region))));
  xmpu_print("\n\r");
}

// Print all the XMPU registers
void print_xmpu(u32 xmpu_base){
  int i=0;
  print_xmpu_status_regs(xmpu_base);
  for (i = 0; i < NR_XMPU_REGIONS; i++) {
    print_xmpu_region_regs(xmpu_base, i);
  }
}