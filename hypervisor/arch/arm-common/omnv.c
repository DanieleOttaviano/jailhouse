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

#include <jailhouse/control.h>
#include <jailhouse/printk.h>
#include <jailhouse/bitops.h>
#include <asm/omnv.h>
#include <asm/bitops.h>

static unsigned long rcpu_start_bitmap = 0;
static unsigned long load_phase = 0;

void enable_rcpu_start(unsigned int rcpu)
{
	set_bit(rcpu, &rcpu_start_bitmap);
}

void disable_rcpu_start(unsigned int rcpu)
{
	clear_bit(rcpu, &rcpu_start_bitmap);
}

void enable_rcpu_load(){
	load_phase = 1;
}

static inline void disable_rcpu_load(void)
{
	load_phase = 0;
}

static int get_rcpu_from_smc_arg(unsigned long val)
{
	for (int i = 0; rcpu_table[i].rcpu_id != (unsigned int)-1; ++i) {
		if (rcpu_table[i].smc_val == val)
			return rcpu_table[i].rcpu_id;
	}
	return -1; // Not found
}

/**
 * omnv_intercept_smc - Intercepts SMC (Secure Monitor Call) requests targeting rCPUs.
 * 
 * @ctx: Pointer to the trap_context structure containing CPU register state.
 *  - regs[0]: SMC function identifier (fid).
 *  - regs[1]: Argument used to determine the target rCPU.
 *
 * This function handles SMC calls related to remote CPUs (rCPUs) in the Jailhouse hypervisor.
 * It determines whether the SMC should be passed through, intercepted, or rejected based on
 * the ownership of the rCPU and the type of SMC function identifier (fid).
 *
 * Return: 
 *   -  0: Passthrough. The SMC is allowed to proceed normally.
 *   -  1: Intercept. The SMC return successfully to the OS without being propagated.
 *   - -1: Error. The SMC is invalid or not permitted. 
 */
int omnv_intercept_smc(struct trap_context *ctx)
{
	unsigned long *regs = ctx->regs;
	unsigned long fid = regs[0] & SMC_FID_MASK;
	int rcpu = get_rcpu_from_smc_arg(regs[1] & SMC_FID_MASK);
	int err = 0;

	/* The SMC fid is not targeting an rCPU */
	if (rcpu == -1)
		goto out;

	/* Only the root cell can handle rCPU SMCs */
	if (this_cell() != &root_cell) {
		panic_printk("[ERROR] OMNV: Non-root cell tried to access rCPU\n");
		err = -1;
		goto out;
	}

	/*
	 * If the rootcell owns the rCPU, passthrough
	 * N.B. The powerdown is done after the rCPU ownership is given to the root cell
	 * 	    so we can safely passthrough the powerdown here.
	 */
	if (test_bit(rcpu, root_cell.rcpu_set->bitmap))
		goto out;

	/* If the rootcell does not own the rCPU, handle the PM_WAKEUP_RCPU and PM_POWERDOWN_RCPU */
	if (fid == PM_WAKEUP_RCPU) {
		if (test_bit(rcpu, &rcpu_start_bitmap)) {
			disable_rcpu_start(rcpu);
			goto out;
		}
		/* In the load phase we need to fake the start of the rCPU
		 * so that rproc_boot() can be called but the rCPU is not actually started.
		 */
		if (load_phase) {
			disable_rcpu_load();
			err = 1; // Intercept
			goto out;
		}
		panic_printk("[ERROR] OMNV: PM_WAKEUP_RCPU invalid on rCPU %d\n", rcpu);
		err = -1;
		goto out;
	}

	if (fid == PM_POWERDOWN_RCPU) {
		/* If the Powerdown is requested before the startup (the start_bitmap is up) it is valid */
		if (test_bit(rcpu, &rcpu_start_bitmap)) {
			goto out;
		}
		panic_printk("[ERROR] OMNV: PM_POWERDOWN_RCPU invalid on rCPU %d\n", rcpu);
		err = -1;
		goto out;
	}

out:
	return err;
}

