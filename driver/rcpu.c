/*
* Jailhouse, a Linux-based partitioning hypervisor
*
* Omnivisor rcpu management through remoteproc linux driver
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
#include <linux/remoteproc.h>

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

/**
 * check_image - Check if the image file exists in the firmware directory
 * @name: Name of the image file to check
 *
 * This function checks if the specified image file exists in the firmware
 * directory /lib/firmware. 
 *
 * Return: 0 on success, or a negative error code on failure.
 */
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

/**
 * unprepare_subdevices - Unprepare all subdevices of a remote processor
 * @rproc: Pointer to the remote processor structure
 *
 * Unprepares each subdevice of the given remote processor. This function is
 * typically called when the remote processor is being shut down or when
 * resources need to be released.
 */
static void unprepare_subdevices(struct rproc *rproc)
{
	struct rproc_subdev *subdev;

	list_for_each_entry_reverse(subdev, &rproc->subdevs, node) {
		if (subdev->unprepare)
			subdev->unprepare(subdev);
	}
}

/**
 * start_subdevices - Start all subdevices of a remote processor
 * @rproc: Pointer to the remote processor structure
 *
 * Starts each subdevice of the given remote processor. On failure, stops
 * previously started subdevices in reverse order and returns the error.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int start_subdevices(struct rproc *rproc)
{
	struct rproc_subdev *subdev;
	int err = 0;

	list_for_each_entry(subdev, &rproc->subdevs, node) {
		if (subdev->start) {
			err = subdev->start(subdev);
			if (err < 0)
				goto unroll_registration;
		}
	}

	return 0;

unroll_registration:
	list_for_each_entry_continue_reverse(subdev, &rproc->subdevs, node) {
		if (subdev->stop)
			subdev->stop(subdev, true);
	}

	return err;
}

/**
 * stop_rcpu_state - Stops the remote CPU (rCPU) if it is running.
 * @rcpu: Pointer to the rcpu_info structure representing the rCPU.
 *
 * Return: 0 on success, -1 if the shutdown process fails.
 */
static int stop_rcpu_state(struct rcpu_info *rcpu)
{
	if (!rcpu || !rcpu->rproc) {
		pr_err("rCPU is not initialized\n");
		return -EINVAL;
	}

	if (rcpu->rproc->state == RPROC_RUNNING) {
		pr_info("Stopping rCPU %s\n", rcpu->name);
		if (rproc_shutdown(rcpu->rproc) < 0) {
			pr_err("Failed to stop rCPU %s\n", rcpu->name);
			return -1;
		}
	}
	
	return 0;
}

/**
 * detach_rcpu_state - Detaches the remote CPU (rCPU) if it is attached.
 * @rcpu: Pointer to the rcpu_info structure representing the rCPU.
 *
 * Return: 0 on success, -1 if the detach process fails.
 */
static int detach_rcpu_state(struct rcpu_info *rcpu)
{
	if (!rcpu || !rcpu->rproc) {
		pr_err("rCPU is not initialized\n");
		return -1;
	}

	if (rcpu->rproc->state == RPROC_ATTACHED) {
		pr_info("Detaching rCPU %s\n", rcpu->name);
		if (rproc_detach(rcpu->rproc) < 0) {
			pr_err("Failed to detach rCPU %s\n", rcpu->name);
			return -1;
		}
	}
	
	return 0;
}

/**
 * start_rcpu_state - Start the remote processor (rCPU) and its subdevices.
 * @rcpu: Pointer to the rcpu_info structure representing the remote processor.
 *
 * This function initializes and starts the remote processor (rCPU) and its
 * associated subdevices.
 *
 * If any step fails, the function performs cleanup by stopping the rproc,
 * unpreparing the subdevices, and restoring the cached resource table.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int start_rcpu_state(struct rcpu_info *rcpu)
{
	int err = 0;

	if (!rcpu || !rcpu->rproc) {
		pr_err("rCPU is not initialized\n");
		return -EINVAL;
	}

	pr_info("Starting rCPU %s\n", rcpu->name);

	err = mutex_lock_interruptible(&(rcpu->rproc)->lock);
	if (err < 0) {
		pr_err("Failed to lock rproc mutex\n");
		return err;
	}

	err = rcpu->rproc->ops->start(rcpu->rproc);
	if (err < 0) {
		pr_err("Failed to start rCPU %s\n", rcpu->name);
		goto unprepare_subdevices;
	}

	err = start_subdevices(rcpu->rproc);
	if (err < 0) {
		pr_err("Failed to start subdevices for rCPU %s\n", rcpu->name);
		goto stop_rproc;
	}

	rcpu->rproc->state = RPROC_RUNNING;
	pr_info("rCPU %s is now up\n", rcpu->name);
	goto unlock;

stop_rproc:
	rcpu->rproc->ops->stop(rcpu->rproc);
unprepare_subdevices:
	unprepare_subdevices(rcpu->rproc);
	rcpu->rproc->table_ptr = rcpu->rproc->cached_table;
unlock:
	mutex_unlock(&rcpu->rproc->lock);
	return err;
}

/**
 * load_rcpu_firmware - Loads the firmware for a remote CPU (rCPU) using remoteproc
 * @rcpu: Pointer to the rcpu_info structure containing information about the rCPU
 *
 * This function attempts to load the firmware for the specified remote CPU
 * using the remoteproc framework. It stops just before booting the remote
 * processor.
 *  
 * WARNING: Note that a specific remoteproc OMNV patch is required for this
 * functionality to work correctly.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int load_rcpu_firmware(struct rcpu_info *rcpu)
{
	int err = 0;

	err = rproc_boot(rcpu->rproc);
	if (err < 0) {
		pr_err("Failed to load rCPU %s\n", rcpu->name);
		rproc_shutdown(rcpu->rproc);
	}

	return err;
}

/**
 * fetch_rproc_from_device - Fetches the remote processor (rproc) pointer from a device node.
 * @dev_node: Pointer to the device node in the device tree corresponding to a remote processor.
 * @rcpu: Pointer to the rcpu_info structure where the retrieved rproc pointer will be stored.
 *
 * This function attempts to retrieve the remote processor (rproc) structure associated with
 * the given device node by using its phandle. If successful, the rproc pointer is linked
 * to the provided rcpu_info structure. If the rproc cannot be retrieved, an error is logged
 * and the function returns -ENODEV.
 *
 * Return: 0 on success, -ENODEV if the remote processor cannot be retrieved.
 */
static int fetch_rproc_from_device(struct device_node *dev_node, struct rcpu_info *rcpu)
{
	int err = 0;

	pr_info("Remote Processor: %s, phandle: %d\n", dev_node->name, dev_node->phandle);
	rcpu->rproc = rproc_get_by_phandle(dev_node->phandle);
	if (!rcpu->rproc) {
		pr_err("Failed to get Remote Processor by phandle\n");
		err = -ENODEV;
	}

	return err;
}

/**
 * fetch_rproc - Fetch a remote processor (rproc) instance from the device tree
 *               and link it to the provided rCPU structure.
 * @rcpu: Pointer to the rCPU structure containing the name and compatible
 *        information of the remote processor to fetch.
 *
 * This function uses the information in the provided rCPU structure to locate
 * a specific remote processor (rproc) in the device tree. If a match is found, 
 * it fetches the rproc pointer and links it to the rCPU structure.
 * 
 * Once the rproc is successfully fetched, the function ensures that the rCPU
 * is detached and stopped if it is currently attached or running.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int fetch_rproc(struct rcpu_info *rcpu)
{
	struct device_node *dev_node;
	struct platform_device *pdev;
	struct device *dev;
	int found = 0;
	int err = 0;

	pr_info("Fetching rproc for rCPU %s.\n", rcpu->name);
	
	/* Find the device node in the device tree */
	dev_node = of_find_compatible_node(NULL, NULL, rcpu->compatible);
	if (!dev_node) {
		pr_err("Failed to find device node\n");
		err = -ENODEV;
		goto out;
	}	

	/*
	 * Check the name to be sure there is only one rCPU otherwise 
	 * try fetching the rproc from the child nodes of the dev_node
	 */
	if (strcmp(dev_node->name, rcpu->name) == 0) {

		/* Fetch rproc pointer from device */
		err = fetch_rproc_from_device(dev_node, rcpu);
		if (err < 0) {
			pr_err("Failed to fetch rproc for rCPU %s\n", rcpu->name);
			goto out;
		}
		found = 1;
	} else {

		/* Find the platform device */
		pdev = of_find_device_by_node(dev_node);
		if (!pdev) {
			pr_err("Failed to find platform device\n");
			err = -ENODEV;
			goto out;
		}	
		dev = &pdev->dev;
		
		/* Fetch rproc pointer from child nodes */
		for_each_available_child_of_node(dev->of_node, dev_node) {	
			if (strcmp(dev_node->name, rcpu->name) == 0) {
				err = fetch_rproc_from_device(dev_node, rcpu);
				if (err < 0) {
					pr_err("Failed to fetch rproc for rCPU %d\n", rcpu->id);
					goto out;
				}
				found = 1;
				break;
			}	
		}
	}

	if (!found) {
		pr_err("Failed to find rCPU %s\n", rcpu->name);
		err = -ENODEV;
		goto out;
	}
	
	/* Detach the rcpu if it is attached */
	err = detach_rcpu_state(rcpu);
	if (err < 0) {
		goto out;
	}

	/* Stop the rcpu if it is running */
	err = stop_rcpu_state(rcpu);
	if (err < 0) {
		goto out;
	}

out:
	return err;	
}

/**
 * setup_rcpu - Setup the rCPU from the cell device description
 * @rcpu_array: Array of rCPU structures where the rCPU to setup is stored
 * @rcpu_id: Position in the array of the rCPU to setup
 * @rcpu_devices: Array of devices in the cell configuration corresponding to the rCPUs
 * @rcpu_device_id: Specific position in the rcpu_devices array of the device to check
 *
 * This function allocates memory for the rCPU structure at the specified position
 * in the rcpu_array. It retrieves the ID, name, and compatible driver information
 * from the cell configuration (rcpu_devices) and assigns them to the rCPU structure.
 * Additionally, it fetches the rCPU from the device tree to link it to an existing
 * remoteproc instance and obtain further information.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
// Setup the rCPU from the cell device description
static int setup_rcpu(struct rcpu_info **rcpu_array, unsigned int rcpu_id, 
				const struct jailhouse_rcpu_device *rcpu_devices, unsigned int rcpu_device_id)
{
	int err = 0;

	/* Allocate memory for the rproc struct */
	rcpu_array[rcpu_id] = vmalloc(sizeof(struct rcpu_info));
	if (!rcpu_array[rcpu_id]) {
		pr_err("Failed to allocate memory for rCPU %d\n", rcpu_id);
		err = -ENOMEM;
		goto out;
	}

	/* Load the ID, the name, and the compatible from the cell configuration */
	rcpu_array[rcpu_id]->id = rcpu_devices[rcpu_device_id].rcpu_id;
	strncpy(rcpu_array[rcpu_id]->name, rcpu_devices[rcpu_device_id].name, JAILHOUSE_RCPU_IMAGE_NAMELEN);
	strncpy(rcpu_array[rcpu_id]->compatible, rcpu_devices[rcpu_device_id].compatible, JAILHOUSE_RCPU_IMAGE_NAMELEN);
	pr_info("rCPU id: %d, name: %s, compatible: %s\n", rcpu_array[rcpu_id]->id, 
							rcpu_array[rcpu_id]->name, rcpu_array[rcpu_id]->compatible);

	/* Find the device node in the device tree */
	err = fetch_rproc(rcpu_array[rcpu_id]);
	if (err < 0) {
		pr_err("Failed to fetch rproc for rCPU %d\n", rcpu_array[rcpu_id]->id);
		goto free_rcpu;
	}
	pr_info("rCPU %s fetched\n", rcpu_array[rcpu_id]->rproc->name);
	goto out;

free_rcpu:
	vfree(rcpu_array[rcpu_id]);
	rcpu_array[rcpu_id] = NULL;

out:
	return err;
}


/**
 * jailhouse_root_rcpus_setup - Setup the rcpus for the jailhouse root-cell
 * @cell: Pointer to the cell structure (must be the root-cell).
 * @config: Pointer to the jailhouse cell description structure.
 *
 * This function sets up the remote CPUs (rcpus) for the jailhouse root-cell.
 * It validates the root-cell configuration to ensure that the requested remote
 * processors are available. If the configuration is valid, it allocates memory
 * for all the ASIC rcpus (excluding soft-core processors) and initializes them
 * in the cell. The initialization includes setting the ID, name, and compatible
 * driver for each core.
 *
 * Return:
 * 0 on success, or a negative error code on failure.
 */
int jailhouse_root_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
	const struct jailhouse_rcpu_device *rcpu_devices;
	unsigned int rcpu_id;
	unsigned int rcpu_device_id;
	int device_found = 0;
	int err = 0;

	if (cell->id != 0) {
		pr_err("Only root cell should call this function\n");
		err = -EINVAL;
		goto out;
	}

	/* Check configuration for the root cell: rcpus assigned and rcpus devices must match */
	if (cpumask_weight(&cell->rcpus_assigned) != config->num_rcpu_devices) {
		pr_err("Number of assigned rcpus %d is different from the number of rcpus devices %d\n",
			   cpumask_weight(&cell->rcpus_assigned), config->num_rcpu_devices);
		err = -EINVAL;
		goto out;
	}
	num_root_rcpus = config->num_rcpu_devices;

	/* Allocate memory needed for remote processors */
	root_rcpus_info = vmalloc(config->num_rcpu_devices * sizeof(struct rcpu_info *));
	if (!root_rcpus_info) {
		pr_err("Failed to allocate memory for rcpu_info\n");
		err = -ENOMEM;
		goto out;
	}

	/* Get the rcpus devices info from the cell configuration */
	rcpu_devices = jailhouse_cell_rcpu_devices(config);

	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {

		/* Check configuration: the rcpus id must be correctly assigned in the devices */
		device_found = 0;
		for (rcpu_device_id = 0; rcpu_device_id < num_root_rcpus; rcpu_device_id++) {
			if (rcpu_id == rcpu_devices[rcpu_device_id].rcpu_id) {
				pr_info("rCPU %d found as a device in the configuration\n", rcpu_id);
				device_found = 1;
				break;
			}
		}
		if (!device_found) {
			pr_err("Missing rCPU %d in the configuration\n", rcpu_id);
			err = -EINVAL;
			goto free_rcpus;
		}

		/* Setup the rCPU ID, name, and compatible from the cell device description */ 
		err = setup_rcpu(root_rcpus_info, rcpu_id, rcpu_devices, rcpu_device_id);
		if (err < 0) {
			pr_err("Failed to setup rCPU %d\n", rcpu_id);
			goto free_rcpus;
		}
	}

	goto out;

free_rcpus:
	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {
		if (root_rcpus_info[rcpu_id]) {
			vfree(root_rcpus_info[rcpu_id]);
			root_rcpus_info[rcpu_id] = NULL;
		}
	}
	vfree(root_rcpus_info);
	root_rcpus_info = NULL;

out:
	return err;
}

/**
 * jailhouse_rcpus_setup - Set up the rCPUs of a cell based on the configuration.
 * 
 * @cell: Pointer to the cell structure representing the new cell (not the root cell).
 * @config: Pointer to the jailhouse_cell_desc structure containing the cell's configuration.
 * 
 * This function configures the rCPUs (remote CPUs) assigned to a cell by processing the
 * configuration provided. It distinguishes between ASIC rCPUs (hardware-based) and 
 * soft-core rCPUs (software-based) and performs the following steps:
 * 
 * 1. For ASIC rCPUs:
 *    - Checks if the rCPU is already assigned to a different cell. If so, it logs an error
 *      and returns an error code.
 *    - If the rCPU is part of the root cell, it removes the rCPU from the root cell's 
 *      assigned rCPUs and assigns it to the new cell.
 * 
 * 2. For soft-core rCPUs:
 *    - Verifies if the rCPU is listed as a device in the configuration. 
 *    - Calculates the soft-core rCPU ID starting from 0 (the soft-core IDs are per-cell).
 *    - Sets up the soft-core rCPU using the cell's device description, including its ID,
 *      name, and compatible driver. 
 * 
 * Return:
 * 0 on success, or a negative error code on failure.
 */
int jailhouse_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
	const struct jailhouse_rcpu_device *rcpu_devices;
	unsigned int rcpu_id;
	unsigned int soft_rcpu_id;
	unsigned int soft_rcpu_device_id;
	int device_found = 0;
	int err = 0;

	/* Get the rcpus devices info from the configuration */
	rcpu_devices = jailhouse_cell_rcpu_devices(config);
	
	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {

		/* Distinguish between asic rcpus and soft-core rcpus */
		if (rcpu_id < num_root_rcpus) {

			/* Check if the region is already assigned to a different cell */
			if (!cpumask_test_cpu(rcpu_id, &root_cell->rcpus_assigned)) {
				pr_err("ERROR: rCPU %d is already assigned to a different cell\n", rcpu_id);
				err = -EINVAL;
				goto out;
			}

			/* Remove the rcpu from the root cell (they are already initialized by the rootcell) */
			pr_info("Removing rCPU %d from rootcell.\n", rcpu_id);
			cpumask_clear_cpu(rcpu_id, &root_cell->rcpus_assigned);
		} else {
			device_found = 0;
			for (soft_rcpu_device_id = 0; soft_rcpu_device_id < config->num_rcpu_devices; soft_rcpu_device_id++) {
				if (rcpu_id == rcpu_devices[soft_rcpu_device_id].rcpu_id) {
					pr_info("soft-rCPU %d found as a device in the configuration\n", rcpu_id);
					device_found = 1;
					break;
				}
			}
			if (!device_found) {
				pr_err("ERROR: soft-rCPU %d not found in the configuration\n", rcpu_id);
				err = -EINVAL;
				goto out;
			}

			/* Calculate the id of the soft-core rcpu starting from 0 */
			soft_rcpu_id = rcpu_id - num_root_rcpus;

			/* Setup the rCPU from the cell device description */
			err = setup_rcpu(cell->soft_rcpus_info, soft_rcpu_id, rcpu_devices, soft_rcpu_device_id);
			if (err < 0) {
				pr_err("Failed to setup soft-rCPU %d\n", soft_rcpu_id);
				goto out;
			}	
		}
	}

out:
	return err;
}

/**
 * jailhouse_load_rcpu_image - Load the firmware image for a remote CPU (rCPU).
 * @cell: Pointer to the cell structure where the rCPU is assigned.
 * @rcpu_uimage: Pointer to the user-space structure containing the rCPU image information.
 *
 * This function loads the firmware image for a remote CPU (rCPU) specified in the
 * user-space structure. It checks if the rCPU ID is valid, verifies if the image file
 * exists, and then loads the firmware via remoteproc.
 *
 * Return:
 * 0 on success, or a negative error code on failure.
 */
int jailhouse_load_rcpu_image(struct cell *cell, struct jailhouse_preload_rcpu_image __user *rcpu_uimage){
	struct jailhouse_preload_rcpu_image rcpu_image;
	struct rcpu_info *rcpu;
	int rcpu_id;
	int err = 0;

	if (copy_from_user(&rcpu_image, rcpu_uimage, sizeof(rcpu_image))) {
		err = -EFAULT;
		goto out;
	}

	rcpu_id = rcpu_image.rcpu_id; // starts from 0

	/* Check if the rcpu id is in the assigned rcpus */
	pr_info("Checking rcpu ID %d\n", rcpu_id);
	if (!cpumask_test_cpu(rcpu_id, &cell->rcpus_assigned)) {
		pr_err("rcpu ID %d not valid\n", rcpu_id);
		err = -EINVAL;
		goto out;
	}

	/* Check if the image file exists */
	pr_info("Checking Image file %s\n", rcpu_image.name);
	err = check_image(rcpu_image.name);
	if (err < 0) {
		pr_err("Image file %s not found\n", rcpu_image.name);
		goto out;
	}

	if (rcpu_id < num_root_rcpus) {
		rcpu = root_rcpus_info[rcpu_id];
	} else {
		rcpu = cell->soft_rcpus_info[rcpu_id - num_root_rcpus];
	}

	pr_info("Loading Firmware %s on rCPU %d\n", rcpu_image.name, rcpu_id);
	
	/* Set firmware name in the rproc struct */
	err = rproc_set_firmware(rcpu->rproc, rcpu_image.name);
	if (err < 0) {
		pr_err("Failed to set firmware for rcpu %d\n", rcpu_id);
		goto out;
	}

	/* Load the firmware via remoteproc */
	err = load_rcpu_firmware(rcpu);
	if (err < 0) {
		pr_err("Failed to load firmware for rcpu %d\n", rcpu_id);
		goto out;
	}

	pr_info("Loaded Firmware %s on rCPU %d\n", rcpu_image.name, rcpu_id);

out:
	return err;
}

/**
 * jailhouse_start_rcpu - Start the rCPUs assigned to a given cell.
 * 
 * @cell: Pointer to the cell structure containing the rCPUs to be started.
 * 
 * This function iterates over the rCPUs assigned to the specified cell and 
 * starts each of them. It distinguishes between ASIC rCPUs (root rCPUs) and 
 * soft-core rCPUs. For root rCPUs, it uses the global `root_rcpus_info` array, 
 * while for soft-core rCPUs, it uses the `soft_rcpus_info` array within the 
 * cell structure.
 * 
 * Return:
 *  0 on success, or a negative error code if starting any rCPU fails.
 */
int jailhouse_start_rcpu(struct cell *cell) {
	unsigned int rcpu_id;
	unsigned int soft_rcpu_id;
	int err = 0;

	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {
		pr_info("Starting rcpus %d of cell %d\n", rcpu_id, cell->id);

		/* distinguish between asic rcpus and soft-core rcpus */
		if (rcpu_id < num_root_rcpus) {
			err = start_rcpu_state(root_rcpus_info[rcpu_id]);
			if (err < 0) {
				pr_err("Failed to start rCPU %d\n", rcpu_id);
			}
		} else {
			soft_rcpu_id = rcpu_id - num_root_rcpus;
			err = start_rcpu_state(cell->soft_rcpus_info[soft_rcpu_id]);
			if (err < 0) {
				pr_err("Failed to start soft-rCPU %d\n", rcpu_id);
			}
		}
	}

	return err;
}

/**
 * jailhouse_root_rcpus_remove - Release and deallocate the rproc instances for ASIC rCPUs.
 *
 * This function is responsible for cleaning up and releasing resources associated with
 * the root rCPU instances. It iterates through the assigned rCPUs, releases their
 * associated remote processor (rproc) instances, and frees the memory allocated for
 * their information structures. Finally, it deallocates the root_rcpus_info array.
 *
 * Return: 0 on success, or a negative error code (-EINVAL) if any rCPU has a NULL
 *         rproc instance.
 */
int jailhouse_root_rcpus_remove()
{
	unsigned int rcpu_id;
	int err = 0;

	/* Check if the root_rcpus_info array is NULL */
	if (root_rcpus_info == NULL)
		goto out;

	for_each_rcpu(rcpu_id, &root_cell->rcpus_assigned) {
		pr_info("Releasing rCPU %s\n", root_rcpus_info[rcpu_id]->name);

		/* Check if the rproc is NULL */
		if (root_rcpus_info[rcpu_id]->rproc == NULL) {
			pr_err("rCPU %s has a NULL rproc instance\n", root_rcpus_info[rcpu_id]->name);
			err = -EINVAL;
			continue;
		}

		/* Remove the rproc instance */
		rproc_put(root_rcpus_info[rcpu_id]->rproc);

		/* Free the memory for the rCPU info */
		if (root_rcpus_info[rcpu_id] != NULL) {
			vfree(root_rcpus_info[rcpu_id]);
			root_rcpus_info[rcpu_id] = NULL;
		}
	}

	/* Free the root_rcpus_info array */
	vfree(root_rcpus_info);
	root_rcpus_info = NULL;

out:
	return err;
}

/**
 * jailhouse_rcpus_remove - Release and deallocate rproc instances for soft rCPUs
 *                          and return ASIC rCPUs to the root cell.
 * @cell: Pointer to the cell structure from which rCPUs are to be removed.
 *
 * This function handles the removal of rCPUs assigned to a given cell. It
 * distinguishes between ASIC rCPUs and soft-core rCPUs and performs the
 * necessary operations to either return ASIC rCPUs to the root cell or
 * deallocate soft-core rCPUs. The steps include:
 * 
 * - For ASIC rCPUs:
 *   - Check if the rproc instance is NULL.
 *   - Detach and stop the rCPU state.
 *   - Reassign the rCPU to the root cell.
 * 
 * - For soft-core rCPUs:
 *   - Check if the rproc instance is NULL.
 *   - Detach and stop the rCPU state.
 *   - Deallocate the rproc instance and free the associated memory.
 * 
 * Additionally, the function frees the array of soft rCPU information if it
 * exists.
 * 
 * Return:
 * 0 on success, or a negative error code on failure. 
 */
int jailhouse_rcpus_remove(struct cell *cell) {
	int err = 0;
	unsigned int rcpu_id;
	unsigned int soft_rcpu_id;

	for_each_rcpu(rcpu_id, &cell->rcpus_assigned) {
		pr_info("Removing rcpus %d from cell %d\n", rcpu_id, cell->id);

		/* Distinguish between ASIC rcpus and soft-core rcpus */
		if (rcpu_id < num_root_rcpus) {

			/* Check if the rproc is NULL */
			if (root_rcpus_info[rcpu_id]->rproc == NULL) {
				pr_err("rCPU %d has a NULL rproc instance\n", rcpu_id);
				err = -EINVAL;
				goto out;
			}

			/* Check if the rcpu is already offline */
			err = detach_rcpu_state(root_rcpus_info[rcpu_id]);
			if (err < 0) {
				pr_err("Failed to detach rCPU %d\n", rcpu_id);
				goto out;
			}

			err = stop_rcpu_state(root_rcpus_info[rcpu_id]);
			if (err < 0) {
				pr_err("Failed to stop rCPU %d\n", rcpu_id);
				goto out;
			}

			/* Reassign the rcpu to the root cell */
			cpumask_set_cpu(rcpu_id, &root_cell->rcpus_assigned);
		} else {
			soft_rcpu_id = rcpu_id - num_root_rcpus;

			/* Check if the rproc is NULL */
			if (cell->soft_rcpus_info[soft_rcpu_id]->rproc == NULL) {
				pr_err("Soft-rCPU %d has a NULL rproc instance\n", soft_rcpu_id);
				err = -EINVAL;
				goto out;
			}

			/* Check if the rcpu is already offline */
			err = detach_rcpu_state(cell->soft_rcpus_info[soft_rcpu_id]);
			if (err < 0) {
				pr_err("Failed to detach soft-rCPU %d\n", soft_rcpu_id);
				goto out;
			}

			err = stop_rcpu_state(cell->soft_rcpus_info[soft_rcpu_id]);
			if (err < 0) {
				pr_err("Failed to stop soft-rCPU %d\n", soft_rcpu_id);
				goto out;
			}

			/* Deallocate the rproc instance */
			pr_info("Releasing soft-rCPU %s\n", cell->soft_rcpus_info[soft_rcpu_id]->name);
			rproc_put(cell->soft_rcpus_info[soft_rcpu_id]->rproc);
			vfree(cell->soft_rcpus_info[soft_rcpu_id]);
		}
	}

	/* Free the soft rproc array */
	if (cell->soft_rcpus_info != NULL) {
		vfree(cell->soft_rcpus_info);
		cell->soft_rcpus_info = NULL;
	}

out:
	return err;
}

#endif /* CONFIG_OMNIVISOR */