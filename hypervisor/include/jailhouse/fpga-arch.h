#ifndef _JAILHOUSE_FPGA_H
#define _JAILHOUSE_FPGA_H

#include <jailhouse/fpga-mgr.h>

int arch_fpga_init(struct fpga_manager* mgr);
int arch_fpga_write_init(struct fpga_manager* mgr, struct fpga_image_info *info,
				    const char *buf, size_t size);

int arch_fpga_write_complete(struct fpga_manager* mgr, struct fpga_image_info *info);
int arch_fpga_write(struct fpga_manager *mgr, const char *buf, size_t count);
size_t arch_fpga_initial_header_size(void);


/*TODO
enum fpga_mgr_states arch_fpga_state(struct fpga_manager* mgr);
u64 arch_fpga_status(struct fpga_manager* mgr);
arch_fpga_read(struct fpga_manager* mgr,char* buf);
...read configuration registers??
*/
#endif /*_JAILHOUSE_ZYNQMP_FPGA_H*/
