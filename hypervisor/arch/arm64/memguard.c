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
#include <asm/pmu_events.h>
#include <asm/pmu.h>

//#define MG_VERBOSE_DEBUG

/*
 * Protocol to interact with the pmu/timer:
 * - here only do enable/disable
 * - masking / unmasking and IRQ setup done by pmu, timer subsystems
 *   as part of the init/deinit phases.
 */

/* Memguard PMU Counter. We currently use only one counter, this is equivalent
 * to the pmu_first_cnt in pmu.c.
 * TODO: (needed?) extend to use more PMU counters at once.
 */
static u32 memguard_pmu_cnt = 0;

/* GICv2: 1024 = 1020 max + 3 special */
static u8 old_p[1024] = {0};

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

static void memguard_isr_debug_print(const char* name)
{
#if 0
	static u32 print_cnt[4] = {0, 0, 0, 0};
	static u32 count = 0;

	u64 now = timer_get_ticks();
	u32 pmu_cnt = pmu_get_val(memguard_pmu_cnt);

	if (count++ % 1000000) {
		printk("[%u, %u] _isr_%s p: %u t: %llu\n",
				this_cpu_id(),
				++print_cnt[this_cpu_id()],
				name,
				pmu_cnt,
				now);
	}
#else
	(void)name;
#endif
}

/**
 * Single counter, memory budget case: transform budget
 * to generate overflow upon expiration.
 */
static inline void memguard_set_pmu_budget(u32 budget)
{
	/* UINT32_MAX */
	u32 val = 0xffffffff;
	val -= budget;

	pmu_set_val(memguard_pmu_cnt, val);
}

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
	static bool once = true;
	unsigned int i;

	for (i = 0; i < 32; i++) {
		gicv2_set_prio(i, old_p[i]);
	}

	if (atomic_cas(&once, true, false) != true) {
		return;
	}

	assert(once == false);
	mconf = &system_config->platform_info.memguard;
	for (i = 32; i < mconf->num_irqs; i++) {
		gicv2_set_prio(i, old_p[i]);
	}
}

/**
 * Memguard timer interrupt: reset budgets and unblock CPUs
 */
bool memguard_isr_timer(void)
{
	struct memguard *memguard = &this_cpu_public()->memguard;

	assert(arm_is_irq_off());
	memguard_isr_debug_print("time");

	memguard->last_time += memguard->budget_time;
	/* Recharge budget */
	memguard_set_pmu_budget(memguard->budget_memory);
	/* Set next regulation period expiration */
	timer_set_cmpval(memguard->last_time);

	/* If we hit after a reset, remove the sticky reset flag */
#ifdef MG_VERBOSE_DEBUG
	if (memguard->block & MG_RESET) {
		printk("%uR\n", this_cpu_id());
	}

	if (memguard->block & MG_BLOCKED) {
		printk("%uU\n", this_cpu_id());
	}
#endif
	memguard->block &= ~MG_RESET;
	/* Disable blocking */
	memguard->block |= MG_UNBLOCK;

	return true;
}

/**
 * Memguard PMU interrupt: trigger asynchronous blocking for this CPU
 */
bool memguard_isr_pmu(void)
{
	struct memguard *memguard = &this_cpu_public()->memguard;

	/* NOTE: both IRQ and FIQ are off here */
	assert(arm_is_irq_off());
	memguard_isr_debug_print("pmu");

	/* clear overflow, let the counter run */
	pmu_clear_overflow(memguard_pmu_cnt);
	/* Lazily signal that the CPU should block.
	 * Will be shortly enacted in the same IRQ-off block.
	 */
	memguard->block |= MG_BLOCK;

	return true;
}

void memguard_cpu_block(void)
{
	/* block is volatile and never set cross-CPU */
	struct memguard *memguard = &this_cpu_public()->memguard;
	unsigned long spsr, elr;
#ifdef MG_VERBOSE_DEBUG
	bool once = true;
#endif

	/* Not blocked, or already blocked */
	if (!(memguard->block & MG_BLOCK)) {
		return;
	}

	/* Must block till next regulation period or memguard reset */
	memguard->block |= MG_BLOCKED;
	memguard->block &= ~(MG_BLOCK | MG_UNBLOCK);


	/* NOTE: IRQs may not be immediately enabled, but irq_on and
	 * the wfe should take place "in-order", so we will eventually
	 * see the interrupt before disabling it again.
	 * If block has changed in the meanwhile, we could have
	 * as well kept irq off the whole time.
	 */
	while (!(memguard->block & (MG_UNBLOCK | MG_RESET))) {
#ifdef MG_VERBOSE_DEBUG
		if (once) {
			printk("%uB\n", this_cpu_id());
			once = false;
		}
#endif
		// XXX: bad emulation of an EL2 irq handler ...
		// not applied all over the places...
		arm_read_sysreg(ELR_EL2, elr);
		arm_read_sysreg(SPSR_EL2, spsr);
		asm volatile(
			"msr daifclr, #0x2\n\t"
			"isb\n\t"
			"yield\n\t"
			"msr daifset, #0x2\n\t"
			: : : "memory");
		arm_write_sysreg(ELR_EL2, elr);
		arm_write_sysreg(SPSR_EL2, spsr);
	}

#ifdef MG_VERBOSE_DEBUG
	if (!once) {
		if (memguard->block & MG_RESET) {
			printk("%ur\n", this_cpu_id());
		}
	}
	once = true;
#endif
	memguard->block &= ~(MG_BLOCKED | MG_UNBLOCK);
}

int memguard_init(void)
{
	u32 irq = system_config->platform_info.memguard.hv_timer;
	u32 num_cnt;
	int err;

	/* register both irq line and interrupt handler */
	err = timer_register(irq, memguard_isr_timer);
	if (err < 0)
		return err;

	memguard_init_prio();

	/* Currently register one single counter */
	num_cnt = 1;
	memguard_pmu_cnt = pmu_register(num_cnt, memguard_isr_pmu);

	mg_print("Using PMU counter: %u\n", memguard_pmu_cnt);

	return err;
}

void memguard_cpu_init(void)
{
	memset(&this_cpu_public()->memguard, 0, sizeof(struct memguard));

	memguard_cpu_init_prio();
	timer_cpu_init();
	pmu_cpu_init();
}

void memguard_cpu_shutdown(void)
{
	timer_cpu_shutdown();
	pmu_cpu_shutdown();
	memguard_cpu_restore_prio();
}

/**
 * Re-activate memguard interrupts since jailhouse disables them
 * when "resetting" a CPU upon cell operations (add/remove/restart)
 */
void memguard_cpu_reset(void)
{
	struct memguard *memguard = &this_cpu_public()->memguard;
	memguard->block |= MG_RESET;

	/* Jailhouse and Linux only play with SGI and PPIs. SPIs are
	 * untouched, and we don't need to expliclty restart the PMU
	 */
	timer_cpu_reset();
}

/** Setup budget time + memory for this CPU. */
int memguard_set(struct memguard *memguard, unsigned long params_address)
{
	unsigned long params_page_offs = params_address & PAGE_OFFS_MASK;
	unsigned int params_pages;
	void *params_mapping;
	struct memguard_params *params;
	unsigned int event_type;

	assert(arm_is_irq_off());

	params_pages = PAGES(params_page_offs + sizeof(struct memguard_params));
	params_mapping = paging_get_guest_pages(NULL, params_address,
						params_pages,
						PAGE_READONLY_FLAGS);
	if (!params_mapping)
		return -ENOMEM;

	params = (struct memguard_params *)(params_mapping + params_page_offs);

	memguard->start_time = timer_get_ticks();
	memguard->last_time = memguard->start_time;

	memguard->budget_time = timer_us_to_ticks(params->budget_time);
	memguard->budget_memory = params->budget_memory;
	event_type = params->event_type;
	if (event_type == 0) {
		/* Use default event type */
		event_type = PMUV3_PERFCTR_L2D_CACHE_REFILL;
	}

	/* NOTE: this function is called on each affected CPU.
	 * Serialization via IRQ off. Reset the overflow indicator anyway.
	 */
	memguard->block = 0;
	pmu_disable(memguard_pmu_cnt);
	pmu_clear_overflow(memguard_pmu_cnt);

	/* Init timer and PMU budgets. Here also set the pmu type */
	pmu_set_type(memguard_pmu_cnt, event_type);
	timer_set_cmpval(memguard->last_time + memguard->budget_time);
	memguard_set_pmu_budget(memguard->budget_memory);

	/* Enable timer and PMU */
	pmu_enable(memguard_pmu_cnt);
	timer_enable();

	mg_print("(CPU %d) mg_set %llu %u (0x%x) [freq: %ld]\n", this_cpu_id(),
		 memguard->budget_time, memguard->budget_memory,
		 event_type, timer_get_frequency());

	return 0;
}
