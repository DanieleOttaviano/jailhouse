/*
* Jailhouse, a Linux-based partitioning hypervisor
*
* Omnivisor demo RISC-V Makefile
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

static long fpga_flags; 

void setup_fpga_flags(long flags)
{
	if(flags &= JAILHOUSE_FPGA_PARTIAL){
		fpga_flags  = FPGA_MGR_PARTIAL_RECONFIG;
	} else {
		fpga_flags = 0; //indicates full reconfiguration 
	}
	//DEBUG
	//pr_info("flags is currently %d\n",*flags);
	//TODO: consider other options!
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
	// the root cell load bitstream with void regions
	setup_fpga_flags(JAILHOUSE_FPGA_FULL);
	pr_info("Setting up FPGA flags: %ld\n", flags);
	return 	load_bitstream(0,bitstream_name);
	setup_fpga_flags(flags);
}

//load a bitstream (FULL if rootcell, PARTIAL if not)
int load_bitstream(unsigned int region_id, const char *bitstream_name)
{
    struct fpga_image_info* info;
    struct fpga_region * fpga_region;
    char name[10];
	unsigned int len;
    int err = 0;
	//ktime_t start, end;
	//s64 duration_ns;	


    sprintf(name,"region%d",region_id);
	// find the fpga region device in the sysfs
    fpga_region = fpga_region_class_find(NULL,name,device_match_name);
    if(!fpga_region){
        pr_err("jailhouse: region %d not found\n",region_id);
        return -ENODEV;
    }

	// Allocate memory for info structure
	info = fpga_image_info_alloc(&fpga_region->dev);
	if (!info)
		return -ENOMEM;
	//fpga_region->mgr  
	// int fpga_mgr_load(struct fpga_manager *mgr, struct fpga_image_info *info);
	info->flags = fpga_flags;

	info->firmware_name = devm_kstrdup(&fpga_region->dev, bitstream_name,  GFP_KERNEL);
	//debug
	pr_info("jailhouse: firmware name is %s\n",info->firmware_name);
	//Remove terminating '\n'
	len = strlen(info->firmware_name);
	if (info->firmware_name[len - 1] == '\n') 
		info->firmware_name[len - 1] = 0;

	// Add info to region and do the programming
	fpga_region->info = info;
    //start = ktime_get();
	err = fpga_region_program_fpga(fpga_region);
	//end = ktime_get();
	//duration_ns = ktime_to_ns(ktime_sub(end, start));
	//pr_err("%s;%lld",info->firmware_name,duration_ns);

	/* Deallocate the image info if you're done with it */
	fpga_region->info = NULL;
	fpga_image_info_free(info);

    if(fpga_region->mgr->state != FPGA_MGR_STATE_OPERATING){
        pr_err("jailhouse: programing region %d failed. FPGA not operating\n",region_id);
        return -ENODEV;
    }

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

	/* Get the file size */
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

	// Free the allocated memory for the overlay data
	vfree(overlay_data);

	// Close the file
	filp_close(file, NULL);

	return 0;
}


int jailhouse_fpga_regions_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
	const struct jailhouse_fpga_device *devices;
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
			pr_info("Loading bitstream %s in FPGA region %d.\n", devices[device_id].fpga_bitstream ,region_id);
			// to do ... complete with dynamic function exchange
			err = load_bitstream(region_id + 1, devices[region_id].fpga_bitstream);
			if (err){
				pr_err("Failed to load bitstream %s in FPGA region %d\n", devices[device_id].fpga_bitstream, region_id);	
				continue;
			}
			
			// if needed insert the device tree overlay
			pr_info("Inserting device tree overlay %s\n", devices[device_id].fpga_dto);
			if(devices[device_id].fpga_dto[0] != '\0'){
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
				pr_info("loading %s module...\n", devices[device_id].fpga_module);
				__request_module(true, devices[region_id].fpga_module);
			}

		}
		else{
			if(cell->id != 0)
				pr_err("Assigned region %d doesn't match with any fpga_device.\n",region_id);
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
		// for each region, remove the kernel module (not possible for security reason)

		// remove the device tree overlay (if needed)
		if(cell->fpga_overlay_ids[region_id] != -1){	
			if (of_overlay_remove(&cell->fpga_overlay_ids[region_id])) {
				pr_err("Failed to remove overlay\n");
				err = -EINVAL;
			}
		}

		// give back the region to the rootcell
		pr_info("Removing FPGA region %d.\n", region_id);
		// to do: Program the region with void bitstream
		cpumask_set_cpu(region_id, &root_cell->fpga_regions_assigned);
	}

	kfree(cell->fpga_overlay_ids);

	return err;
}

#endif /* CONFIG_OMNV_FPGA */