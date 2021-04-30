/*
 * PMU support for Memguard for Jailhouse ARM64, GICv2
 *
 * Copyright (c) Universita' di Modena e Reggio Emilia, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Luca Miccio <lucmiccio@gmail.com>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * Based on the initial work from the
 * Czech Technical University in Prague
 * (see memguard.c)
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <jailhouse/control.h>
#include <jailhouse/assert.h>
#include <jailhouse/panic.h>
#include <asm/gic_v2.h>
#include <asm/pmu.h>

static u32 pmu_first_cnt = 0;
static bool (*_pmu_isr_handler)(void) = NULL;

void pmu_cpu_init(void)
{
	const struct jailhouse_memguard_config *mconf;
	u32 mdcr;
	u32 irq;

	assert(pmu_first_cnt != 0);
	arm_read_sysreg(MDCR_EL2, mdcr);
	/* Clear HPMN */
	mdcr &= ~MDCR_EL2_HPMN_MASK;
	/* Set first cnt-number available to EL2+ only, and enable EL2 evt */
	mdcr |= (MDCR_EL2_HPME | pmu_first_cnt);
	arm_write_sysreg(MDCR_EL2, mdcr);

	/* disable counter and reset overflow */
	pmu_disable(pmu_first_cnt);
	pmu_int_disable(pmu_first_cnt);
	pmu_clear_overflow(pmu_first_cnt);

	/* Enable PMU IRQs for this CPU */
	mconf = &system_config->platform_info.memguard;
	assert(this_cpu_id() < mconf->num_pmu_irq);
	irq = mconf->pmu_cpu_irq[this_cpu_id()];

	/* NOTE: targets is a bitmap */
	gicv2_set_targets(irq, (1U << this_cpu_id()));
	pmu_print("irq %u, cpu %u, t 0x%x\n", irq, this_cpu_id(), gicv2_get_targets(irq));
	gicv2_enable_irq(irq);
	pmu_int_enable(pmu_first_cnt);

	/* Enable PMCCNTR_EL0 */
	pmu_enable(31);
	/* Allow PMU Events */
	pmu_enable_all();
}

void pmu_cpu_shutdown(void)
{
	const struct jailhouse_memguard_config *mconf;
	mconf = &system_config->platform_info.memguard;

	pmu_disable_all();
	pmu_disable(pmu_first_cnt);
	pmu_int_disable(pmu_first_cnt);
	gicv2_disable_irq(mconf->pmu_cpu_irq[this_cpu_id()]);

	if (ACCESS_ONCE(_pmu_isr_handler) != NULL) {
		_pmu_isr_handler = NULL;
	}
}

/**
 * Register the handler to be called upon PMU overflow iRQ.
 * Check availability for requested counters and
 * store the fist counter that will be (currently) used
 * by default.
 *
 * TODO: allow usage of more than one counter? Needed?
 */
int pmu_register(u32 req_cnt, bool (*handler)(void))
{
	u32 arch_cnt;

	/* Do the consistency checks at boot time on a CPU */
	arm_read_sysreg(PMCR_EL0, arch_cnt);
	arch_cnt = (arch_cnt >> PMCR_N_SHIFT) & PMCR_N_MASK;
	/* one PMU should be always left to the OS */
	if (req_cnt >= arch_cnt) {
		printk("PMU: Invalid num PMUs: requested: %u, available %u\n",
			req_cnt, arch_cnt - 1);
		panic_stop();
	}

	/* Save the first EL2+ reserved counter for later */
	pmu_first_cnt = arch_cnt - req_cnt;

	assert(_pmu_isr_handler == NULL);
	_pmu_isr_handler = handler;

	return pmu_first_cnt;
}

bool pmu_isr_handler(void)
{
	if (_pmu_isr_handler != NULL)
		return (*_pmu_isr_handler)();
	else
		return true;
}
