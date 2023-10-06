/*
 * Jailhouse AArch64 support
 *
 * Copyright (C) 2015 Huawei Technologies Duesseldorf GmbH
 *
 * Authors:
 *  Claudio Fontana <claudio.fontana@huawei.com>
 *  Antonios Motakis <antonios.motakis@huawei.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_ASM_BITOPS_H
#define _JAILHOUSE_ASM_BITOPS_H

/* also include from arm-common */
#include_next <asm/bitops.h>
#include <asm/processor.h>

static inline int atomic_test_and_set_bit(int nr, volatile unsigned long *addr)
{
	u32 ret;
	u64 test, tmp;

	/* word-align */
	addr = (unsigned long *)((u64)addr & ~0x7) + nr / BITS_PER_LONG;
	nr %= BITS_PER_LONG;


	/* AARCH64_TODO: using Inner Shareable DMB at the moment,
	 * revisit when we will deal with shareability domains */

	do {
		asm volatile (
			"ldxr	%3, %2\n\t"
			"ands	%1, %3, %4\n\t"
			"b.ne	1f\n\t"
			"orr	%3, %3, %4\n\t"
			"1:\n\t"
			"stxr	%w0, %3, %2\n\t"
			"dmb    ish\n\t"
			: "=&r" (ret), "=&r" (test),
			  "+Q" (*(volatile unsigned long *)addr),
			  "=&r" (tmp)
			: "r" (1ul << nr));
	} while (ret);
	return !!(test);
}

/*
 * Usage: if (atomic_cas(addr, old, new) != old): failure.
 * tmp = *addr;
 * if (tmp == old) *addr = new;
 * return tmp;
 *
 * acquire + release semantic, no spurious returns (see
 * https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html#g_t_005f_005fatomic-Builtins)
 */
static inline u32 atomic_cas(u32 *addr, u32 old, u32 new)
{
	u32 tmp, oldval;

	dmb(ish);
	asm volatile (
		"1: ldxr	%w1, [%2]\n\t"
		"subs		%w0, %w1, %w3\n\t"
		"b.ne		2f\n\t"
		"stxr		%w0, %w4, [%2]\n\t" /* 1 on failure */
		"cbnz		%w0, 1b\n\t"
		"2:\n\t"
		: "=&r"(tmp), "=&r"(oldval)
		: "r"(addr), "r"(old), "r"(new)
		: "memory", "cc");
	dmb(ish);

	return oldval;
}

#endif
