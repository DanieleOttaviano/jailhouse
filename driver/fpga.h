/*
* Jailhouse, a Linux-based partitioning hypervisor
*
* Omnivisor FPGA regions virtualization support
*
* Copyright (c) Daniele Ottaviano, 2024
*
* Authors:
*   Daniele Ottaviano <danieleottaviano97@gmail.com>
*   Anna Amodio <anna.amodio@studenti.unina.it>
*
* This work is licensed under the terms of the GNU GPL, version 2.  See
* the COPYING file in the top-level directory.
*
*/
#ifndef _JAILHOUSE_FPGA_H
#define _JAILHOUSE_FPGA_H

#include "cell.h"


#define for_each_region(region, set) \
	for_each_cpu(region, set)

#ifdef CONFIG_OMNV_FPGA

int load_root_bitstream(char *bitstream_name, long flags);
int load_bitstream(unsigned int region_id,
			const char *bitstream);
int jailhouse_fpga_regions_setup(struct cell *cell, 
			const struct jailhouse_cell_desc *config);
int jailhouse_fpga_regions_remove(struct cell *cell);

#else /* !CONFIG_OMNV_FPGA */

static inline int load_root_bitstream(char *bitstream_name)
{
	return 0;
}

static inline int load_bitstream(unsigned int region_id,
			char *bitstream_name)
{
	pr_err("ERROR: CONFIG_OMNV_FPGA is not set in Jailhouse\n");
	return -1;
}

static inline int jailhouse_fpga_regions_setup(struct cell *cell, 
			const struct jailhouse_cell_desc *config)
{
	return 0;
}

static inline int jailhouse_fpga_regions_remove(struct cell *cell)
{
	return 0;
}

#endif /* CONFIG_OMNV_FPGA */
#endif /* _JAILHOUSE_FPGA_H */