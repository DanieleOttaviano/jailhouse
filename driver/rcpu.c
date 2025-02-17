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


static struct rcpu_info **root_rcpus_info;
static int num_root_rcpus;
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

// Check if the image file exists
static int check_image(const char *name)
{
	int err = 0;
	struct file *file;
	char filepath[256];

	snprintf(filepath, sizeof(filepath), "/lib/firmware/%s", name);

	file = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(file)) {
		err = PTR_ERR(file);
		pr_err("Failed to open Image file %s: %d\n", filepath, err);
		return err;
	}
	filp_close(file, NULL);
	return err;
}

int stop_rcpu_state(struct rcpu_info *rcpu)
{
	if (!rcpu || !rcpu->rproc) {
		pr_err("rCPU is not initialized\n");
		return -EINVAL;
	}

	// Check if the rcpu is running and shut it down
	if (rcpu->rproc->state == RPROC_RUNNING) {
		pr_info("Stopping rCPU %s\n", rcpu->name);
		if (rproc_shutdown(rcpu->rproc) < 0) {
			pr_err("Failed to stop rCPU %s\n", rcpu->name);
			return -1;
		}
	}
	
	return 0;
}

int detach_rcpu_state(struct rcpu_info *rcpu)
{
	if (!rcpu || !rcpu->rproc) {
		pr_err("rCPU is not initialized\n");
		return -1;
	}

	// Check if the rcpu is attached and detach it
	if (rcpu->rproc->state == RPROC_ATTACHED) {
		pr_info("Detaching rCPU %s\n", rcpu->name);
		if (rproc_detach(rcpu->rproc) < 0) {
			pr_err("Failed to detach rCPU %s\n", rcpu->name);
			return -1;
		}
	}
	
	return 0;
}

int start_rcpu_state(struct rcpu_info *rcpu)
{
	if (!rcpu || !rcpu->rproc) {
		pr_err("rCPU is not initialized\n");
		return -1;
	}

	// detach the rcpu if it is attached
	if(detach_rcpu_state(rcpu) < 0){
		return -1;
	}

	// stop the rcpu if it is running
	if(stop_rcpu_state(rcpu) < 0){
		return -1;
	}

	// Start the rcpu
	pr_info("Starting rCPU %s\n", rcpu->name);
	if (rproc_boot(rcpu->rproc) < 0) {
		pr_err("Failed to start rCPU %s\n", rcpu->name);
		return -1;
	}

	return 0;
}

// Fetch rproc pointer from device
int fetch_rproc_from_device(struct device_node *dev_node, struct rcpu_info *rcpu)
{		
	//DEBUG
	pr_info("Remote Processor: %s, phandle: %d\n", dev_node->name, dev_node->phandle);
	
	rcpu->rproc = rproc_get_by_phandle(dev_node->phandle);
	if (!rcpu->rproc) {
		pr_err("Failed to get Remote Processor by phandle\n");
		return -1;
	}
	
	return 0;
}

// Fetch rproc pointer from rCPU name and compatible
int fetch_rproc(struct rcpu_info *rcpu)
{
	struct device_node *dev_node;
	struct platform_device *pdev;
	struct device *dev;
	int found = 0;

	pr_info("Fetching rproc for rCPU %s.\n", rcpu->name);
	
	// Find the device node in the device tree
	dev_node = of_find_compatible_node(NULL, NULL, rcpu->compatible);
	if(!dev_node) {
		pr_err("Failed to find device node\n");
		return -1;
	}	

	// Check the name to be sure there is only one rCPU otherwise 
	// try fetching the rproc from the child nodes of the dev_node
	if(strcmp(dev_node->name, rcpu->name) == 0){
		// fetch rproc pointer from device
		if(fetch_rproc_from_device(dev_node, rcpu) < 0){
			pr_err("Failed to fetch rproc for rCPU %s\n", rcpu->name);
			return -1;
		}
		found = 1;
	}
	else{	
		// Find the platform device
		pdev = of_find_device_by_node(dev_node);
		if(!pdev) {
			pr_err("Failed to find platform device\n");
			return -1;
		}	
		dev = &pdev->dev;
		// Check child nodes
		for_each_available_child_of_node(dev->of_node, dev_node){	
			if(strcmp(dev_node->name, rcpu->name) == 0){
				// fetch rproc pointer from device
				if(fetch_rproc_from_device(dev_node, rcpu) < 0){
					pr_err("Failed to fetch rproc for rCPU %d\n", rcpu->id);
					return -1;
				}
				found = 1;
			}	
		}
	}

	if(!found){
		pr_err("Failed to find rCPU %s\n", rcpu->name);
		return -1;
	}
	
	// Detach the rcpu if it is attached
	if(detach_rcpu_state(rcpu) < 0){
		return -1;
	}

	// Stop the rcpu if it is running
	if(stop_rcpu_state(rcpu) < 0){
		return -1;
	}

	return 0;	
}

// Setup the rCPU from the cell device description
int setup_rcpu(struct rcpu_info **rcpu_array, unsigned int rcpu_id , const struct jailhouse_rcpu_device *rcpu_devices, unsigned int rcpu_device_id)
{
	// allocate memory for the rproc struct
	rcpu_array[rcpu_id] = vmalloc(sizeof(struct rcpu_info));
	if (!rcpu_array[rcpu_id]) {
		pr_err("Failed to allocate memory for rCPU %d\n", rcpu_array[rcpu_id]->id);
		return -ENOMEM;
	}

	// copy the id, the name, and the compatible from the configuration
	rcpu_array[rcpu_id]->id = rcpu_devices[rcpu_device_id].rcpu_id;
	strncpy(rcpu_array[rcpu_id]->name, rcpu_devices[rcpu_device_id].name, JAILHOUSE_RCPU_IMAGE_NAMELEN);
	strncpy(rcpu_array[rcpu_id]->compatible, rcpu_devices[rcpu_device_id].compatible, JAILHOUSE_RCPU_IMAGE_NAMELEN);
	pr_info("rCPU id: %d, name: %s, compatible: %s\n", rcpu_array[rcpu_id]->id, rcpu_array[rcpu_id]->name, rcpu_array[rcpu_id]->compatible);

	// Find the device node in the device tree
	if(fetch_rproc(rcpu_array[rcpu_id]) < 0){
		pr_err("Failed to fetch rproc for rCPU %d\n", rcpu_array[rcpu_id]->id);
		return -1;
	}
	pr_info("rCPU %s fetched\n", rcpu_array[rcpu_id]->rproc->name);
	return 0;
}


// Setup the rcpus of the rootcell in the root_rcpu_info array
int jailhouse_root_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
	const struct jailhouse_rcpu_device *rcpu_devices;
	unsigned int rcpu_id;
	unsigned int rcpu_device_id;
	int device_found = 0;
	
	// check rootcell call this function	
	if(cell->id != 0){
		pr_err("ERROR: only root cell must call\n");
		return -1;
	}

	// check that the number of assigned rcpus is equal to the number of rcpus devices
	if (cpumask_weight(&cell->rcpus_assigned) != config->num_rcpu_devices){
		pr_err("ERROR: number of assigned rcpus %d is different from the number of rcpus devices %d\n", 
			cpumask_weight(&cell->rcpus_assigned), config->num_rcpu_devices);
		return -1;
	}
	num_root_rcpus = config->num_rcpu_devices;

	// allocate memory for the root_rcpus_info array
	pr_info("Allocating memory for root_rcpus_info\n");
	root_rcpus_info = vmalloc(config->num_rcpu_devices * sizeof(struct rcpu_info *));
	if (!root_rcpus_info) {
		pr_err("Failed to allocate memory for rcpu_info\n");
		return -ENOMEM;
	}

	// get the rcpus devices info from the configuration
	rcpu_devices = jailhouse_cell_rcpu_devices(config);
	
	// for each asic rcpu assigned to the root cell
	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {

		// check the user assigned rcpus id correctly in the devices
		device_found = 0;
		for(rcpu_device_id = 0; rcpu_device_id < num_root_rcpus; rcpu_device_id++) {
			if(rcpu_id == rcpu_devices[rcpu_device_id].rcpu_id) {
				pr_info("rCPU %d found as a device in the configuration\n", rcpu_id);
				device_found = 1;
				break;
			}
		}
		if(!device_found){
			pr_err("ERROR: missing rCPU %d in the configuration\n", rcpu_id);
			return -1;
		}

		// Setup the rCPU from the cell device description
		if(setup_rcpu(root_rcpus_info, rcpu_id, rcpu_devices, rcpu_device_id) < 0){
			pr_err("Failed to setup rCPU %d\n", rcpu_id);
			return -1;
		}
	}

	return 0;
}

// Setup the rcpus of a cell by taking the asic rcpus from the rootcell and allocating new memory for the soft-core rcpus
int jailhouse_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
	const struct jailhouse_rcpu_device *rcpu_devices;
	unsigned int rcpu_id;
	unsigned int soft_rcpu_id;
	unsigned int soft_rcpu_device_id;
	int device_found = 0;


	// get the rcpus devices info from the configuration
	rcpu_devices = jailhouse_cell_rcpu_devices(config);
	
	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {
		// distinguish between asic rcpus and soft-core rcpus
		if(rcpu_id < num_root_rcpus){ 
			// remove the rcpu from the root cell (they are already initialized by the rootcell)
			pr_info("Removing rCPU %d from rootcell.\n", rcpu_id);
			cpumask_clear_cpu(rcpu_id, &root_cell->rcpus_assigned);
		}
		else{
			device_found = 0;
			for(soft_rcpu_device_id = 0; soft_rcpu_device_id < config->num_rcpu_devices; soft_rcpu_device_id++) {
				if(rcpu_id == rcpu_devices[soft_rcpu_device_id].rcpu_id) {
					pr_info("soft-rCPU %d found as a device in the configuration\n", rcpu_id);
					device_found = 1;
					break;
				}
			}
			if(!device_found){
				pr_err("ERROR: soft-rCPU %d not found in the configuration\n", rcpu_id);
				return -1;
			}

			// calculate the id of the soft-core rcpu starting from 0
			soft_rcpu_id = rcpu_id - num_root_rcpus;

			// Setup the rCPU from the cell device description
			if(setup_rcpu(cell->soft_rcpus_info, soft_rcpu_id, rcpu_devices, soft_rcpu_device_id) < 0){
				pr_err("Failed to setup soft-rCPU %d\n", soft_rcpu_id);
				return -1;
			}	
		}
	}

	return 0;
}

// load the rcpu image
int jailhouse_load_rcpu_image(struct cell *cell, struct jailhouse_preload_rcpu_image __user *rcpu_uimage){
	struct jailhouse_preload_rcpu_image rcpu_image;
	int rcpu_id;
	int err;

	if (copy_from_user(&rcpu_image, rcpu_uimage, sizeof(rcpu_image)))
		return -EFAULT;

	rcpu_id = rcpu_image.rcpu_id; //starts from 0

	// Check if the rcpu id is in the assigned rcpus
	pr_info("Checking rcpu ID %d\n", rcpu_id);
	err = !(cpumask_test_cpu(rcpu_id, &cell->rcpus_assigned));
	if (err) {
		pr_err("rcpu ID %d not valid\n",rcpu_id);
		return err;
	}

	// Check if the image file exists
	pr_info("Checking Image file %s\n", rcpu_image.name);
	if (check_image(rcpu_image.name)){
		return err;
	}

	// Load the rcpu image. distinguish between asic rcpus and soft-core rcpus
	if(rcpu_id < num_root_rcpus){
		pr_info("Loading Firmware %s on rCPU %d\n", rcpu_image.name, rcpu_id);
		err = rproc_set_firmware(root_rcpus_info[rcpu_id]->rproc, rcpu_image.name);
		if (err) {
			pr_err("Failed to set firmware for rcpu %d\n", rcpu_id);
			return err;
		}
		pr_info("Loaded Firmware %s on rCPU %d\n", rcpu_image.name, rcpu_id);
	}
	else{// soft-core
		pr_info("Loading Firmware %s on soft-rCPU %d\n", rcpu_image.name, rcpu_id);
		err = rproc_set_firmware(cell->soft_rcpus_info[rcpu_id - num_root_rcpus]->rproc, rcpu_image.name);
		if (err) {
			pr_err("Failed to set firmware for rcpu %d\n", rcpu_id);
			return err;
		}
		pr_info("Loaded Firmware %s on soft-rCPU %d\n", rcpu_image.name, rcpu_id);
	}

	return 0;
}

// start the rcpu
int jailhouse_start_rcpu(struct cell *cell){
	unsigned int rcpu_id;
	unsigned int soft_rcpu_id;

	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {		
		pr_info("Starting rcpus %d of cell %d\n", rcpu_id, cell->id);
		// distinguish between asic rcpus and soft-core rcpus
		if(rcpu_id < num_root_rcpus){
			if(start_rcpu_state(root_rcpus_info[rcpu_id]) < 0){
				pr_err("Failed to start rCPU %d\n", rcpu_id);
				return -1;
			}
		}
		else{
			soft_rcpu_id = rcpu_id - num_root_rcpus;
			if(start_rcpu_state(cell->soft_rcpus_info[soft_rcpu_id]) < 0){
				pr_err("Failed to start soft-rCPU %d\n", rcpu_id);
				return -1;
			}	
		}
	}
	return 0;
}

// Release and dealloc the rproc instances for asic rcpus
int jailhouse_root_rcpus_remove()
{
	unsigned int rcpu_id;

	// check if the root_rcpus_info array is NULL
	if(root_rcpus_info == NULL)
		return 0;

	for_each_rcpu(rcpu_id, &root_cell->rcpus_assigned) {
		pr_info("Releasing rCPU %s\n", root_rcpus_info[rcpu_id]->name);
		
		// remove the rproc instance
		rproc_put(root_rcpus_info[rcpu_id]->rproc);

		if(root_rcpus_info[rcpu_id] != NULL)
			vfree(root_rcpus_info[rcpu_id]);
	}
	// free the root_rcpus_info array
	vfree(root_rcpus_info);

	return 0;
}

// Release and dealloc the rproc instances for soft rcpus, and return the asic rcpus to the root cell
int jailhouse_rcpus_remove(struct cell *cell){
	int err = 0;
	unsigned int rcpu_id;
	unsigned int soft_rcpu_id;
	
	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {	
		pr_info("Removing rcpus %d from cell %d\n", rcpu_id, cell->id);
		// distinguish between asic rcpus and soft-core rcpus
		if(rcpu_id < num_root_rcpus){
			// Check if the rproc is NULL
			if(root_rcpus_info[rcpu_id]->rproc == NULL){
				break;
			}

			// Check if the rcpu is alredy offline
			detach_rcpu_state(root_rcpus_info[rcpu_id]);
			stop_rcpu_state(root_rcpus_info[rcpu_id]);

			//reassign the rcpu to the root cell
			cpumask_set_cpu(rcpu_id, &root_cell->rcpus_assigned);
		}
		else{
			soft_rcpu_id = rcpu_id - num_root_rcpus;
			//check if the rproc is NULL
			if(cell->soft_rcpus_info[soft_rcpu_id]->rproc == NULL){
				break;
			}

			// Check if the rcpu is alredy offline
			detach_rcpu_state(cell->soft_rcpus_info[soft_rcpu_id]);
			stop_rcpu_state(cell->soft_rcpus_info[soft_rcpu_id]);
	
			//dealloc the rproc instance
			pr_info("Releasing soft-rCPU %s\n", cell->soft_rcpus_info[soft_rcpu_id]->name);
			rproc_put(cell->soft_rcpus_info[soft_rcpu_id]->rproc);
			vfree(cell->soft_rcpus_info[soft_rcpu_id]);
		}
	}
	
	// free the soft rproc array
	if(cell->soft_rcpus_info != NULL)
		vfree(cell->soft_rcpus_info);

	return err;
}

#endif /* CONFIG_OMNIVISOR */