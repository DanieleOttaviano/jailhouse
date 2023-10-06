/*
 * ARM QoS Support for Jailhouse. Board-specific definitions.
 *
 * Copyright (c) Boston University, 2020
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *  Rohan Tabish <rtabish@illinois.edu>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 * See the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_ARM64_QOS_BOARD_H
#define _JAILHOUSE_ARM64_QOS_BOARD_H

#if defined(CONFIG_MACH_ZYNQMP_ZCU102)
/*
 * In the ZCU102, QoS registers require secure access. We must perform
 * an smc to a patched ATF to interact with them. Define read/write
 * functions accordingly.
 *
 * NOTE: Trying to modify the EL3 to make the QoS-related region available at
 * EL2 level (e.g., make the range [0xfd600000, 0xfd600000+0x200000)
 * accessible) resulted in abort errors on access. Rohan also tried to
 * "play" with the XMMU but without success. The SMC-call seems to be
 * the only way that reliably works.
 */

#include <asm/smc.h>
/*
 * Only support for FPD_GPV QoS regulators is currently available for
 * the ZCU102.
 *
 * The layout of the following regulators has been extracted from:
 * https://www.xilinx.com/html_docs/registers/ug1087/ug1087-zynq-ultrascale-registers.html
 *
 * NOTE: On the ZCU102, the FPD_GPV registers are accessible only from
 * EL3. In order for JH to be able to access these registers, the ATF
 * should be patched to allow read/write operations through the two
 * services defined below (ZCU102_QOS_READ_SMC and
 * ZCU102_QOS_WRITE_SMC)
 */

#define ZCU102_QOS_READ_SMC     0x8400ff04
#define ZCU102_QOS_WRITE_SMC    0x8400ff05

static inline u32 qos_read32(void *addr)
{
	int ret;
	ret = smc_arg1(ZCU102_QOS_READ_SMC, (unsigned long)(addr));
#ifdef CONFIG_DEBUG
	if (ret < 0)
		printk("QOS READ: %d\n", ret);
#endif
	return ret;
}

static inline void qos_write32(void *addr, u32 val)
{
	int ret __attribute__((unused));
	ret = smc_arg2(ZCU102_QOS_WRITE_SMC, (unsigned long)addr, val);
#ifdef CONFIG_DEBUG
	if (ret < 0)
		printk("QOS WRITE: %d\n", ret);
#endif
	return;
}

/* Since we are going to use SMC calls to access any of the QoS
 * registers, do no perform a real mapping but only provide a pointer
 * that reflects the linear 1:1 mapping done in the ATF. */
#define qos_map_device(base, size)	((void *)(base))

/* END -- CONFIG_MACH_ZYNQMP_ZCU102 */
#else

/* Other boards, currently only S32V: CONFIG_MACH_NXP_S32 */
#include <jailhouse/paging.h>

/* If the a platform-specific way to interact with QoS registers is
 * specified, use standard mmio functions */
static inline u32 qos_read32(void *addr)
{
	return mmio_read32(addr);
}

static inline void qos_write32(void *addr, u32 val)
{
	mmio_write32(addr, val);
	return;
}

/* Unless the QoS registers are not directly accessible from EL2, use
 * the stardard mapping to access the I/O registers */
#define qos_map_device(base, size)	paging_map_device(base, size);
// FIXME: implement the unmapping logic...

#endif

#endif
