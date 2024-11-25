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

#include <linux/fpga/fpga-mgr.h>
#include <linux/fpga/fpga-region.h>

#include "fpga.h"


static long fpga_flags; 

static void set_flags(u32 *flags)
{
	if(fpga_flags &= JAILHOUSE_FPGA_PARTIAL){
		*flags  = FPGA_MGR_PARTIAL_RECONFIG;
	} else {
		*flags = 0; /* indicates full reconfiguration */
	}
	pr_info("flags is currently %d\n",*flags);
	//consider other options!
	/*
	if (encrypted with device key)
	*flags |= FPGA_MGR_ENCRYPTED_BITSTREAM;
	else if (encrypted with user key)
	*flags |= FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM;

	if (ddr bitstream authentication)
	*flags |= FPGA_MGR_DDR_MEM_AUTH_BITSTREAM;
	else if (secure memory bitstream authentication)
	*flags |= FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM;

	compressed bitstream, LSB first bitstream are only for Altera FPGAs
	*/
}


void setup_fpga_flags(long flags)
{
    fpga_flags = flags;
}

int load_bitstream(struct cell *cell, struct jailhouse_preload_bitstream __user *bitstream)
{
    int ret;
    struct fpga_image_info* info;
    struct fpga_region * fpga_region;
    char name[10];
	unsigned int len;
	unsigned int __user region_id = bitstream->region;
	ktime_t start, end;
	s64 duration_ns;

	//check if cell owns this region first.
	if(cell->fpga_regions_assigned & ((1U << region_id) == 0)){
		pr_err("Cell doesn't own region %d\n",region_id);
		return -EPERM;
	}

    sprintf(name,"region%d",region_id);
    fpga_region = fpga_region_class_find(NULL,name,device_match_name);
    if(!fpga_region){
        pr_err("Region %d not found\n",region_id);
        return -ENODEV;
    }

	info = fpga_image_info_alloc(&fpga_region->dev);
	if (!info)
		return -ENOMEM;

	set_flags(&info->flags);

	info->firmware_name = devm_kstrdup(&fpga_region->dev, bitstream->name,  GFP_KERNEL);
	//debug
	//pr_info("Firmware name is %s\n",info->firmware_name);
	len = strlen(info->firmware_name);
	if (info->firmware_name[len - 1] == '\n') //lose terminating '\n'
		info->firmware_name[len - 1] = 0;

	/* Add info to region and do the programming */
	fpga_region->info = info;
    start = ktime_get();
	ret = fpga_region_program_fpga(fpga_region);
	end = ktime_get();
	duration_ns = ktime_to_ns(ktime_sub(end, start));
	//pr_err("%s;%lld",info->firmware_name,duration_ns);

	/* Deallocate the image info if you're done with it */
	fpga_region->info = NULL;
	fpga_image_info_free(info);

    if(fpga_region->mgr->state != FPGA_MGR_STATE_OPERATING){
        pr_err("Programing region %d failed. FPGA not operating\n",region_id);
        return -ENODEV;
    }

   return ret;
}