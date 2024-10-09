#ifndef _JAILHOUSE_DRIVER_FPGA_H
#define _JAILHOUSE_DRIVER_FPGA_H

#include "jailhouse.h"
#include <linux/kobject.h>
#include <linux/fpga/fpga_mgr.h>

struct region {
	struct kobject kobj;
	unsigned int id;
	char name[JAILHOUSE_CELL_ID_NAMELEN+1];
    u64 status;
    enum fpga_mgr_states state;	
};

int jailhouse_fpga_create(struct jailhouse_fpga_create* fpga_create_args);
int jailhouse_fpga_load(struct jailhouse_fpga_load* fpga_load_args);

#endif /* _JAILHOUSE_DRIVER_FPGA */