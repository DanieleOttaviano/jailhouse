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

void setup_fpga_flags(long flags)
{
	if(flags &= JAILHOUSE_FPGA_PARTIAL){
		fpga_flags  = FPGA_MGR_PARTIAL_RECONFIG;
	} else {
		fpga_flags = 0; //indicates full reconfiguration 
	}
	// TODO: Daniele Ottaviano 
	// consider other options!
	/*
	if (encrypted with device key)
	fpga_flags |= FPGA_MGR_ENCRYPTED_BITSTREAM;
	else if (encrypted with user key)
	fpga_flags |= FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM;

	if (ddr bitstream authentication)
	fpga_flags |= FPGA_MGR_DDR_MEM_AUTH_BITSTREAM;
	else if (secure memory bitstream authentication)
	fpga_flags |= FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM*/

	//compressed bitstream, LSB first bitstream are only for Altera FPGAs
}

int load_root_bitstream(char *bitstream_name, long flags)
{
	int err = 0;

	// the root cell load bitstream with void regions
	setup_fpga_flags(flags);
	err = load_bitstream(0,bitstream_name);
	if (err){
		pr_err("jailhouse: failed to load root bitstream\n");
	}

	// TODO: Daniele Ottaviano
	// Consider to do and & with the flags selected by the rootcell config
	setup_fpga_flags(JAILHOUSE_FPGA_PARTIAL);
	return err;
}

int load_bitstream(unsigned int region_id, const char *bitstream_name)
{
    struct fpga_image_info* info;
    struct fpga_region * fpga_region;
    char name[10];
	unsigned int len;
    int err = 0;
	//ktime_t start, end;
	//s64 duration_ns;	

	// find the fpga region device in the sysfs
    sprintf(name,"region%d",region_id);
    fpga_region = fpga_region_class_find(NULL,name,device_match_name);
    if(!fpga_region){
        pr_err("jailhouse: region %d not found\n",region_id);
        return -ENODEV;
    }

	// Allocate memory for info structure
	info = fpga_image_info_alloc(&fpga_region->dev);
	if (!info){
		pr_err("jailhouse: failed to allocate memory for bitstream info\n");
		return -ENOMEM;
	}
	
	// Set the flags (e.g. partial reconfiguration)
	info->flags = fpga_flags;
	fpga_region->mgr->flags = fpga_flags;

	// Allocate space for the bitstream name and copy it.
	info->firmware_name = devm_kstrdup(&fpga_region->dev, bitstream_name,  GFP_KERNEL);
	if (!info->firmware_name) {
		pr_err("jailhouse: failed to allocate memory for bitstream name\n");
    	err = -ENOMEM;
		goto out;
    }

	//Remove terminating '\n'
	len = strlen(info->firmware_name);
	if (info->firmware_name[len - 1] == '\n') 
		info->firmware_name[len - 1] = 0;

	// Add info to region and do the programming
	fpga_region->info = info;
    //start = ktime_get();
	err = fpga_region_program_fpga(fpga_region);
	if(err){
		pr_err("jailhouse: programing region %d failed\n",region_id);
		goto out;
	}
	//end = ktime_get();
	//duration_ns = ktime_to_ns(ktime_sub(end, start));
	//pr_err("%s;%lld",info->firmware_name,duration_ns);

    if(fpga_region->mgr->state != FPGA_MGR_STATE_OPERATING){
        pr_err("jailhouse: programing region %d failed. FPGA not operating\n",region_id);
        err = -1;
		goto out;
    }

out:
	/* Deallocate the image info if you're done with it */
	fpga_region->info = NULL;
	fpga_image_info_free(info);

	return err;
}


int apply_overlay(unsigned int * overlay_id, const char* dto_name)
{
	struct file *file;
	char path[256];
	loff_t size;
	void* overlay_data = NULL;
	ssize_t read_size;

	// Initialize the overlay id
	*overlay_id = -1;

	// Concatenate the dto_name to the path /lib/firmware/
	snprintf(path, sizeof(path), "/lib/firmware/%s", dto_name);

	// Open the file
	file = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(file)) {
		pr_err("Failed to open file %s\n", path);
		return PTR_ERR(file);
	}

	// Get the file size
	size = vfs_llseek(file, 0, SEEK_END);
	if (size < 0) {
		pr_err("Failed to determine file size\n");
		filp_close(file, NULL);
		return -EIO;
	}
	pr_info("File size: %lld\n", size);
	
	// Allocate memory for the overlay data
	overlay_data = vmalloc(size);
	if (!overlay_data) {
		pr_err("Failed to allocate memory for overlay data\n");
		filp_close(file, NULL);
		return -ENOMEM;
	}

	// Reset file position to the beginning
	vfs_llseek(file, 0, SEEK_SET);

	// Read the file into the overlay data
	read_size = kernel_read(file, overlay_data, size, &file->f_pos);
	if (read_size < 0 || read_size != size) {
		pr_err("Failed to read file content.\n");
		vfree(overlay_data);
		filp_close(file, NULL);
		return -EIO;
	}

	// Apply the overlay
	if (of_overlay_fdt_apply(overlay_data, size, overlay_id)) {
		pr_err("Failed to apply device tree overlay\n");
		vfree(overlay_data);
		filp_close(file, NULL);
		return -EINVAL;
	}

	// Free the allocated memory for the overlay data and close the file
	vfree(overlay_data);
	filp_close(file, NULL);

	return 0;
}


int jailhouse_fpga_regions_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
	const struct jailhouse_fpga_device *devices;
	volatile unsigned long __iomem *fpga_start;
	unsigned int region_id;
	unsigned int device_id;
	int err = 0;
	int device_found = 0;

	// Allocate memory for the overlay ids
	cell->fpga_overlay_ids = kmalloc(sizeof(int) * config->num_fpga_devices, GFP_KERNEL);
	if (!cell->fpga_overlay_ids) {
		pr_err("Failed to allocate memory for overlay ids\n");
		return -ENOMEM;
	}

	devices = jailhouse_cell_fpga_devices(config);
	for_each_region(region_id, &cell->fpga_regions_assigned) {
		// for each devices, check if there is one with fpga_region_id equal to region_id.
		device_found = 0;
		for(device_id = 0; device_id < config->num_fpga_devices; device_id++) {
			if(region_id == devices[device_id].fpga_region_id) {
				device_found = 1;
				break;
			}
		}

		if(device_found){
			// Check if the region is already assigned to a different cell
			if (!cpumask_test_cpu(region_id, &root_cell->fpga_regions_assigned)) {
				pr_err("ERROR: region %d is already assigned to a different cell\n", region_id);
				return -1;
			}

			pr_info("Loading bitstream %s in FPGA region %d.\n", devices[device_id].fpga_bitstream ,region_id);

			// Map the physical FPGA configuration address to virtual address space
			fpga_start = ioremap(devices[device_id].fpga_conf_addr, sizeof(unsigned long));
			if (!fpga_start) {
				pr_err("Failed to map FPGA base address\n");
				return -ENOMEM;
			}

			// Disable the module (noop if not enable)
			iowrite8(0, fpga_start);
			// Initiate safe shutdown of the DFX region and wait for it to complete
			iowrite8(1, (uint8_t *)fpga_start+1);
			while(true){
				if(ioread16(((uint16_t *)fpga_start + 1)) == 0x000F){
					break;
				}
			}

			// Load the bitstream of the region using dfx
			err = load_bitstream(region_id + 1, devices[region_id].fpga_bitstream);
			if (err){
				pr_err("Failed to load bitstream %s in FPGA region %d\n", devices[device_id].fpga_bitstream, region_id);	
				continue;
			}
			
			//Initiate safe connection of the DFX region and wait for it to complete
			iowrite8(0, (uint8_t *)fpga_start+1);
			while(true){
				if(ioread16(((uint16_t *)fpga_start + 1)) == 0x0){
					break;
				}
			}
			iounmap(fpga_start);

			// if needed load the device tree overlay
			if(devices[device_id].fpga_dto[0] != '\0'){
				pr_info("Loading device tree overlay: %s\n", devices[device_id].fpga_dto);
				err = apply_overlay(&cell->fpga_overlay_ids[device_id], devices[device_id].fpga_dto);
				if(err){
					pr_err("Failed to apply device tree overlay %s\n", devices[device_id].fpga_dto);
					continue;
				}
			}
			else{
				cell->fpga_overlay_ids[device_id] = -1;
			}

			// if needed and not already loaded, load the module and initialize it
			if(devices[device_id].fpga_module[0] != '\0'){
				pr_info("Loading module: %s\n", devices[device_id].fpga_module);
				__request_module(true, devices[region_id].fpga_module);
			}

		}
		else{
			// TODO: Daniele Ottaviano
			// Manage the error here
			if(cell->id != 0){
				pr_err("Assigned region %d doesn't match with any fpga_device. Check the cell configuration.\n",region_id);
			}
		}
		
		// if cell is not rootcell remove the assigned regions from the rootcell
		if(cell->id != 0){
			pr_info("Removing FPGA region %d from rootcell.\n", region_id);
			cpumask_clear_cpu(region_id, &root_cell->fpga_regions_assigned);
		}
	}
	
	return err;
}

int jailhouse_fpga_regions_remove(struct cell *cell)
{
	unsigned int region_id;
	int err = 0;

	for_each_region(region_id, &cell->fpga_regions_assigned) {
		// TODO: Daniele Ottaviano
		// remove the kernel module (not possible for security reason)

		// remove the device tree overlay (if needed)
		if(cell->fpga_overlay_ids[region_id] != -1){	
			// TODO: Daniele Ottaviano
			// Solve the OF memory leak error
			if (of_overlay_remove(&cell->fpga_overlay_ids[region_id])) {
				pr_err("Failed to remove overlay\n");
				err = -EINVAL;
			}
		}

		// TODO: Daniele Ottaviano
		// The module stop the FPGA accelerator
		// but it would be safe to have here something that clear the FPGA region.

		// Reassign the region to the root cell
		cpumask_set_cpu(region_id, &root_cell->fpga_regions_assigned);
	}

	kfree(cell->fpga_overlay_ids);

	return err;
}

#endif /* CONFIG_OMNV_FPGA */