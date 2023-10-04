
#include "xmpu.h"

void set_xmpu_status(uint32_t xmpu_base, xmpu_status_config *config){
  *(XMPU_CTRL_REGISTER(xmpu_base))   = (0x00000000 | (config->align << 3) | (config->def_wr_allowed << 1) | (config->def_rd_allowed));
  *(XMPU_POISON_REGISTER(xmpu_base)) = (0x00000000 | (config->poison << 20));
  *(XMPU_LOCK_REGISTER(xmpu_base))   = (0x00000000 | (config->lock));
}

void set_xmpu_region(uint32_t xmpu_base, uint32_t region_offset, xmpu_region_config *config){
  uint32_t tmp_addr;
  uint32_t region_addr_start;
  uint32_t region_addr_end;

  tmp_addr = config->addr_start >> 20;
  region_addr_start = tmp_addr << 8;
  
  tmp_addr = config->addr_end >> 20;
  region_addr_end = tmp_addr << 8;

  *(XMPU_START_REGISTER(xmpu_base, region_offset))  = region_addr_start;
  *(XMPU_END_REGISTER(xmpu_base, region_offset))    = region_addr_end;
  *(XMPU_MASTER_REGISTER(xmpu_base, region_offset)) = (0x00000000 | (config->master_mask << 16) | (config->master_id));
  *(XMPU_CONFIG_REGISTER(xmpu_base, region_offset)) = (0x00000000 | (config->ns_checktype << 4) | (config->region_ns << 3) | (config->wrallowed << 2) | (config->rdallowed << 1) | (config->enable));
}

// Print the XMPU status registers
void print_xmpu_status_regs(uint32_t xmpu_base){
  printf("CTRL:        0x%08lX\n\r", *(XMPU_CTRL_REGISTER(xmpu_base)));
  printf("ERR_STATUS1: 0x%08lX\n\r", *(XMPU_ERR_STATUS1_REGISTER(xmpu_base)));
  printf("ERR_STATUS2: 0x%08lX\n\r", *(XMPU_ERR_STATUS2_REGISTER(xmpu_base)));
  printf("POISON:      0x%08lX\n\r", *(XMPU_POISON_REGISTER(xmpu_base)));
  printf("ISR:         0x%08lX\n\r", *(XMPU_ISR_REGISTER(xmpu_base)));
  printf("IMR:         0x%08lX\n\r", *(XMPU_IMR_REGISTER(xmpu_base)));
  printf("IEN:         0x%08lX\n\r", *(XMPU_IEN_REGISTER(xmpu_base)));
  printf("IDS:         0x%08lX\n\r", *(XMPU_IDS_REGISTER(xmpu_base)));
  printf("LOCK:        0x%08lX\n\r", *(XMPU_LOCK_REGISTER(xmpu_base)));
  printf("\n\r");
}

// Print the XMPU region registers
void print_xmpu_region_regs(uint32_t xmpu_base, uint32_t region){
  printf("R%02ld_START:   0x%08lX\n\r", region, *(XMPU_START_REGISTER(xmpu_base, region)));
  printf("R%02ld_END:     0x%08lX\n\r", region, *(XMPU_END_REGISTER(xmpu_base, region)));
  printf("R%02ld_MASTER:  0x%08lX\n\r", region, *(XMPU_MASTER_REGISTER(xmpu_base, region)));
  printf("R%02ld_CONFIG:  0x%08lX\n\r", region, *(XMPU_CONFIG_REGISTER(xmpu_base, region)));
  printf("\n\r");
}

// Print all the XMPU registers
void print_xmpu(uint32_t xmpu_base){
  print_xmpu_status_regs(xmpu_base);
  for (int i = 0; i < NR_XMPU_REGIONS; i++) {
    print_xmpu_region_regs(xmpu_base, i);
  }
}