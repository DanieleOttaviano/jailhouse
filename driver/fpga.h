/*
* Jailhouse, a Linux-based partitioning hypervisor
*
* Omnivisor remote core virtualization support
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

/* first mask is subset of second?*/
static inline int fpga_subset(u32 *src1, u32 *src2)
{	
	return (*src1 & *src2) == *src1;
}

/* unset in src2 bits that are set in src1*/
static inline void remove_regions_from_cell(u32 *src1, u32 *src2)
{
	*src2 &= ~(*src1);
}

/* set bits in src1 are set in src2 */
static inline void give_regions_to_cell(u32 *src1, u32 *src2)
{
	*src2 |= (*src1);
}

#ifdef CONFIG_OMNV_FPGA

void setup_fpga_flags(long flags);
int load_bitstream(struct cell *cell, struct jailhouse_preload_bitstream __user *bitstream);

#else /* !CONFIG_OMNV_FPGA */

static inline void setup_fpga_flags(long flags)
{
}

static inline int load_bitstream(struct cell *cell, struct jailhouse_preload_bitstream __user *bitstream)
{
    pr_err("ERROR: CONFIG_OMNV_FPGA is not set in Jailhouse\n");
    return -1;
}

#endif /* CONFIG_OMNV_FPGA */
#endif /* _JAILHOUSE_FPGA_H */