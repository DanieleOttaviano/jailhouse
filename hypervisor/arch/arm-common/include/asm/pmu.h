/*
 * Hypervisor PMU for Jailhouse
 *
 * Copyright (C) Technical University of Munich, 2020
 *
 * Author:
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef ARM_PMU_H

#if defined(__aarch64__)
#include <asm/pmu64.h>
#else
static inline bool irq_is_pmu(u32 irqno)
{
	/* Don't manage pmu interrupts */
	return false;
}

static inline bool pmu_isr_handler(void)
{
	/* and if we end up here, just pretend all's good */
	return true;
}
#endif

#endif
