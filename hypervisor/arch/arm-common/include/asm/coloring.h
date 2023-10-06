/*
 * Jailhouse Cache Coloring Support - Stubs for ARMv7
 *
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See the
 * COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_ASM_COLORING_H
#define _JAILHOUSE_ASM_COLORING_H

#include <jailhouse/utils.h>
#include <jailhouse/control.h>

static inline void arch_color_dyncolor_flush(void)
{
	return;
}

static inline int color_copy_root(struct cell *root, bool init)
{
	return -EINVAL;
}

static inline void arm_color_dcache_flush_memory_region(
	unsigned long phys,
	unsigned long size,
	unsigned long virt,
	u64 color_mask,
	enum dcache_flush flush_type)
{
	BUG();
}

static inline int
color_paging_create(const struct paging_structures *pg_structs,
		    unsigned long phys, unsigned long size, unsigned long virt,
		    unsigned long access_flags, unsigned long paging_flags,
		    u64 color_mask, u64 mem_flags)
{
	return -EINVAL;
}

static inline int
color_paging_destroy(const struct paging_structures *pg_structs,
		     unsigned long phys, unsigned long size, unsigned long virt,
		     unsigned long paging_flags, u64 color_mask, u64 mem_flags)
{
	return -EINVAL;
}

#endif
