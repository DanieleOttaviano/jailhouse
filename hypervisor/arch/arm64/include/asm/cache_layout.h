/*
 * Jailhouse Low-level cache operations and cache-layout detection
 *
 * Copyright (C) Università di Modena e Reggio Emilia, 2018
 * Copyright (C) Boston University, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Luca Miccio <lucmiccio@gmail.com>
 *  Renato Mancuso (BU) <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See the
 * COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_ARM_CACHE_LAYOUT_H
#define _JAILHOUSE_ARM_CACHE_LAYOUT_H

#include <jailhouse/printk.h>
#include <asm/bitops.h>

#define ARM_TLB_INVAL_ALL_EL(x) \
	asm volatile(	\
			"tlbi alle" #x "\n"\
			"dsb ish\n" \
			"isb\n" \
			: : : "memory")

static inline void arm_inval_icache(void)
{
	asm volatile("ic iallu" ::: "memory");
	dsb(ish);
	isb();
}

#ifdef CONFIG_DEBUG
#define verb_print(fmt, ...)			\
	printk("[COL] " fmt, ##__VA_ARGS__)

#define MAX_CACHE_LEVEL		7

typedef struct cache {
	/* Total size of the cache in bytes */
	u64 size;
	/* Size of a single way in bytes */
	u64 line_size;
	/* Size of each cache line in bytes */
	u64 way_size;
	/* Associativity */
	u32 assoc;
	/* Number of sets */
	u32 sets;
	/* Max number of colors supported by this cache */
	u64 colors;
	/* Which level is this cache at (cache level = level - 1)*/
	int level;
} cache_t;

/** Autodetect cache(s) geometry.
 *  Return the size of a way or 0 if no cache was detected.
 */
extern u64 arm_cache_layout_detect(void);

extern cache_t cache[];

/** Perform a per-way clean + invalidate (flush) on the x-cache level. */
void arm_flush_dcache_lx_per_way(unsigned int level);
#endif

#endif
