/*
 * Memguard for Jailhouse ARM64, GICv2
 *
 * Copyright (c) Czech Technical University in Prague, 2018
 * Copyright (C) Boston University, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Joel Matějka <matejjoe@fel.cvut.cz>
 *  Michal Sojka <michal.sojka@cvut.cz>
 *  Přemysl Houdek <houdepre@fel.cvut.cz>
 *  Renato Mancuso (BU) <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_MEMGUARD

#if defined(__aarch64__)
/*
 * TODO: memguard generic implementation as a unit, without strong integration
 * in the gicv2. init and shutdown functions could then be moved in the
 * unit.init, and in the irqchip.
 */
/** Register hv_timer handler, initialize SPI prio */
extern int memguard_init(void);
/** Initialize SGI/PPI prio, unmask timer, enable hv_timer IRQ */
extern void memguard_cpu_init(void);
/** Mask and disable timer, deregister handler, disable hv_timer IRQ */
extern void memguard_cpu_shutdown(void);

extern bool memguard_isr_timer(void);
#else
/* ARMv7 stubs */
static inline int memguard_init(void)
{
	return 0;
}

static inline void memguard_cpu_init(void)
{
	return;
}

static inline void memguard_cpu_shutdown(void)
{
	return;
}

static inline bool memguard_isr_timer(void)
{
	return true;
}
#endif

#endif
