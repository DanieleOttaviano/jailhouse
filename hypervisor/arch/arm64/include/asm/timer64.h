/*
 * Hypervisor timer for Jailhouse
 *
 * Copyright (c) Università di Modena e Reggio Emilia, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Author:
 *  Luca Miccio <lucmiccio@gmail.com>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ARM64_TIMER_H
#define _ARM64_TIMER_H

#include <jailhouse/types.h>
#include <jailhouse/assert.h>
#include <jailhouse/control.h>
#include <jailhouse/cell.h>
#include <asm/processor.h>
#include <asm/sysregs.h>
#include <asm/gic_v2.h>
#include <asm/gic_v3.h>

#define CNTHP_CTL_EL2_ENABLE	(1<<0)
#define CNTHP_CTL_EL2_IMASK	(1<<1)
#define CNTHP_CTL_EL2_ISTATUS	(1<<2)

extern bool timer_isr_handler(void);
extern unsigned int hv_timer_irq;

static inline unsigned long timer_get_frequency(void)
{
	unsigned long freq;

	arm_read_sysreg(CNTFRQ_EL0, freq);
	return freq;
}

static inline u64 timer_get_ticks(void)
{
	u64 pct64;

	isb();
	arm_read_sysreg(CNTPCT_EL0, pct64);
	return pct64;
}

static inline void timer_set_cmpval(u64 cmp)
{
	arm_write_sysreg(CNTHP_CVAL_EL2, cmp);
	isb();
}

static inline void timer_unmask(void)
{
	u32 reg;
	/* NOTE: Enable + Unmasked + counter > timer
	 * --> interrupt.
	 */
	arm_read_sysreg(CNTHP_CTL_EL2, reg);
	reg &= ~CNTHP_CTL_EL2_IMASK;
	arm_write_sysreg(CNTHP_CTL_EL2, reg);
}

static inline void timer_mask(void)
{
	u32 reg;
	arm_read_sysreg(CNTHP_CTL_EL2, reg);
	reg |= CNTHP_CTL_EL2_IMASK;
	arm_write_sysreg(CNTHP_CTL_EL2, reg);
}

static inline bool timer_fired(void)
{
	u32 reg;
	arm_read_sysreg(CNTHP_CTL_EL2, reg);
	return ((reg & CNTHP_CTL_EL2_ISTATUS) != 0);
}

static inline void timer_enable(void)
{
	u32 reg;

	isb();

	arm_read_sysreg(CNTHP_CTL_EL2, reg);
	reg |= CNTHP_CTL_EL2_ENABLE;
	arm_write_sysreg(CNTHP_CTL_EL2, reg);
}

static inline void timer_disable(void)
{
	u32 reg;

	isb();

	arm_read_sysreg(CNTHP_CTL_EL2, reg);
	reg &= ~CNTHP_CTL_EL2_ENABLE;
	arm_write_sysreg(CNTHP_CTL_EL2, reg);
}

/** Re-enable hv_timer upon reset. Don't reset its value. */
static inline void timer_cpu_reset(void)
{
	/* Upon reset (create / destroy cells) JH disables all PPI
	 * except the maintenance IRQ, we re-enable the hv_timer
	 */
	if (system_config->platform_info.arm.gic_version == 3)
		gicv3_enable_irq(hv_timer_irq);
	else
		gicv2_enable_irq(hv_timer_irq);
}

/* -------------------------- FUNCTION DECLARATION ------------------------- */

extern u64 timer_us_to_ticks(u64 us);

/** Register timer irq and handler */
extern int timer_register(unsigned int irq, bool (*handler)(void));
/** Init of timer interrupt. After initialization, setting compare
 * and issuing a timer_enable() will let the timer fire upon expiration.
 */
extern void timer_cpu_init(void);
/** Cleanup */
extern void timer_cpu_shutdown(void);

#endif
