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
#include <jailhouse/percpu.h>
#include <jailhouse/paging.h>
#include <jailhouse/control.h>
#include <jailhouse/cell-config.h>
#include <jailhouse/assert.h>
#include <jailhouse/memguard-common.h>
#include <jailhouse/memguard.h>
#include <asm/memguard.h>
#include <asm/gic_v2.h>
#include <asm/timer.h>

#ifdef CONFIG_DEBUG
static inline void memguard_print_priorities(void)
{
	u32 inum = system_config->platform_info.memguard.num_irqs;
	u32 prio;
	for (unsigned int i = 0; i < inum; i++) {
		prio = gicv2_get_prio(i);
		mg_print("%3d %02x\n", i, prio);
	}
	prio = gicv2_get_cpu_prio_mask();
	mg_print("mask: 0x%08x\n", prio);
}

static inline void memguard_dump_timer_regs(void)
{
	/* This function dumps the configuration of the timer registers */
	u64 reg;
	arm_read_sysreg(CNTPCT_EL0, reg);
	mg_print("CNT: %lld\n", reg);

	arm_read_sysreg(CNTHP_CVAL_EL2, reg);
	mg_print("CMP: %lld\n", reg);

	arm_read_sysreg(CNTHP_CTL_EL2, reg);
	mg_print("CTL: %lld\n", reg);
}
#endif

/* GICv2: 1024 = 1020 max + 3 special */
static u8 old_p[1024] = {0};

/**
 * Give max priority to the memguard timer interrupt, max + step to the PMU
 * interrupts, and decrease all other priorities which are lower than the
 * threshold. Never give idle (always masked) priority.
 */
static inline void adjust_prio(unsigned int low, unsigned int high)
{
	u32 prio;
	const struct jailhouse_memguard_config *mconf;
	mconf = &system_config->platform_info.memguard;

	assert(mconf->irq_prio_min - mconf->irq_prio_step > 0);
	for (unsigned int i = low; i < high; i++) {
		prio = gicv2_get_prio(i);

		while ((prio < mconf->irq_prio_threshold) &&
			(prio < (u32)(mconf->irq_prio_min -
				      mconf->irq_prio_step))) {
			prio += mconf->irq_prio_step;
		}

		gicv2_set_prio(i, prio);
	}
}

/**
 * Save old priorities and initialize SPI priorities. SPI IPRIORITYn are
 * not banked.
 */
static void memguard_init_prio(void)
{
	unsigned int i;
	u32 prio;
	const struct jailhouse_memguard_config *mconf;
	mconf = &system_config->platform_info.memguard;

	assert(mconf->irq_prio_min - mconf->irq_prio_step > 0);
	for (i = 0; i < mconf->num_irqs; i++) {
		prio = gicv2_get_prio(i);
		old_p[i] = prio;
	}

	/* Update priorities for SPI interrupts */
	adjust_prio(32, mconf->num_irqs);

	/* One PMU interrupt per CPU */
	assert(mconf->num_pmu_irq <= JAILHOUSE_MAX_PMU2CPU_IRQ);
	for (i = 0; i < mconf->num_pmu_irq; i++) {
		assert(is_spi(mconf->pmu_cpu_irq[i]));
		gicv2_set_prio(mconf->pmu_cpu_irq[i],
			       mconf->irq_prio_max + mconf->irq_prio_step);
	}
}

static void memguard_cpu_init_prio(void)
{
	const struct jailhouse_memguard_config *mconf;
	mconf = &system_config->platform_info.memguard;

	/* Update priorities for SGI and PPI interrupts */
	adjust_prio(0, 32);

	/* Update prio for hv timer */
	assert(is_ppi(mconf->hv_timer));

	gicv2_set_prio(mconf->hv_timer, mconf->irq_prio_max);
}

/* IRQChip and CPU Shutdown is called on each CPU. Jailhouse's irqchip
 * doesn't support a shutdown and a cpu_shutdown.
 */
static void memguard_cpu_restore_prio(void)
{
	const struct jailhouse_memguard_config *mconf;
	static volatile bool once = true;
	unsigned int i;

	for (i = 0; i < 32; i++) {
		gicv2_set_prio(i, old_p[i]);
	}

	// FIXME: CPUs are synchronized on shutdown_lock in hypervisor_disable
	// before returning from HVC. Here we're on the cleanup before
	// returning to EL1. No synchronization. This should be atomic.
	if (once) {
		once = false;
		mconf = &system_config->platform_info.memguard;

		for (i = 32; i < mconf->num_irqs; i++) {
			gicv2_set_prio(i, old_p[i]);
		}
	}
}

/**
 * Memguard timer interrupt: reset budgets and unblock CPUs
 */
bool memguard_isr_timer(void)
{
	volatile struct memguard *memguard = &this_cpu_public()->memguard;
	//u32 cntval = memguard_pmu_count();
	u32 cntval = 0;
#ifdef CONFIG_DEBUG
	u64 timval = timer_get_ticks();

	static u32 print_cnt = 0;
	if (print_cnt < 50) {
		printk("[%u, %u] _isr_tim p: %u t: %llu\n",
				this_cpu_id(),
				++print_cnt,
				cntval,
				timval);
		//memguard_dump_timer_regs();
	}
#endif
	memguard->last_time += memguard->budget_time;
	memguard->pmu_evt_cnt += memguard->budget_memory + 1 + cntval;
	timer_set_cmpval(memguard->last_time);
	//memguard_pmu_set_budget(memguard->budget_memory);
	ACCESS_ONCE(memguard->must_block) = 0;

	return true;
}

int memguard_init(void)
{
	int err;
	u32 irq = system_config->platform_info.memguard.hv_timer;

	/* register both irq line and interrupt handler */
	err = timer_register(irq, memguard_isr_timer);
	if (err < 0)
		return err;

	memguard_init_prio();
	return err;
}

void memguard_cpu_init(void)
{
	memguard_cpu_init_prio();
	timer_cpu_init();
	// INIT PMU
}

void memguard_cpu_shutdown(void)
{
	mg_print("(CPU %u) Shutdown\n", this_cpu_id());

	timer_cpu_shutdown();
	memguard_cpu_restore_prio();
	// shutdown pmu
}

/** Setup budget time + memory for this CPU. */
int memguard_set(struct memguard *memguard, unsigned long params_address)
{
	unsigned long params_page_offs = params_address & PAGE_OFFS_MASK;
	unsigned int params_pages;
	void *params_mapping;
	struct memguard_params *params;

	params_pages = PAGES(params_page_offs + sizeof(struct memguard_params));
	params_mapping = paging_get_guest_pages(NULL, params_address,
						params_pages,
						PAGE_READONLY_FLAGS);
	if (!params_mapping)
		return -ENOMEM;

	params = (struct memguard_params *)(params_mapping + params_page_offs);

	memguard->start_time = timer_get_ticks();
	memguard->last_time = memguard->start_time;
	memguard->pmu_evt_cnt = 0;

	memguard->budget_time = timer_us_to_ticks(params->budget_time);
	memguard->budget_memory = params->budget_memory;
	/* Ignore flags */
	memguard->flags = 0;
	/* FIXME: there's actually a race between updating a new budget
	 * and a previous budget for this CPU which expires. For the
	 * timer it doesn't matter because we unblock upon expiration,
	 * for the memory, it is more complex.
	 */
	memguard->must_block = false;
	timer_set_cmpval(memguard->last_time + memguard->budget_time);
	timer_enable();

	mg_print("(CPU %d) mg_set %llu %u %x\n", this_cpu_id(),
		 memguard->budget_time, memguard->budget_memory,
		 memguard->flags);

	return 0;
}
