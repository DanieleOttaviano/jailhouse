/*
* Jailhouse, a Linux-based partitioning hypervisor
*
* Omnivisor demo RISC-V Makefile
*
* Copyright (c) Daniele Ottaviano, 2024
*
* Authors:
*   Daniele Ottaviano <danieleottaviano97@gmail.com>
*
* This work is licensed under the terms of the GNU GPL, version 2.  See
* the COPYING file in the top-level directory.
*
*/

#include "rcpu.h"

#ifdef CONFIG_OMNIVISOR
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>


static const struct of_device_id rproc_of_match[] = {
	{ .compatible = "xlnx,zynqmp-r5-remoteproc", },
	{ .compatible = "xlnx,versal-r5-remoteproc", },
	{},
};

/*
 * A state-to-string lookup table, for exposing a human readable state
 * via sysfs. Always keep in sync with enum rproc_state
 */
static const char * const rproc_state_string[] = {
	[RPROC_OFFLINE]		= "offline",
	[RPROC_SUSPENDED]	= "suspended",
	[RPROC_RUNNING]		= "running",
	[RPROC_CRASHED]		= "crashed",
	[RPROC_DELETED]		= "deleted",
	[RPROC_ATTACHED]	= "attached",
	[RPROC_DETACHED]	= "detached",
	[RPROC_LAST]		= "invalid",
};

static struct rproc **rproc;
static int num_rprocs;

static int image_exists(const char *name){
	struct file *file;
	int err;
	char filepath[256];

	snprintf(filepath, sizeof(filepath), "/lib/firmware/%s", name);

	file = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(file)) {
		err = PTR_ERR(file);
		pr_err("Failed to open Image file %s: %d\n", filepath, err);
		return err;
	}
	filp_close(file, NULL);
	return 0;
}

// check if the number of requested rcpus is less or equal than the available rcpus
int jailhouse_rcpus_check(struct cell *cell){
	int required_rcpus = 0;
	required_rcpus = cpumask_weight(&cell->rcpus_assigned);

	//DEBUG
	//pr_info("Required rcpus:  %d\n", required_rcpus);
	//pr_info("Available rcpus: %d\n", num_rprocs);

	if (required_rcpus > num_rprocs){
		pr_err("Requested rcpus (%d) are more than available rcpus (%d)\n", required_rcpus, num_rprocs);	
		return -EINVAL;
	}

	return 0;
}

// Save the existing remoteproc instances in the rproc array
int jailhouse_rcpus_setup(){
	int err;
	int i = 0;
	struct device_node *dev_node;
	struct platform_device *pdev;
	struct device *dev;
	phandle phandle;

	// Find the device node in the device tree
	dev_node = of_find_matching_node(NULL, rproc_of_match);
	if(!dev_node) {
		pr_err("Failed to find device node\n");
		return -ENODEV;
	}
	else{
		//DEBUG
		pr_info("Found Device Node: %s\n", dev_node->name);
	}

	// chech the available number of processors
	num_rprocs = of_get_available_child_count(dev_node);
	if (num_rprocs <= 0) {
		pr_err("Failed to get number of rprocs\n");
		return -ENODEV;
	}
	else{
		//DEBUG
		pr_info("Number of Remote Cores (rcpus): %d\n", num_rprocs);
	}

	// allocate memory for rproc array
	rproc = kzalloc(num_rprocs * sizeof(struct rproc *), GFP_KERNEL);
	if (!rproc) {
		pr_err("Failed to allocate memory for rproc array\n");
		return -ENOMEM;
	}

	// Find the platform device from the device node
	pdev = of_find_device_by_node(dev_node);
	if(!pdev) {
		pr_err("Failed to find platform device\n");
		return -ENODEV;
	}	
	dev = &pdev->dev;

	//TO DO ... check on configuration mode lockstep or split for supported processors

	// For each processor in the device tree, get the remoteproc instance
	for_each_available_child_of_node(dev->of_node, dev_node) {	
		phandle = dev_node->phandle;
		//DEBUG
		pr_info("Remote Processor: %s, phandle: %d\n", dev_node->name, phandle);
		rproc[i] = rproc_get_by_phandle(phandle);
		if (!rproc[i]) {
			pr_err("Failed to get Remote Processor by phandle\n");
			return -ENODEV;
		}

		// Check if the rcu is in running or attached state
		if (rproc[i]->state == RPROC_RUNNING || rproc[i]->state == RPROC_ATTACHED) {
			pr_info("rcpu %d is %s, %s it\n", i,
				rproc[i]->state == RPROC_RUNNING ? "running" : "attached", 
				rproc[i]->state == RPROC_RUNNING ? "stopping" : "detaching");
			err = rproc[i]->state == RPROC_RUNNING ? rproc_shutdown(rproc[i]) : rproc_detach(rproc[i]);
			if (err) {
				pr_err("Failed to %s rcpu %d\n", 
					rproc[i]->state == RPROC_RUNNING ? "stop" : "detach", i);
			}
		}

		i++;
	}

	return 0;
}

// load the rcpu image
int jailhouse_load_rcpu_image(struct cell *cell, 
                struct jailhouse_preload_rcpu_image __user *rcpu_uimage){
	struct jailhouse_preload_rcpu_image rcpu_image;
	int id;
	int err;

	if (copy_from_user(&rcpu_image, rcpu_uimage, sizeof(rcpu_image)))
		return -EFAULT;

	id = rcpu_image.rcpu_id; //starts from 0

	// Check if the rcpu id is in the assigned rcpus
	err = !(cpumask_test_cpu(id, &cell->rcpus_assigned));
	if (err) {
		pr_err("rcpu ID %d not valid\n",id);
		return err;
	}

	// Check if the image file exists
	err = image_exists(rcpu_image.name); 
	if (err) {
		pr_err("Image %s not found\n", rcpu_image.name);
		return err;
	}

	// Load the rcpu image
	err = rproc_set_firmware(rproc[id], rcpu_image.name);
	if (err) {
		pr_err("Failed to set firmware for rcpu %d\n", id);
		return err;
	}
	//DEBUG
	pr_info("Loaded Firmware %s on rcpu %d\n", rcpu_image.name, id);

	return 0;
}

// start the rcpu
int jailhouse_start_rcpu(unsigned int rcpu_id){
	int err;
	
	// Check if the rcu is in running or attached state
	if (rproc[rcpu_id]->state == RPROC_RUNNING || rproc[rcpu_id]->state == RPROC_ATTACHED) {
		pr_info("rcpu %d is %s, %s it\n", rcpu_id,
			rproc[rcpu_id]->state == RPROC_RUNNING ? "running" : "attached", 
			rproc[rcpu_id]->state == RPROC_RUNNING ? "stopping" : "detaching");
		err = rproc[rcpu_id]->state == RPROC_RUNNING ? rproc_shutdown(rproc[rcpu_id]) : rproc_detach(rproc[rcpu_id]);
		if (err) {
			pr_err("Failed to %s rcpu %d\n", 
				rproc[rcpu_id]->state == RPROC_RUNNING ? "stop" : "detach", rcpu_id);
		}
	}

	err = rproc_boot(rproc[rcpu_id]);
	if (err) {
		pr_err("Failed to start rcpu %d\n", rcpu_id);
		return err;
	}
	return 0;
}

// stop the rcpu
int jailhouse_stop_rcpu(unsigned int rcpu_id){
	int err;

	// Check if the rcu is alredy offline
	if (rproc[rcpu_id]->state == RPROC_OFFLINE) {
		pr_err("rcpu %d is already offline\n", rcpu_id);
		return 0;
	}

	// Stop the rcpu
	err = rproc_shutdown(rproc[rcpu_id]);
	if (err) {
		pr_err("Failed to stop rcpu %d\n", rcpu_id);
		return err;
	}
	return 0;
}

// free the rproc array
int jailhouse_rcpus_remove(){
	int i;
	for (i = 0; i < num_rprocs; i++) {
		rproc_put(rproc[i]);
	}
	kfree(rproc);
	//DEBUG
	pr_info("Freed rproc array\n");
	return 0;
}

#endif /* CONFIG_OMNIVISOR */