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
static unsigned long load_phase_bitmap = 0;
static unsigned long fpga_load_bitmap = 0;
static int fpga_load_cell_id = -1;

void enable_rcpu_start(unsigned int rcpu)
{
	set_bit(rcpu, &rcpu_start_bitmap);
}

static void disable_rcpu_start(unsigned int rcpu)
{
	clear_bit(rcpu, &rcpu_start_bitmap);
}

void enable_rcpu_load(unsigned int rcpu)
{
	set_bit(rcpu, &load_phase_bitmap);
}

static inline void disable_rcpu_load(unsigned int rcpu)
{
	clear_bit(rcpu, &load_phase_bitmap);
}

void enable_fpga_load(unsigned int cell_id, unsigned int region_id)
{
	fpga_load_cell_id = (int)cell_id;
	set_bit(region_id, &fpga_load_bitmap);
}

static void disable_fpga_load(unsigned int region_id)
{
	clear_bit(region_id, &fpga_load_bitmap);
	fpga_load_cell_id = -1;
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
 * omnv_intercept_smc_rcpus - Intercepts SMC (Secure Monitor Call) requests targeting rCPUs.
 *
 * @cell: Pointer to the cell structure representing the current cell.
 * @fid: SMC function identifier.
 * @rcpu: Target rCPU identifier.
 *
 * This function verifies the ownership of the rCPU and the validity of the SMC request.
 * 
 * Return: 
 *   -  0: Passthrough. The SMC is allowed to proceed normally.
 *   -  1: Intercept. The SMC return successfully to the OS without being propagated.
 *   - -1: Error. The SMC is invalid or not permitted. 
 */
static int omnv_intercept_smc_rcpus(struct cell *cell, unsigned long fid, int rcpu)
{
	int err = 0;

	/*
	 * If the cell owns the rCPU, passthrough
	 * N.B. The powerdown is done after the rCPU ownership is given to the root cell
	 * 	    so we can safely passthrough the powerdown here.
	 */
	if (test_bit(rcpu, cell->rcpu_set->bitmap))
		goto out;
	
	/* Only the root cell can handle rCPU SMCs */
	if (this_cell() != &root_cell) {
		panic_printk("[ERROR] OMNV: Non-root_cell tried to access not owned rCPU\n");
		err = -1;
		goto out;
	}

	/* If the rootcell does not own the rCPU, handle the PM_WAKEUP_RCPU and PM_POWERDOWN_RCPU */
	if (fid == PM_WAKEUP_RCPU) {
		/* In the load phase we need to fake the start of the rCPU
		* so that rproc_boot() can be called but the rCPU is not actually started.
		*/
		if (test_bit(rcpu, &load_phase_bitmap)) {
			disable_rcpu_load(rcpu);
			err = 1; // Intercept
			goto out;
		}

		if (test_bit(rcpu, &rcpu_start_bitmap)) {
			disable_rcpu_start(rcpu);
			goto out;
		}
		
		panic_printk("[ERROR] OMNV: invalid PM_WAKEUP_RCPU on rCPU %d\n", rcpu);
		err = -1;
		goto out;
	}

	if (fid == PM_POWERDOWN_RCPU) {
		/* If the Powerdown is requested before the startup (the start_bitmap is up) it is valid */
		if (test_bit(rcpu, &rcpu_start_bitmap)) {
			goto out;
		}
		panic_printk("[ERROR] OMNV: invalid PM_POWERDOWN_RCPU on rCPU %d\n", rcpu);
		err = -1;
		goto out;
	}

out:
	return err;
}

/**
 * omnv_intercept_smc_fpga - Intercepts SMC (Secure Monitor Call) requests targeting FPGA regions.
 * 
 * @cell: Pointer to the cell structure representing the current cell.
 * @fid: SMC function identifier.
 * 
 * This function verifies the ownership of the FPGA and the validity of the SMC request.
 * 
 * Return:
 * -  0: Passthrough. The SMC is allowed to proceed normally.
 * - -1: Error. The SMC is invalid or not permitted. 
 */
static int omnv_intercept_smc_fpga(struct cell *cell, unsigned long fid, __u32 region_size_id)
{
	int err = 0;
	struct cell *cell_owner;
	const struct jailhouse_fpga_device *cell_owner_fpga_devices;	
	const struct jailhouse_fpga_device *cell_fpga_devices = jailhouse_cell_fpga_devices(cell->config);	

	/*
	 * If the cell owns the FPGA region, passthrough
	 */
	for(unsigned int i = 0; i < cell->config->num_fpga_devices; i++){
		if(region_size_id == cell_fpga_devices[i].fpga_bitstream_size){
			goto out;
		}
	}
	
	/*
	 * Only the root cell can handle FPGA SMCs
	 */
	if (cell != &root_cell) {
		panic_printk("[ERROR] OMNV: Non-root_cell tried to load FPGA bitstream\n");
		err = -1;
		goto out;
	}

	/* Find the cell that owns the FPGA region */
	for_each_cell(cell_owner){
		if(cell_owner->config->id == (unsigned int)fpga_load_cell_id) break;
	} 
	cell_owner_fpga_devices = jailhouse_cell_fpga_devices(cell_owner->config);
	printk("[INFO] OMNV: cell_owner fpga region bitmap: 0x%lx\n", cell_owner->fpga_region_set->bitmap[0]);
	
	//TODO: Daniele Ottaviano, implement a more fine grained control of the FPGA status access
	if (fid == PM_FPGA_GET_STATUS) {
		err = 0; // Passthrough
		goto out;
	}

	/* Check if the driver enabled FPGA loading of the region otherwise deny */
	if (fid == PM_FPGA_LOAD) {
		/* Check if an FPGA load is enabled by the hypervisor during the create*/
		for(__u32 i = 0; i < cell_owner->config->num_fpga_devices; i++){
			printk("[INFO] OMNV: checking fpga device with region id %d and size 0x%08x [region_size_id: 0x%08x]\n", 
				cell_owner_fpga_devices[i].fpga_region_id, cell_owner_fpga_devices[i].fpga_bitstream_size, region_size_id);
			if(test_bit(cell_owner_fpga_devices[i].fpga_region_id, &fpga_load_bitmap) &&
				region_size_id == cell_owner_fpga_devices[i].fpga_bitstream_size) {
				disable_fpga_load(cell_owner_fpga_devices[i].fpga_region_id);
				goto out;
			}
		}
		printk("[ERROR] OMNV: invalid FPGA Load SMC request\n");
		err = -1; // ERROR: No FPGA load enabled
		goto out;
	}

out:
	return err;
}

/**
 * omnv_intercept_smc - Intercepts SMC (Secure Monitor Call) requests targeting rCPUs or FPGA regions.
 * 
 * @ctx: Pointer to the trap_context structure containing CPU register state.
 *  - regs[0]: SMC function identifier (fid).
 *  - regs[1]: Argument used to determine the target rCPU.
 *
 * This function handles SMC calls related to remote CPUs (rCPUs) or FPGA regions in the Jailhouse hypervisor.
 * It determines whether the SMC should be passed through, intercepted, or rejected based on
 * the ownership of the resources and the type of SMC function identifier (fid).
 *
 * Return: 
 *   -  0: Passthrough. The SMC is allowed to proceed normally.
 *   -  1: Intercept. The SMC return successfully to the OS without being propagated.
 *   - -1: Error. The SMC is invalid or not permitted. 
 */
int omnv_intercept_smc(struct trap_context *ctx){
	int err = 0;
	struct cell *cell = this_cell();
	int rcpu = -1;
	unsigned int region_size_id;
	unsigned long *regs = ctx->regs;
	unsigned long fid = regs[0] & SMC_FID_MASK;
	
	printk("OMNV: Intercepted SMC fid: 0x%lx from cell %s\n", fid, cell->config->name);
	if (fid == PM_FPGA_LOAD || fid == PM_FPGA_GET_STATUS) {
		// the region_size is used as an ID for the FPGA region
		region_size_id = (unsigned int)(regs[2] & 0xFFFFFFFF);
		err = omnv_intercept_smc_fpga(cell, fid, region_size_id);
	} else if (fid == PM_WAKEUP_RCPU || fid == PM_POWERDOWN_RCPU) {
		rcpu = get_rcpu_from_smc_arg(regs[1] & SMC_RCPU_MASK);		
		/* The SMC fid is not targeting an rCPU */
		if (rcpu == -1)
			goto out;
		err = omnv_intercept_smc_rcpus(cell, fid, rcpu);
	} else {
		/* Not an OMNV related SMC */
		goto out;	
	}

out:
	return err;
}