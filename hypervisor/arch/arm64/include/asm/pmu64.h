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
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef _ARMV8_PMU_H
#define _ARMV8_PMU_H

#include <jailhouse/types.h>
#include <jailhouse/printk.h>
#include <asm/sysregs.h>
#include <asm/processor.h>

/*
 * Per-CPU PMCR: config reg
 */
#define PMCR_E		(1 << 0) /* Enable all counters */
#define PMCR_P		(1 << 1) /* Reset all counters */
#define PMCR_C		(1 << 2) /* Cycle counter reset */
#define PMCR_D		(1 << 3) /* CCNT counts every 64th cpu cycle */
#define PMCR_X		(1 << 4) /* Export to ETM */
#define PMCR_DP		(1 << 5) /* Disable CCNT if non-invasive debug*/
#define PMCR_LC		(1 << 6) /* Overflow on 64 bit cycle counter */
#define PMCR_LP		(1 << 7) /* Long event counter enable */
#define	PMCR_N_SHIFT	11	 /* Number of counters supported */
#define	PMCR_N_MASK	0x1f
#define	PMCR_MASK	0xff	 /* Mask for writable bits */

/* Reg def copied from kvm_arm.h */
/* Hyp Debug Configuration Register bits */
#define MDCR_EL2_TDRA		(1 << 11)
#define MDCR_EL2_TDOSA		(1 << 10)
#define MDCR_EL2_TDA		(1 << 9)
#define MDCR_EL2_TDE		(1 << 8)
#define MDCR_EL2_HPME		(1 << 7)
#define MDCR_EL2_TPM		(1 << 6)
#define MDCR_EL2_TPMCR		(1 << 5)
#define MDCR_EL2_HPMN_MASK	0x1f

#ifdef CONFIG_DEBUG
#define pmu_print(fmt, ...)			\
	printk("[PMU] " fmt, ##__VA_ARGS__)
#else
#define pmu_print(fmt, ...) do { } while (0)
#endif

static inline void pmu_disable_all(void)
{
	arm_write_sysreg(PMCR_EL0, 0);
}

static inline void pmu_enable_all(void)
{
	arm_write_sysreg(PMCR_EL0, (PMCR_E | PMCR_P | PMCR_C));
}

static inline void pmu_disable(u32 idx)
{
	arm_write_sysreg(PMCNTENCLR_EL0, (1 << idx));
}

static inline void pmu_enable(u32 idx)
{
	arm_write_sysreg(PMCNTENSET_EL0, (1 << idx));
}

static inline void pmu_int_disable(u32 idx)
{
	arm_write_sysreg(PMINTENCLR_EL1, (1 << idx));
}

static inline void pmu_int_enable(u32 idx)
{
	arm_write_sysreg(PMINTENSET_EL1, (1 << idx));
}

static inline void pmu_clear_overflow(u32 idx)
{
	arm_write_sysreg(PMOVSCLR_EL0, (1 << idx));
}

/** Get counter value for PMCNT[idx] */
static inline u32 pmu_get_val(u32 idx)
{
	u32 val;
	arm_write_sysreg(PMSELR_EL0, idx);
	isb();
	arm_read_sysreg(PMXEVCNTR_EL0, val);
	return val;
}

static inline void pmu_set_val(u32 idx, u32 val)
{
	arm_write_sysreg(PMSELR_EL0, idx);
	isb();
	arm_write_sysreg(PMXEVCNTR_EL0, val);
}

static inline u32 pmu_get_type(u32 idx)
{
	u32 type;
	arm_write_sysreg(PMSELR_EL0, idx);
	isb();
	arm_read_sysreg(PMXEVTYPER_EL0, type);
	return type;
}

static inline void pmu_set_type(u32 idx, u32 type)
{
	arm_write_sysreg(PMSELR_EL0, idx);
	isb();
	arm_write_sysreg(PMXEVTYPER_EL0, type);
}

/** Check if \a irqno is a PMU-related interrupt */
static inline bool irq_is_pmu(u32 irqno)
{
	const struct jailhouse_memguard_config *mconf;
	mconf = &system_config->platform_info.memguard;

	if (irqno == mconf->pmu_cpu_irq[this_cpu_id()]) {
		return true;
	}

	return false;
}

/** Re-enable the PMU interrupts on this CPU on restart.
 *  NOTE: Unused helper function: currently neither JH nor linux
 *  touch the SPIs, and they're not de-activated across CPU resets.
 */
static inline void pmu_cpu_reset(void)
{
	const struct jailhouse_memguard_config *mconf;
	u32 irq;

	mconf = &system_config->platform_info.memguard;
	irq = mconf->pmu_cpu_irq[this_cpu_id()];
	gicv2_enable_irq(irq);
}


/* ----------------------- FUNCTION DECLARATION ---------------------------- */
/** Init and shutdown */
extern void pmu_cpu_init(void);
extern void pmu_cpu_shutdown(void);

/** ISR handler */
extern bool pmu_isr_handler(void);

/**
 * Register the handler to be called upon PMU overflow iRQ.
 * Check availability for requested counters \a req_cnt and
 * store the fist counter that will be (currently) used
 * by default.
 *
 * @returns
 * 	- first (and currently only) counter to be used by the caller
 */
extern int pmu_register(u32 req_cnt, bool (*handler)(void));

#endif
