/*
* Jailhouse, a Linux-based partitioning hypervisor
*
* Omnivisor FPGA regions virtualizzation support
*
* Copyright (c) Daniele Ottaviano, 2024
*
* Authors:
*   Daniele Ottaviano <danieleottaviano97@gmail.com>
*	Anna Amodio <anna.amodio@studenti.unina.it>
*
* This work is licensed under the terms of the GNU GPL, version 2.  See
* the COPYING file in the top-level directory.
*
*/

#include "fpga.h"

#ifdef CONFIG_OMNV_FPGA
#include <linux/fpga/fpga-mgr.h>
#include <linux/fpga/fpga-region.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/vmalloc.h>
#include <linux/io.h>

static long fpga_flags; 

/**
 * setup_fpga_flags - Set up FPGA flags for loading bitstreams.
 * @flags: Flags for loading bitstreams (e.g., partial reconfiguration).
 * 	
 * This function sets up the FPGA flags based on the provided
 * flags. It determines whether to use partial reconfiguration
 * or full reconfiguration based on the input flags.
 * 
 * Return: None.
 */
static void setup_fpga_flags(long flags)
{
	if(flags &= JAILHOUSE_FPGA_PARTIAL){
		fpga_flags  = FPGA_MGR_PARTIAL_RECONFIG;
	} else {
		/*0 indicates full reconfiguration */
		fpga_flags = 0;  
	}
	/* TODO: Daniele Ottaviano 
	 * consider other options!
	 *
	 * if (encrypted with device key)
	 * fpga_flags |= FPGA_MGR_ENCRYPTED_BITSTREAM;
	 * else if (encrypted with user key)
	 * fpga_flags |= FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM;
	 *
	 * if (ddr bitstream authentication)
	 * fpga_flags |= FPGA_MGR_DDR_MEM_AUTH_BITSTREAM;
	 * else if (secure memory bitstream authentication)
	 * fpga_flags |= FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM
	 * 
	 * compressed bitstream, LSB first bitstream are only for Altera FPGAs
	 */
}

/** 
 * load_bitstream - Load a bitstream into the specified FPGA region.
 * @region_id: ID of the FPGA region to load the bitstream into.
 * @bitstream_name: Name of the bitstream file to load.
 * 
 * This function loads a bitstream into the specified FPGA region by
 * finding the region device in sysfs, allocating memory for the
 * bitstream info structure, and programming the region with the
 * bitstream.
 * 
 * Return: 0 on success, or a negative error code on failure.
 */
static int load_bitstream(unsigned int region_id, const char *bitstream_name, long flags)
{
	struct fpga_image_info *info = NULL;
	struct fpga_region *fpga_region = NULL;
	char name[10];
	unsigned int len;
	int err = 0;

	/* Set up the FPGA flags */
	setup_fpga_flags(flags);

	/* Find the FPGA region device in the sysfs */
	sprintf(name, "region%d", region_id);
	fpga_region = fpga_region_class_find(NULL, name, device_match_name);
	if (!fpga_region) {
		pr_err("Region %d not found\n", region_id);
		err = -ENODEV;
		goto out;
	}

	/* Allocate memory for info structure */
	info = fpga_image_info_alloc(&fpga_region->dev);
	if (!info) {
		pr_err("Failed to allocate memory for bitstream info\n");
		err = -ENOMEM;
		goto out;
	}

	/* Set the flags (e.g., partial reconfiguration) */
	info->flags = fpga_flags;
	fpga_region->mgr->flags = fpga_flags;

	/* Allocate space for the bitstream name and copy it */
	info->firmware_name = devm_kstrdup(&fpga_region->dev, bitstream_name, GFP_KERNEL);
	if (!info->firmware_name) {
		pr_err("Failed to allocate memory for bitstream name\n");
		err = -ENOMEM;
		goto cleanup_info;
	}

	/* Remove terminating '\n' */
	len = strlen(info->firmware_name);
	if (info->firmware_name[len - 1] == '\n') 
		info->firmware_name[len - 1] = 0;

	/* Add info to region and do the programming */
	fpga_region->info = info;
	err = fpga_region_program_fpga(fpga_region);
	if (err) {
		pr_err("Programming region %d failed\n", region_id);
		goto cleanup_info;
	}

	if (fpga_region->mgr->state != FPGA_MGR_STATE_OPERATING) {
		pr_err("Programming region %d failed. FPGA not operating\n", region_id);
		err = -EIO;
		goto cleanup_info;
	}

	goto out;

cleanup_info:
	if (info) {
		fpga_region->info = NULL;
		fpga_image_info_free(info);
	}

out:
	return err;
}

/** 
 * apply_overlay - Apply a device tree overlay to the system.
 * @overlay_id: Pointer to store the overlay ID.
 * @dto_name: Name of the device tree overlay file.
 *
 * This function applies a device tree overlay to the system by reading
 * the specified file and applying it using the of_overlay_fdt_apply()
 * function. The overlay ID is returned through the overlay_id pointer.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int apply_overlay(unsigned int *overlay_id, const char *dto_name)
{
	struct file *file;
	char path[256];
	loff_t size;
	void *overlay_data = NULL;
	ssize_t read_size;
	int err = 0;

	/* Initialize the overlay id */
	*overlay_id = -1;

	/* Concatenate the dto_name to the path /lib/firmware/ */
	snprintf(path, sizeof(path), "/lib/firmware/%s", dto_name);

	/* Open the file */
	file = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(file)) {
		pr_err("Failed to open file %s\n", path);
		err = PTR_ERR(file);
		goto out;
	}

	/* Get the file size */
	size = vfs_llseek(file, 0, SEEK_END);
	if (size < 0) {
		pr_err("Failed to determine file size\n");
		err = -EIO;
		goto close_file;
	}
	pr_info("File size: %lld\n", size);

	/* Allocate memory for the overlay data */
	overlay_data = vmalloc(size);
	if (!overlay_data) {
		pr_err("Failed to allocate memory for overlay data\n");
		err = -ENOMEM;
		goto close_file;
	}

	/* Reset file position to the beginning */
	vfs_llseek(file, 0, SEEK_SET);

	/* Read the file into the overlay data */
	read_size = kernel_read(file, overlay_data, size, &file->f_pos);
	if (read_size < 0 || read_size != size) {
		pr_err("Failed to read file content.\n");
		err = -EIO;
		goto free_overlay_data;
	}

	/* Apply the overlay */
	err = of_overlay_fdt_apply(overlay_data, size, overlay_id);
	if (err) {
		pr_err("Failed to apply device tree overlay\n");
		err = -EINVAL;
		goto free_overlay_data;
	}

free_overlay_data:
	vfree(overlay_data);
close_file:
	filp_close(file, NULL);
out:
	return err;
}

/**
 * jailhouse_root_bitstream - Load the root cell bitstream.
 * @bitstream_name: Name of the bitstream file to load.
 * @flags: Flags for loading the bitstream (e.g., partial reconfiguration).
 * 
 * This function loads the root cell bitstream into the FPGA region
 * by setting up the FPGA flags and calling the load_bitstream()
 * function.
 * 
 * Return: 0 on success, or a negative error code on failure.
 */
int jailhouse_root_bitstream(char *bitstream_name, long flags)
{
	int err = 0;

	/* The root cell load bitstream with void regions */
	err = load_bitstream(0,bitstream_name, flags);
	if (err){
		pr_err("jailhouse: failed to load root bitstream\n");
	}

	return err;
}

/**
 * jailhouse_fpga_regions_setup - Configures FPGA regions for a given cell
 *
 * @cell: Pointer to the cell structure representing the Jailhouse cell
 * @config: Pointer to the Jailhouse cell descriptor containing configuration
 *          details for the cell
 *
 * This function sets up the FPGA regions for the specified Jailhouse cell
 * based on the provided configuration. It ensures that the FPGA regions
 * are properly initialized and mapped for the cell's use.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
int jailhouse_fpga_regions_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
	const struct jailhouse_fpga_device *devices;
	volatile unsigned long __iomem *fpga_start;
	unsigned int region_id;
	unsigned int device_id;
	int err = 0;
	int device_found = 0;

	/* Allocate memory for the overlay ids */
	cell->fpga_overlay_ids = kmalloc(sizeof(int) * config->num_fpga_devices, GFP_KERNEL);
	if (!cell->fpga_overlay_ids) {
		pr_err("Failed to allocate memory for overlay ids\n");
		err = -ENOMEM;
		goto out;
	}

	devices = jailhouse_cell_fpga_devices(config);
	for_each_region(region_id, &cell->fpga_regions_assigned) {
		
		/* Check if there is one with fpga_region_id equal to region_id. */
		device_found = 0;
		for (device_id = 0; device_id < config->num_fpga_devices; device_id++) {
			if (region_id == devices[device_id].fpga_region_id) {
				device_found = 1;
				break;
			}
		}

		if (device_found) {
			/* Check if the region is already assigned to a different cell */
			if (!cpumask_test_cpu(region_id, &root_cell->fpga_regions_assigned)) {
				pr_err("Region %d is already assigned to a different cell\n", region_id);
				err = -EINVAL;
				goto release_overlay_ids;
			}

			pr_info("Loading bitstream %s in FPGA region %d.\n", devices[device_id].fpga_bitstream, region_id);

			/* Map the physical FPGA configuration address to virtual address space */
			fpga_start = ioremap(devices[device_id].fpga_conf_addr, sizeof(unsigned long));
			if (!fpga_start) {
				pr_err("Failed to map FPGA base address\n");
				err = -ENOMEM;
				goto release_overlay_ids;
			}

			/* Initiate safe shutdown of the DFX region and wait for it to complete */
			iowrite8(1, (uint8_t *)fpga_start + 1);
			while (true) {
				if (ioread16(((uint16_t *)fpga_start + 1)) == 0x0003) {
					break;
				}
			}

			/* Load the bitstream of the region using dfx */
			err = load_bitstream(region_id + 1, devices[device_id].fpga_bitstream, devices[device_id].fpga_flags);
			if (err) {
				pr_err("Failed to load bitstream %s in FPGA region %d\n", devices[device_id].fpga_bitstream, region_id);
				goto unmap_fpga;
			}

			/* Initiate safe connection of the DFX region and wait for it to complete */
			iowrite8(0, (uint8_t *)fpga_start + 1);
			while (true) {
				if (ioread16(((uint16_t *)fpga_start + 1)) == 0x0) {
					break;
				}
			}
			iounmap(fpga_start);

			/* If needed, load the device tree overlay */
			if (devices[device_id].fpga_dto[0] != '\0') {
				pr_info("Loading device tree overlay: %s\n", devices[device_id].fpga_dto);
				err = apply_overlay(&cell->fpga_overlay_ids[device_id], devices[device_id].fpga_dto);
				if (err) {
					pr_err("Failed to apply device tree overlay %s\n", devices[device_id].fpga_dto);
					goto release_overlay_ids;
				}
			} else {
				cell->fpga_overlay_ids[device_id] = -1;
			}

			/* If needed and not already loaded, load the module and initialize it */
			if (devices[device_id].fpga_module[0] != '\0') {
				pr_info("Loading module: %s\n", devices[device_id].fpga_module);
				err = __request_module(true, devices[device_id].fpga_module);
				if (err) {
					pr_err("Failed to load module %s\n", devices[device_id].fpga_module);
					goto release_overlay_ids;
				}
			}
		} else {
			if (cell->id != 0) {
				pr_err("Assigned region %d doesn't match with any fpga_device. Check the cell configuration.\n", region_id);
				err = -EINVAL;
				goto release_overlay_ids;
			}
		}

		/* If cell is not rootcell, remove the assigned regions from the rootcell */
		if (cell->id != 0) {
			pr_info("Removing FPGA region %d from rootcell.\n", region_id);
			cpumask_clear_cpu(region_id, &root_cell->fpga_regions_assigned);
		}
	}

unmap_fpga:
	if (err < 0)
		iounmap(fpga_start);
release_overlay_ids:
	if (err < 0) {
		kfree(cell->fpga_overlay_ids);
		cell->fpga_overlay_ids = NULL;
	}
out:
	return err;
}

/**
 * jailhouse_fpga_regions_remove - Remove FPGA regions from a cell
 *
 * @cell: Pointer to the cell structure representing the Jailhouse cell
 *
 * This function removes the FPGA regions assigned to the specified cell.
 * It cleans up the device tree overlays and reassigns the regions to the
 * root cell. It also handles the unloading of the FPGA kernel module
 * if necessary.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
int jailhouse_fpga_regions_remove(struct cell *cell)
{
	unsigned int region_id;
	int err = 0;

	for_each_region(region_id, &cell->fpga_regions_assigned) {
		/* TODO: Daniele Ottaviano
		 * remove the kernel module (not possible for security reason)
		 */

		/* remove the device tree overlay (if needed) */
		if(cell->fpga_overlay_ids[region_id] != -1){	
			/* TODO: Daniele Ottaviano
			 * Solve the OF memory leak error
			 */
			if (of_overlay_remove(&cell->fpga_overlay_ids[region_id])) {
				pr_err("Failed to remove overlay\n");
				err = -EINVAL;
			}
		}

		/* TODO: Daniele Ottaviano
		 * The module stop the FPGA accelerator
		 * but it would be safe to have here something that clear the FPGA region.
		 */

		/* Reassign the region to the root cell */
		cpumask_set_cpu(region_id, &root_cell->fpga_regions_assigned);
	}

	kfree(cell->fpga_overlay_ids);

	return err;
}

#endif /* CONFIG_OMNV_FPGA */