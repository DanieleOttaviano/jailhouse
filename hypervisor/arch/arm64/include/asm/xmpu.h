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

#ifndef _JAILHOUSE_ASM_XMPU_H
#define _JAILHOUSE_ASM_XMPU_H

#include <jailhouse/types.h>

#if defined(CONFIG_XMPU_ACTIVE) && defined(CONFIG_MACH_ZYNQMP_ZCU102)
//Debug Print
void print_xmpu_status_regs(u32 xmpu_base);
void print_xmpu_region_regs(u32 xmpu_base, u32 region);
void print_xmpu(u32 xmpu_base);

#endif /* CONFIG_XMPU_ACTIVE && CONFIG_MACH_ZYNQMP_ZCU102 */ 

#endif /* _JAILHOUSE_ASM_XMPU_H  */