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

/* Platform specific ID mapping for SMC calls */
#if defined(__aarch64__) && defined(CONFIG_MACH_ZYNQMP_ZCU102)
#define SMC_FID_MASK        	0xff
#define SMC_RCPU_MASK		  	0xff
#define PM_POWERDOWN_RCPU   	0x08
#define PM_WAKEUP_RCPU      	0x0a
#define PM_FPGA_LOAD       		0x16
#define PM_FPGA_GET_STATUS		0x17

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
#define SMC_RCPU_MASK		  	0x00
#define PM_POWERDOWN_RCPU   	0xff
#define PM_WAKEUP_RCPU      	0xff
#define PM_FPGA_LOAD       		0xff
#define PM_FPGA_GET_STATUS		0xff

struct rcpu_map {
	unsigned int smc_val;
	unsigned int rcpu_id;
};

static const struct rcpu_map rcpu_table[] = {
	{ 0, -1 }, // Only sentinel
};

#endif


void enable_rcpu_start(unsigned int rcpu);
void enable_rcpu_load(unsigned int rcpu);
void enable_fpga_load(unsigned int cell_id);
int omnv_intercept_smc(struct trap_context *ctx);