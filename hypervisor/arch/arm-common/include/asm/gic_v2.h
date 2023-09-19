/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_ASM_GIC_V2_H
#define _JAILHOUSE_ASM_GIC_V2_H

#include <asm/gic.h>

#define GICC_SIZE		0x2000
#define GICH_SIZE		0x2000

#define GICDv2_CIDR0		0xff0
#define GICDv2_PIDR0		0xfe0
#define GICDv2_PIDR2		0xfe8
#define GICDv2_PIDR4		0xfd0

#define GICC_CTLR		0x0000
#define GICC_PMR		0x0004
#define GICC_IAR		0x000c
#define GICC_EOIR		0x0010
#define GICC_DIR		0x1000

#define GICC_CTLR_GRPEN1	(1 << 0)
#define GICC_CTLR_EOImode	(1 << 9)

#define GICC_PMR_DEFAULT	0xf0

#define GICH_HCR		0x000
#define GICH_VTR		0x004
#define GICH_VMCR		0x008
#define GICH_ELSR0		0x030
#define GICH_ELSR1		0x034
#define GICH_APR		0x0f0
#define GICH_LR_BASE		0x100

#define GICV_PMR_SHIFT		3
#define GICH_VMCR_PMR_SHIFT	27
#define GICH_VMCR_EN0		(1 << 0)
#define GICH_VMCR_EN1		(1 << 1)
#define GICH_VMCR_ACKCtl	(1 << 2)
#define GICH_VMCR_EOImode	(1 << 9)

#define GICH_HCR_EN		(1 << 0)
#define GICH_HCR_UIE		(1 << 1)
#define GICH_HCR_LRENPIE	(1 << 2)
#define GICH_HCR_NPIE		(1 << 3)
#define GICH_HCR_VGRP0EIE	(1 << 4)
#define GICH_HCR_VGRP0DIE	(1 << 5)
#define GICH_HCR_VGRP1EIE	(1 << 6)
#define GICH_HCR_VGRP1DIE	(1 << 7)
#define GICH_HCR_EOICOUNT_SHIFT	27

#define GICH_LR_HW_BIT		(1 << 31)
#define GICH_LR_GRP1_BIT	(1 << 30)
#define GICH_LR_ACTIVE_BIT	(1 << 29)
#define GICH_LR_PENDING_BIT	(1 << 28)
#define GICH_LR_PRIORITY_SHIFT	23
#define GICH_LR_SGI_EOI_BIT	(1 << 19)
#define GICH_LR_CPUID_SHIFT	10
#define GICH_LR_PHYS_ID_SHIFT	10
#define GICH_LR_VIRT_ID_MASK	0x3ff

extern void *gicc_base;
extern void *gich_base;

/* set irq-related bit for bit-managed registers */
static inline void gicv2_set_bit(unsigned int base, unsigned int irq)
{
	mmio_write32(gicd_base + base + IRQ_BIT_REG_OFF(irq),
		     1 << IRQ_BIT_POS(irq));
}

static inline void gicv2_enable_irq(unsigned int irq)
{
	gicv2_set_bit(GICD_ISENABLER, irq);
}

static inline void gicv2_disable_irq(unsigned int irq)
{
	gicv2_set_bit(GICD_ICENABLER, irq);
}

/* Prio helpers */
static inline unsigned int gicv2_get_cpu_prio_mask(void)
{
	return mmio_read32(gicc_base + GICC_PMR);
}

static inline unsigned int gicv2_get_prio(unsigned int irq)
{
	return mmio_read8(gicd_base + GICD_IPRIORITYR + irq);
}

static inline void gicv2_set_prio(unsigned int irq, u8 prio)
{
	mmio_write8(gicd_base + GICD_IPRIORITYR + irq, prio);
}

/* Target CPUs helpers */
static inline unsigned int gicv2_get_targets(unsigned int irq)
{
	return mmio_read8(gicd_base + GICD_ITARGETSR + irq);
}

static inline void gicv2_set_targets(unsigned int irq, unsigned int targets)
{
	/* NOTE: each byte has a bitmap of the CPUs to targets */
	mmio_write8(gicd_base + GICD_ITARGETSR + irq, targets);
}

#endif /* _JAILHOUSE_ASM_GIC_V2_H */
