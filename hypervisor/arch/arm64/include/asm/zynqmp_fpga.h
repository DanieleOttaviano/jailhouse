#ifndef _JAILHOUSE_ZYNQMP_FPGA_H
#define _JAILHOUSE_ZYNQMP_FPGA_H

#include <jailhouse/fpga-mgr.h>

int zynqmp_fpga_init();
struct fpga_manager* get_fpga_manager();


#endif /*_JAILHOUSE_ZYNQMP_FPGA_H*/