
#include <jailhouse/printk.h>
#include <jailhouse/assert.h>
#include <jailhouse/unit.h>
#include <jailhouse/control.h>
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

static xmpu_channel ddr_xmpu_device[NR_XMPU_DDR];
static xmpu_channel fpd_xmpu_device;
//static xmpu_channel ocm_xmpu_device;


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

void set_xmpu_region_default(u32 xmpu_base, u32 region_offset){
  u32 xmpu_start_register   = 0x00000000;
  u32 xmpu_end_register     = 0x00000000;
  u32 xmpu_master_register  = 0x00000000;
  u32 xmpu_config_register  = 0x00000008; 

  xmpu_write32((void *)(XMPU_START_REGISTER(xmpu_base, region_offset)), xmpu_start_register);
  xmpu_write32((void *)(XMPU_END_REGISTER(xmpu_base, region_offset)), xmpu_end_register);
  xmpu_write32((void *)(XMPU_MASTER_REGISTER(xmpu_base, region_offset)), xmpu_master_register);
  xmpu_write32((void *)(XMPU_CONFIG_REGISTER(xmpu_base, region_offset)), xmpu_config_register);
}

void set_xmpu_status_default(u32 xmpu_base){
  u32 xmpu_ctrl_register    = 0;
  u32 xmpu_poison_register  = 0;
  u32 xmpu_lock_register    = 0;

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
    //not implemented
    xmpu_print("ERROR: OCM XMPU managment not implemented\n\r");
  }
  else{
    xmpu_print("ERROR: XMPU base address not valid\n\r");
  }
  
  xmpu_write32((void *)(XMPU_CTRL_REGISTER(xmpu_base)), xmpu_ctrl_register);
  xmpu_write32((void *)(XMPU_POISON_REGISTER(xmpu_base)), xmpu_poison_register);
  xmpu_write32((void *)(XMPU_LOCK_REGISTER(xmpu_base)), xmpu_lock_register); 
}


void set_xmpu_default(u32 xmpu_base){
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

static void arm_xmpu_cell_exit(struct cell *cell){
  u8 i = 0;
  u8 xmpu_channel_n = 0;
  u8 valid_reg_n = 0;
  u32 region_base = 0;
  u32 xmpu_base = 0;
  xmpu_print("Shutting down XMPU for cell %d\n\r", cell->config->id);

  xmpu_base = XMPU_DDR_BASE_ADDR; // IF RPU. To do: take from cell configuration 
  xmpu_channel_n = 0; // IF RPU. To do: take from cell configuration 
  
  if(cell->config->rcpu_set_size != 0){
    for(i = 0; i<NR_XMPU_REGIONS; i++){
      if(ddr_xmpu_device[xmpu_channel_n].region[i].id == cell->config->id){ 
        // clean region
        valid_reg_n = i;
        region_base = valid_reg_n * XMPU_REGION_OFFSET;
        set_xmpu_region_default(xmpu_base, region_base);
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].id = 0;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].used = 0;
      }
    }
  }
  else{
    xmpu_print("No rCPUs in this cell\n\r"); 
  }
}

// Cell init
static int arm_xmpu_cell_init(struct cell *cell){
  u32 xmpu_base = 0;
  u32 region_base = 0; 
  u32 addr_start, addr_end;
  u32 ddr_end_addr = 0x80000000;
  u8 i = 0;
  u8 valid_reg_n = 0;
  u8 xmpu_channel_n = 0;
  const struct jailhouse_memory *mem;
	unsigned int n;

  xmpu_print("Initializing XMPU for cell %d\n\r", cell->config->id);

  xmpu_base = XMPU_DDR_BASE_ADDR; // IF RPU. To do: take from cell configuration 
  xmpu_channel_n = 0; // IF RPU. To do: take from cell configuration

  // If there is at least one rCPU in the configuration
  if(cell->config->rcpu_set_size != 0){
    // check all the memory regions: Allow access to readable and writable regions in DDR and protect all the others
    for_each_mem_region(mem, cell->config, n){
      if(((mem->flags & JAILHOUSE_MEM_WRITE) || (mem->flags & JAILHOUSE_MEM_READ)) && (mem->virt_start <= ddr_end_addr - mem->size)){
        addr_start = mem->phys_start;
        addr_end =  mem->phys_start + mem->size - 1;
        
        //Configure XMPU0 to protect APU memory from RPU accesses	
        // Check for free region
        for(i = 0; i<NR_XMPU_REGIONS; i++){
          // if the id is 0, the region is free
          if(ddr_xmpu_device[xmpu_channel_n].region[i].used == 0){
            valid_reg_n = i; 
            break;
          }
        }
        if (i == NR_XMPU_REGIONS){
          xmpu_print("ERROR: no free region\n\r");
          return -1;
        }

        //DEBUG
        xmpu_print("Allowed memory region: 0x%08x - 0x%08x\n\r", addr_start, addr_end);
        xmpu_print("DDR Channel: %d, region: %d\n\r", xmpu_channel_n, valid_reg_n); 
        
        // Configure region
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].addr_start =     addr_start;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].addr_end =       addr_end;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].master_id =      0x0000;  // to do: take from cell configuration
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].master_mask =    0x03E0;  // to do: take from cell configuration
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].ns_checktype =   0;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].region_ns =      0;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].wrallowed =      (mem->flags & JAILHOUSE_MEM_WRITE) ? 1 : 0;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].rdallowed =      (mem->flags & JAILHOUSE_MEM_READ) ? 1 : 0;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].enable =         1;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].id =             cell->config->id;
        ddr_xmpu_device[xmpu_channel_n].region[valid_reg_n].used =           1;
        // Set XMPU registers region 
        region_base = valid_reg_n * XMPU_REGION_OFFSET;
        set_xmpu_region(xmpu_base, region_base, &ddr_xmpu_device[0].region[valid_reg_n]);  
      }
    }
  }
  else{
    xmpu_print("No rCPUs in this cell\n\r");
  }

  return 0;
}

// Shutdown the XMPU
static void arm_xmpu_shutdown(void){
  u8 i = 0;
  xmpu_print("Shutting down XMPU\n\r");

  // Set to default values
  for(i=0 ; i<NR_XMPU_DDR; i++){
    set_xmpu_default(XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET));
  }
  set_xmpu_default(XMPU_FPD_BASE_ADDR);
  
  // DEBUG
  // print_xmpu_status_regs(XMPU_DDR_0_BASE_ADDR);
  // print_xmpu_region_regs(XMPU_DDR_0_BASE_ADDR, 0);
}

// Initialize the XMPU
static int arm_xmpu_init(void){
  u8 i,j = 0;
  u8 xmpu_channel_n = 0;
  u32 xmpu_base = 0;

  xmpu_print("Initializing XMPU to his default values\n\r");

  /* DDR */

  // Set all ddr XMPU to default values
  for(i=0 ; i<NR_XMPU_DDR; i++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (i*XMPU_DDR_OFFSET);
    set_xmpu_default(xmpu_base);
  } 
  // For each processing element other than APUs: Block ddr accesses
  for(j=0; j<NR_XMPU_DDR; j++){
    xmpu_base = XMPU_DDR_BASE_ADDR + (j*XMPU_DDR_OFFSET);  
    xmpu_channel_n = j;               
    // Configure all the XMPU regions to be usable (used = 0)
    for(i = 0; i<NR_XMPU_REGIONS; i++){
      ddr_xmpu_device[xmpu_channel_n].region[i].used = 0;
    }
    if(j==0){ //RPU
     // Configure status register to block memory accesses to all others masters
      ddr_xmpu_device[xmpu_channel_n].status.poison =        1;
      ddr_xmpu_device[xmpu_channel_n].status.align =         1;
      ddr_xmpu_device[xmpu_channel_n].status.def_wr_allowed =0;
      ddr_xmpu_device[xmpu_channel_n].status.def_rd_allowed =0;
      ddr_xmpu_device[xmpu_channel_n].status.lock =          0;
      set_xmpu_status(xmpu_base, &ddr_xmpu_device[xmpu_channel_n].status); 
    }
    // TO DO ... (other masters)
  }

  /* FPD */

  // For each processing element other than APUs: Block fpd main switch accesses
  // Set to default values
  set_xmpu_default(XMPU_FPD_BASE_ADDR);
  
  // // Configure xmpu fpd to allow APU access and remove all the others
  fpd_xmpu_device.region[15].addr_start =    0x00000000;//0xFD000000;
  fpd_xmpu_device.region[15].addr_end =      0xFFFFFF00;//0xFF9CFF00;
  fpd_xmpu_device.region[15].master_id =     0x0080;//0x0000;
  fpd_xmpu_device.region[15].master_mask =   0x03C0;//0x03E0;
  fpd_xmpu_device.region[15].ns_checktype =  0;
  fpd_xmpu_device.region[15].region_ns =     1;//0;
  fpd_xmpu_device.region[15].wrallowed =     1;//0;
  fpd_xmpu_device.region[15].rdallowed =     1;//0;
  fpd_xmpu_device.region[15].enable =        1;
  // Set XMPU registers region
  set_xmpu_region(XMPU_FPD_BASE_ADDR, R15_OFFSET, &fpd_xmpu_device.region[15]);  

  //configure status
  fpd_xmpu_device.status.poison =        1;
  fpd_xmpu_device.status.align =         0; //4kb
  fpd_xmpu_device.status.def_wr_allowed =0;//1;
  fpd_xmpu_device.status.def_rd_allowed =0;//1;
  fpd_xmpu_device.status.lock =          0;
  // Set XMPU registers status
  set_xmpu_status(XMPU_FPD_BASE_ADDR, &fpd_xmpu_device.status);

  // DEBUG
  // print_xmpu_status_regs(XMPU_DDR_1_BASE_ADDR);
  // print_xmpu_region_regs(XMPU_DDR_1_BASE_ADDR, R15_OFFSET);
  // print_xmpu_status_regs(XMPU_DDR_2_BASE_ADDR);
  // print_xmpu_region_regs(XMPU_DDR_2_BASE_ADDR, R15_OFFSET);

  return 0;
}


DEFINE_UNIT_MMIO_COUNT_REGIONS_STUB(arm_xmpu);
DEFINE_UNIT(arm_xmpu, "ARM XMPU");