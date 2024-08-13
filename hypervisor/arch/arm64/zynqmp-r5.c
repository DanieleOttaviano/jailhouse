/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Zynqmp RPUs and TCM management
 *
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/printk.h>
#include <asm/zynqmp-pm.h>
#include <asm/zynqmp-r5.h>


#if defined(CONFIG_OMNIVISOR) && defined(CONFIG_MACH_ZYNQMP_ZCU102)

/*
 * zynqmp_r5_0_start
 *
 * Start R5 Core from designated boot address (TCM or OCM).
 *
 * return 0 on success, otherwise non-zero value on failure
 */
int zynqmp_r5_start(enum pm_node_id node_id, u32 bootaddr)
{
	enum rpu_boot_mem bootmem;
	int ret;
	u32 status;

	//check node id
	if (node_id != NODE_RPU_0 && node_id != NODE_RPU_1) {
		printk("Invalid RPU node id %d\n", node_id);
		return -1;
	}

	//check boot from ocm or tcm
	bootmem = (bootaddr & 0xF0000000) == 0xF0000000 ?
		  PM_RPU_BOOTMEM_HIVEC : PM_RPU_BOOTMEM_LOVEC;

	// BOOT EXP
	printk("RPU boot from %s.\n",
		bootmem == PM_RPU_BOOTMEM_HIVEC ? "OCM" : "TCM");

	// If core is already powered on, turn off before re-wake.
	ret = zynqmp_pm_get_node_status(node_id, &status, NULL, NULL);
	if (ret){
		printk("ret after get node status\n");
		return ret;
	}
	if (status) {
		ret = zynqmp_pm_force_pwrdwn(node_id,
					     ZYNQMP_PM_REQUEST_ACK_BLOCKING);
		if (ret)
			return ret;
	}

	// Wake up the RPU core
	return zynqmp_pm_request_wake(node_id, 1,
				     bootmem, ZYNQMP_PM_REQUEST_ACK_NO);
}

int zynqmp_r5_stop(enum pm_node_id node_id){
	//check node id
	if (node_id != NODE_RPU_0 && node_id != NODE_RPU_1) {
		printk("Invalid RPU node id %d\n", node_id);
		return -1;
	}

	// power down the RPU core
	return zynqmp_pm_force_pwrdwn(node_id,
				     ZYNQMP_PM_REQUEST_ACK_BLOCKING);
}

int zynqmp_r5_tcm_request(int tcm_id){
	int ret1,ret2;
	if(tcm_id == 0){
		// Request TCM_0_A
		ret1 = zynqmp_pm_request_node(NODE_TCM_0_A,
								ZYNQMP_PM_CAPABILITY_ACCESS,
								0,
								ZYNQMP_PM_REQUEST_ACK_BLOCKING);
		// Request TCM_1_A
		ret2 = zynqmp_pm_request_node(NODE_TCM_1_A,
								ZYNQMP_PM_CAPABILITY_ACCESS,
								0,
								ZYNQMP_PM_REQUEST_ACK_BLOCKING);
	}
	else if(tcm_id == 1){
		// Request TCM_0_B
		ret1 = zynqmp_pm_request_node(NODE_TCM_0_B,
								ZYNQMP_PM_CAPABILITY_ACCESS,
								0,
								ZYNQMP_PM_REQUEST_ACK_BLOCKING);
		// Request TCM_1_B
		ret2 = zynqmp_pm_request_node(NODE_TCM_1_B,
								ZYNQMP_PM_CAPABILITY_ACCESS,
								0,
								ZYNQMP_PM_REQUEST_ACK_BLOCKING);
	}
	else{
		return -1;
	}
	
	return ret1 || ret2;
}

int zynqmp_r5_tcm_release(int tcm_id){
	int ret1,ret2;
	if(tcm_id == 0){
		// Request TCM_0_A
		ret1 = zynqmp_pm_release_node(NODE_TCM_0_A);
		// Request TCM_1_A
		ret2 = zynqmp_pm_release_node(NODE_TCM_1_A);
	}
	else if(tcm_id == 1){
		// Request TCM_0_B
		ret1 = zynqmp_pm_release_node(NODE_TCM_0_B);
		// Request TCM_1_B
		ret2 = zynqmp_pm_release_node(NODE_TCM_1_B);
	}
	else{
		return -1;
	}
	
	return ret1 || ret2;
}

#endif /* CONFIG_OMNIVISOR && CONFIG_MACH_ZYNQMP_ZCU102 */