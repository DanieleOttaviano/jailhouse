/*
 * Jailhouse: ARM64 Hypervisor (CNTHP) Timer.
 * NOTE: Assumes GICv2 controller.
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
 *
 * Conversion functions from inmates/lib/arm-common/timing.c
 */
#include <jailhouse/mmio.h>
#include <jailhouse/printk.h>
#include <jailhouse/assert.h>
#include <asm/timer.h>
#include <asm/sysregs.h>
#include <asm/processor.h>
#include <asm/gic.h>
#include <asm/gic_v2.h>

/*
 * GICv2:
 * 	IRQ 0 - 31: banked for max 8 CPUs
 * 	ensure timer interrupt is PPI (16 - 31)
 * 	enable delivery of timer interrupt to CPU at init
 *
 * Timer management:
 * 	only install one timer interrupt handler
 * 	programming and unmasking on all CPUs
 */

/* Private timer ISR handler */
static bool (*_timer_isr_handler)(void) = NULL;
/* Timer interrupt */
static unsigned int hv_timer_irq = 0;

static unsigned long emul_division(u64 val, u64 div)
{
	unsigned long cnt = 0;

	while (val > div) {
		val -= div;
		cnt++;
	}
	return cnt;
}

u64 timer_ticks_to_ns(u64 ticks)
{
	return emul_division(ticks * 1000,
			     timer_get_frequency() / 1000 / 1000);
}

u64 timer_us_to_ticks(u64 us)
{
	/* NOTE: consider if some empiricaly rounding is needed.
	 * E.g., adjust value if freq is 999MHz instead of 1GHz.
	 */
	return (us * timer_get_frequency()) / 1000000;
}

int timer_register(unsigned int irq, bool (*handler)(void))
{
	if (!is_ppi(irq)) {
		printk("Expected PPI interrupt, got %u\n", irq);
		return -ENODEV;
	}

	assert(_timer_isr_handler == NULL);
	_timer_isr_handler = handler;
	hv_timer_irq = irq;

	return 0;
}

/** CPU shutdown, invoked when jailhouse is disabled */
void timer_cpu_shutdown(void)
{
	/* reset the HV timer, disable and mask it */
	timer_set_cmpval(0xffffffffffffffffULL);
	timer_mask();
	timer_disable();

	if (ACCESS_ONCE(_timer_isr_handler) != NULL) {
		_timer_isr_handler = NULL;
	}

	/* Disable distributor IRQ */
	gicv2_disable_irq(hv_timer_irq);
}

void timer_cpu_init(void)
{
	/* Approx 200 years from 0: UINT64_MAX */
	timer_set_cmpval(0xffffffffffffffffULL);
	timer_unmask();

	/* Enable IRQ in the distributor */
	gicv2_enable_irq(hv_timer_irq);

	/* NOTE: init completed, timer_enable() will
	 * trigger an interrupt if counter > timer
	 */
}

bool timer_isr_handler(void)
{
	if (_timer_isr_handler != NULL)
		return (*_timer_isr_handler)();
	else
		return true;
}
