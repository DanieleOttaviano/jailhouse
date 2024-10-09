#if defined(CONFIG_FPGA)
#include <linux/fpga/fpga-mgr.h>
#include <linux/fpga/fpga-region.h>

#include <jailhouse/hypercall.h>
#include "fpga.h"


int jailhouse_fpga_create(struct jailhouse_fpga_create* fpga_create_args)
{
    //argomenti per la create. Il nome, il numero della regione
    int ret;
    struct region *region;
    region = (struct region*)kzalloc(sizeof(struct region), GFP_KERNEL);
    region->id = regifpga_create_args->region_id;
    strcpy(&region->name, &fpga_create_args->region_name, JAILHOUSE_CELL_ID_NAMELEN);
    region->state = FPGA_MGR_STATE_UNKNOWN;
    region->status = 0; 
    jailhouse_sysfs_region_create(region);
    ret = jailhouse_call_arg1(JAILHOUSE_HC_FPGA_CREATE, region_id);
    if(!ret){
        pr_err("Create for region %d failed\n",region_id);
        kfree(region);
        jailhouse_sysfs_region_destroy(region);
    }


    return ret;

}

int jailhouse_fpga_load(struct jailhouse_fpga_load* fpga_load_args)
{
    int ret;
    struct fpga_image_info* info;
    struct fpga_region * fpga_region;
    struct region *region;
    char name[10];
	char image_name[255];
	unsigned int len;
    unsigned int region_id = fpga_load_args->fpga_region;


    sprintf(name,"region%d",0);
    fpga_region = fpga_region_class_find(NULL,name,device_match_name);
    if(!fpga_region){
        pr_err("Region %d not found\n",region_id);
        return -ENODEV;
    }

    /* CONTROLLARE CHE LA REGIONE ESISRA: */
	info = fpga_image_info_alloc(&fpga_region->dev);
	if (!info)
		return -ENOMEM;

	info->flags = fpga_load_args->fpga_flags;


	info->firmware_name = devm_kstrdup(&fpga_region->dev, fpga_load_args->fpga_name,  GFP_KERNEL);
	len = strlen(info->firmware_name);
	if (info->firmware_name[len - 1] == '\n') //lose terminating '\n'
		info->firmware_name[len - 1] = 0;

	/* Add info to region and do the programming */
	region->info = info;
	ret = fpga_region_program_fpga(fpga_region);

	/* Deallocate the image info if you're done with it */
	fpga_region->info = NULL;
	fpga_image_info_free(info);

	
    /* controlla se è tutto ok 
        se si ritorna
        se no chiamata all'hypervisor per restituire la regione alla root cell. 
    */
    if(fpga_region->mgr->state != FPGA_MGR_STATE_OPERATING || ret){
        pr_err("Programming region %d failed\n",region_id);
        jailhouse_call_arg1(JAILHOUSE_HC_FPGA_DESTROY, region_id);
        return -ENODEV;
    }

   region = (struct region*)kzalloc(sizeof(struct region), GFP_KERNEL);
   region->id = region_id
   region->name = "Name!";
   region->state = fpga_region->mgr->state;
   region->status = 0; //get status!!!

   return ret;
}

#endif /* CONFIG_FPGA */