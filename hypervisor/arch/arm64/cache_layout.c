/*
 * Jailhouse Cache Coloring Support
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
 * Autodetection of the cache geometry.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See the
 * COPYING file in the top-level directory.
 */
#include <jailhouse/utils.h>
#include <jailhouse/paging.h>
#include <asm/cache_layout.h>
#include <asm/sysregs.h>

#define CLIDR_CTYPE(reg, n)	GET_FIELD((reg), 3*(n)+2, 3*(n))
#define CLIDR_ICB(reg)		GET_FIELD((reg), 32, 30)

enum clidr_ctype {
	CLIDR_CTYPE_NOCACHE,
	CLIDR_CTYPE_IONLY,
	CLIDR_CTYPE_DONLY,
	CLIDR_CTYPE_IDSPLIT,
	CLIDR_CTYPE_UNIFIED,
};

#define CSSELR_LEVEL(reg)	SET_FIELD((reg), 3, 1)
#define CSSELR_IND		0x1

/* Assume ARM v8.0, v8.1, v8.2 */
#define CCSIDR_LINE_SIZE(reg)	GET_FIELD((reg), 2, 0)
#define CCSIDR_ASSOC(reg)	GET_FIELD((reg), 12, 3)
#define CCSIDR_NUM_SETS(reg)	GET_FIELD((reg), 27, 13)

const char * cache_types[] = {"Not present", "Instr. Only", "Data Only", "I+D Split", "Unified"};

cache_t cache[MAX_CACHE_LEVEL];

/** Autodetect cache(s) geometry.
 *  Return the size of a way or 0 if no cache was detected.
 */
u64 arm_cache_layout_detect(void)
{
	/* First, parse CLIDR_EL1 to understand how many levels are
	 * present in the system. */
	u64 reg, geom;
	unsigned int max_cache_level;

	unsigned int n;
	unsigned llc = 0;
	u64 type, assoc, ls, sets;

	arm_read_sysreg(clidr_el1, reg);

	max_cache_level = CLIDR_ICB(reg);
	if (max_cache_level == 0) {
		max_cache_level = MAX_CACHE_LEVEL;
		verb_print("\tUsing default max cache levels\n");
	}
	verb_print("\tmax cache level = %u\n", max_cache_level);

	for (n = 0; n < max_cache_level; ++n) {
		cache[n].way_size = 0;
		cache[n].level = -1;

		type = CLIDR_CTYPE(reg, n);
		verb_print("\tL%d Cache Type: %s\n", n + 1, cache_types[type]);

		if (type == CLIDR_CTYPE_NOCACHE)
			continue;

		/* Save the last seen available level of cache */
		llc = n;

		/* Fetch additional info about this cache level */
		arm_write_sysreg(csselr_el1, CSSELR_LEVEL(n));
		arm_read_sysreg(ccsidr_el1, geom);

		/* Parse info about this level */
		ls = 1 << (4 + CCSIDR_LINE_SIZE(geom));
		assoc = CCSIDR_ASSOC(geom) + 1;
		sets = CCSIDR_NUM_SETS(geom) + 1;

		/* Only keep track of dcache information */
		cache[n].level = n;

		cache[n].size = ls * assoc * sets;
		cache[n].line_size = ls;
		cache[n].way_size = ls * sets;
		cache[n].assoc = assoc;
		cache[n].sets = sets;
		cache[n].colors = sets / (PAGE_SIZE / ls);

		verb_print("\t\tTotal size: %lld\n", ls * assoc * sets);
		verb_print("\t\tLine size: %lld\n", ls);
		verb_print("\t\tAssoc.: %lld\n", assoc);
		verb_print("\t\tNum. sets: %lld\n", sets);

		if (type == CLIDR_CTYPE_IDSPLIT) {
			arm_write_sysreg(csselr_el1, (CSSELR_LEVEL(n) | CSSELR_IND));
			arm_read_sysreg(ccsidr_el1, geom);

			ls = 1 << (4 + CCSIDR_LINE_SIZE(geom));
			assoc = CCSIDR_ASSOC(geom) + 1;
			sets = CCSIDR_NUM_SETS(geom) + 1;

			verb_print("\t\tTotal size (I): %lld\n", ls * assoc * sets);
			verb_print("\t\tLine size (I): %lld\n", ls);
			verb_print("\t\tAssoc. (I): %lld\n", assoc);
			verb_print("\t\tNum. sets (I): %lld\n", sets);

		}

		/* Perform coloring at the last unified cache level */
		if (type == CLIDR_CTYPE_UNIFIED) {
			/* Compute the max. number of colors */
			verb_print("\t\tNum. colors: %lld\n", cache[n].colors);
		}

	}

	verb_print("\tNOTE: L%d Cache selected for coloring.\n",
		   cache[llc].level + 1);

	return cache[llc].way_size;
}

/** Perform a per-way clean + invalidate (flush) on the x-cache level. */
void arm_flush_dcache_lx_per_way(unsigned int level)
{
	/* index the cache level */
	unsigned int i = level - 1;

	/* Assume way and line size are power of 2 */
	unsigned int wshift = 32 - ffsl(cache[i].assoc);
	unsigned int sshift = ffsl(cache[i].line_size);
	u32 s;
	u32 w;
	u64 sw;

	dsb(ish);
	for (w = 0; w < cache[i].assoc; w++) {
		for (s = 0; s < cache[i].sets; s++) {
			sw = (u64)((w << wshift) | (s << sshift) | level);
			asm volatile("dc cisw, %0\n" : : "r" (sw) : "memory");
		}
	}
	dsb(ish);
}
