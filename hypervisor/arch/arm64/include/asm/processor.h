/*
 * Jailhouse AArch64 support
 *
 * Copyright (C) 2015 Huawei Technologies Duesseldorf GmbH
 *
 * Authors:
 *  Antonios Motakis <antonios.motakis@huawei.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_ASM_PROCESSOR_H
#define _JAILHOUSE_ASM_PROCESSOR_H

#include <jailhouse/types.h>

/* also include from arm-common */
#include_next <asm/processor.h>

#define NUM_USR_REGS		31

#define ARM_PARKING_CODE		\
	0xd503207f, /* 1: wfi  */	\
	0x17ffffff, /*    b 1b */

#define ARM_DAIF_F	(1 << 6)
#define ARM_DAIF_I	(1 << 7)
#define ARM_DAIF_A	(1 << 8)
#define ARM_DAIF_D	(1 << 9)

#ifndef __ASSEMBLY__

union registers {
	struct {
		/*
		 * We have an odd number of registers, and the stack needs to
		 * be aligned after pushing all registers. Add 64 bit padding
		 * at the beginning.
		 */
		unsigned long __padding;
		unsigned long usr[NUM_USR_REGS];
	};
};

/* ARMv8 wfe, v7 may require a dsb */
#define arm_wfe()	asm volatile("wfe" : : : "memory")

static inline unsigned long arm_irq_get_state(void)
{
	unsigned long flags;

	asm volatile("mrs %0, daif" : "=&r"(flags) : : "memory");

	return flags;
}

static inline bool arm_is_irq_off(void)
{
	return arm_irq_get_state() & ARM_DAIF_I;
}

static inline unsigned long arm_irq_save(void)
{
	unsigned long flags = arm_irq_get_state();

	asm volatile("msr daifset, #0x2" : : : "memory");

	return flags;
}

static inline void arm_irq_restore(unsigned long flags)
{
	asm volatile("msr daif, %0" : : "r"(flags) : "memory");
}

#endif /* !__ASSEMBLY__ */

#endif /* !_JAILHOUSE_ASM_PROCESSOR_H */
