/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (C) 2024 Daniele Ottaviano
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/config.h>
#include <asm/traps.h>


#if defined(__aarch64__) && defined(CONFIG_MACH_ZYNQMP_ZCU102)
#define SMC_FID_MASK        	0xff
#define PM_POWERDOWN_RCPU   	0x08
#define PM_WAKEUP_RCPU      	0x0a

struct rcpu_map {
	unsigned int smc_val;
	unsigned int rcpu_id;
};

static const struct rcpu_map rcpu_table[] = {
	{ 7, 0 },
	{ 8, 1 },
	{ 0, -1 }, // Sentinel
};

#else
#define SMC_FID_MASK        	0x00
#define PM_WAKEUP_RCPU      	0xff
#define PM_POWERDOWN_RCPU   	0xff

struct rcpu_map {
	unsigned int smc_val;
	unsigned int rcpu_id;
};

static const struct rcpu_map rcpu_table[] = {
	{ 0, -1 }, // Only sentinel
};

#endif


void enable_rcpu_start(unsigned int rcpu);
void disable_rcpu_start(unsigned int rcpu);
void enable_rcpu_load(void);
int omnv_intercept_smc(struct trap_context *ctx);