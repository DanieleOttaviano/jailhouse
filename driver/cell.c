/*
 * Jailhouse, a Linux-based partitioning hypervisor
 * 
 * Copyright (c) Siemens AG, 2013-2016
 * Copyright (C) Daniele Ottaviano, 2024
 * 
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *  Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * dottavia, 2024-07-01: Omnivisor support
 * 
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* For compatibility with older kernel versions */
#include <linux/version.h>

#include <linux/bitops.h>
#include <linux/cpu.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/cacheflush.h>

#include "rcpu.h"
#include "fpga.h"
#include "cell.h"
#include "main.h"
#include "pci.h"
#include "sysfs.h"

#include <jailhouse/hypercall.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,7,0)
#define add_cpu(cpu)		cpu_up(cpu)
#define remove_cpu(cpu)		cpu_down(cpu)
#endif

struct cell *root_cell;

static LIST_HEAD(cells);
static cpumask_t offlined_cpus;


void jailhouse_cell_kobj_release(struct kobject *kobj)
{
	struct cell *cell = container_of(kobj, struct cell, kobj);

	jailhouse_pci_cell_cleanup(cell);
	vfree(cell->memory_regions);
	kfree(cell);
}

static struct cell *cell_create(const struct jailhouse_cell_desc *cell_desc)
{
	struct cell *cell;
	unsigned int id;
	int err;

	if (cell_desc->num_memory_regions >=
	    ULONG_MAX / sizeof(struct jailhouse_memory))
		return ERR_PTR(-EINVAL);

	/* determine cell id */
	id = 0;
retry:
	list_for_each_entry(cell, &cells, entry)
		if (cell->id == id) {
			id++;
			goto retry;
		}

	cell = kzalloc(sizeof(*cell), GFP_KERNEL);
	if (!cell)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&cell->entry);

	cell->id = id;

	bitmap_copy(cpumask_bits(&cell->cpus_assigned),
		    jailhouse_cell_cpu_set(cell_desc),
		    min((unsigned int)nr_cpumask_bits,
		        cell_desc->cpu_set_size * 8));

	bitmap_copy(cpumask_bits(&cell->rcpus_assigned),
		    jailhouse_cell_rcpu_set(cell_desc),
		    min((unsigned int)nr_cpumask_bits,
		        cell_desc->rcpu_set_size * 8));
	// DEBUG PRINT
	//pr_err("cpus->assigned %ld\nrcpus->assigned %ld\n",cell->cpus_assigned, cell->rcpus_assigned);

	bitmap_copy(cpumask_bits(&cell->fpga_regions_assigned),
		    jailhouse_cell_fpga_regions(cell_desc),
		    min((unsigned int)nr_cpumask_bits,
		        cell_desc->fpga_regions_size * 8));
	
	//DEBUG PRINT
	// pr_err("fpga regions size %x\n",cell_desc->fpga_regions_size);
	// pr_err("num fpga devices %d\n",cell_desc->num_fpga_devices);
	// pr_err("regions assigned %ld\n",cell->fpga_regions_assigned);

	if(cell_desc->num_rcpu_devices > 0 && cell->id != 0){
		cell->soft_rcpus_info = vmalloc(cell_desc->num_rcpu_devices * sizeof(struct soft_rcpus_info *));
		if (!cell->soft_rcpus_info) {
			pr_err("Failed to allocate memory for soft rcpus\n");
			kfree(cell);
			return ERR_PTR(-ENOMEM);
		}
	}
	else{
		cell->soft_rcpus_info = NULL;
	}	

	cell->num_memory_regions = cell_desc->num_memory_regions;
	cell->memory_regions = vmalloc(sizeof(struct jailhouse_memory) *
				       cell->num_memory_regions);
	if (!cell->memory_regions) {
		kfree(cell);
		return ERR_PTR(-ENOMEM);
	}

	memcpy(cell->name, cell_desc->name, JAILHOUSE_CELL_ID_NAMELEN);
	cell->name[JAILHOUSE_CELL_ID_NAMELEN] = 0;

	memcpy(cell->memory_regions, jailhouse_cell_mem_regions(cell_desc),
	       sizeof(struct jailhouse_memory) * cell->num_memory_regions);

	err = jailhouse_pci_cell_setup(cell, cell_desc);
	if (err) {
		jailhouse_rcpus_remove(cell);
		jailhouse_fpga_regions_remove(cell);
		vfree(cell->memory_regions);
		kfree(cell);
		return ERR_PTR(err);
	}

	err = jailhouse_sysfs_cell_create(cell);
	if (err)
		/* cleanup done by jailhouse_sysfs_cell_create */
		return ERR_PTR(err);

	return cell;
}

static void cell_register(struct cell *cell)
{
	list_add_tail(&cell->entry, &cells);
	jailhouse_sysfs_cell_register(cell);
}

static struct cell *find_cell(struct jailhouse_cell_id *cell_id)
{
	struct cell *cell;

	list_for_each_entry(cell, &cells, entry)
		if (cell_id->id == cell->id ||
		    (cell_id->id == JAILHOUSE_CELL_ID_UNUSED &&
		     strcmp(cell->name, cell_id->name) == 0))
			return cell;
	return NULL;
}

static void cell_delete(struct cell *cell)
{
	list_del(&cell->entry);
	jailhouse_sysfs_cell_delete(cell);
}

int jailhouse_cell_prepare_root(const struct jailhouse_cell_desc *cell_desc)
{
	root_cell = cell_create(cell_desc);
	if (IS_ERR(root_cell))
		return PTR_ERR(root_cell);

	// Does the root cell need to load the fpga devices in this way? 
	//jailhouse_fpga_regions_setup(root_cell, cell_desc);
	pr_info("root cell rcpus setup stated ...\n");
	jailhouse_root_rcpus_setup(root_cell, cell_desc);

	return 0;
}

void jailhouse_cell_register_root(void)
{
	root_cell->id = 0;
	cell_register(root_cell);
}

void jailhouse_cell_delete_root(void)
{
	jailhouse_root_rcpus_remove();
	cell_delete(root_cell);
	root_cell = NULL;
}

int jailhouse_cmd_cell_create(struct jailhouse_cell_create __user *arg)
{
	struct jailhouse_cell_create cell_params;
	struct jailhouse_cell_desc *config;
	struct jailhouse_cell_id cell_id;
	void __user *user_config;
	struct cell *cell;
	unsigned int cpu;
	int err = 0;

	if (copy_from_user(&cell_params, arg, sizeof(cell_params)))
		return -EFAULT;

	config = kmalloc(cell_params.config_size, GFP_USER | __GFP_NOWARN);
	if (!config)
		return -ENOMEM;

	user_config = (void __user *)(unsigned long)cell_params.config_address;
	if (copy_from_user(config, user_config, cell_params.config_size)) {
		err = -EFAULT;
		goto kfree_config_out;
	}

	if (cell_params.config_size < sizeof(*config) ||
	    memcmp(config->signature, JAILHOUSE_CELL_DESC_SIGNATURE,
		   sizeof(config->signature)) != 0) {
		pr_err("jailhouse: Not a cell configuration\n");
		err = -EINVAL;
		goto kfree_config_out;
	}
	if (config->revision != JAILHOUSE_CONFIG_REVISION) {
		pr_err("jailhouse: Configuration revision mismatch\n");
		err = -EINVAL;
		goto kfree_config_out;
	}
	if (config->architecture != JAILHOUSE_ARCHITECTURE) {
		pr_err("jailhouse: Configuration architecture mismatch\n");
		goto kfree_config_out;
	}

	config->name[JAILHOUSE_CELL_NAME_MAXLEN] = 0;

	/* CONSOLE_ACTIVE implies CONSOLE_PERMITTED for non-root cells */
	if (CELL_FLAGS_VIRTUAL_CONSOLE_ACTIVE(config->flags))
		config->flags |= JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED;

	if (mutex_lock_interruptible(&jailhouse_lock) != 0) {
		err = -EINTR;
		goto kfree_config_out;
	}

	if (!jailhouse_enabled) {
		err = -EINVAL;
		goto unlock_out;
	}

	cell_id.id = JAILHOUSE_CELL_ID_UNUSED;
	memcpy(cell_id.name, config->name, sizeof(cell_id.name));
	if (find_cell(&cell_id) != NULL) {
		err = -EEXIST;
		goto unlock_out;
	}

	cell = cell_create(config);
	if (IS_ERR(cell)) {
		err = PTR_ERR(cell);
		goto unlock_out;
	}

	config->id = cell->id;

	if (!cpumask_subset(&cell->cpus_assigned, &root_cell->cpus_assigned)) {
		err = -EBUSY;
		goto error_cell_delete;
	}

	// to do ... check if the rcpus + soft_rcpus are subset of the root cell rcpus
	// if (!cpumask_subset(&cell->rcpus_assigned, &root_cell->rcpus_assigned)) {
	// 	err = -EBUSY;
	// 	goto error_cell_delete;
	// }

	if (!cpumask_subset(&cell->fpga_regions_assigned, &root_cell->fpga_regions_assigned)) {
		err = -EBUSY;
		goto error_cell_delete;
	}
	/* Off-line each CPU assigned to the new cell and remove it from the
	 * root cell's set. */
	for_each_cpu(cpu, &cell->cpus_assigned) {
#ifdef CONFIG_X86
		if (cpu == 0) {
			/*
			 * On x86, Linux only parks CPU 0 when offlining it and
			 * expects to be able to get it back by sending an IPI.
			 * This is not support by Jailhouse wich destroys the
			 * CPU state across non-root assignments.
			 */
			pr_err("Cannot assign CPU 0 to other cells\n");
			err = -EINVAL;
			goto error_cpu_online;
		}
#endif
		if (cpu_online(cpu)) {
			err = remove_cpu(cpu);
			if (err)
				goto error_cpu_online;
			cpumask_set_cpu(cpu, &offlined_cpus);
		}
		cpumask_clear_cpu(cpu, &root_cell->cpus_assigned);
	}

	//to do ... error management
	err = jailhouse_fpga_regions_setup(cell, config);
	if(err < 0)
		goto unlock_out;
	
	//to do ... error management
	err = jailhouse_rcpus_setup(cell, config);
	if(err < 0)
		goto unlock_out;
	

	jailhouse_pci_do_all_devices(cell, JAILHOUSE_PCI_TYPE_DEVICE,
	                             JAILHOUSE_PCI_ACTION_CLAIM);

	err = jailhouse_call_arg1(JAILHOUSE_HC_CELL_CREATE, __pa(config));
	if (err < 0)
		goto error_cpu_online;

	cell_register(cell);

	pr_info("Created Jailhouse cell \"%s\"\n", config->name);

unlock_out:
	mutex_unlock(&jailhouse_lock);

kfree_config_out:
	kfree(config);

	return err;

error_cpu_online:
	for_each_cpu(cpu, &cell->cpus_assigned) {
		if (!cpu_online(cpu) && add_cpu(cpu) == 0)
			cpumask_clear_cpu(cpu, &offlined_cpus);
		cpumask_set_cpu(cpu, &root_cell->cpus_assigned);
	}

error_cell_delete:
	cell_delete(cell);
	goto unlock_out;
}

static int cell_management_prologue(struct jailhouse_cell_id *cell_id,
				    struct cell **cell_ptr)
{
	cell_id->name[JAILHOUSE_CELL_ID_NAMELEN] = 0;

	if (mutex_lock_interruptible(&jailhouse_lock) != 0)
		return -EINTR;

	if (!jailhouse_enabled) {
		mutex_unlock(&jailhouse_lock);
		return -EINVAL;
	}

	*cell_ptr = find_cell(cell_id);
	if (*cell_ptr == NULL) {
		mutex_unlock(&jailhouse_lock);
		return -ENOENT;
	}
	return 0;
}

#define MEM_REQ_FLAGS	(JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_LOADABLE)

static int load_image(struct cell *cell,
		      struct jailhouse_preload_image __user *uimage)
{
	struct jailhouse_preload_image image;
	const struct jailhouse_memory *mem;
	unsigned int regions, page_offs;
	u64 image_offset, phys_start;
	void *image_mem;
	int err = 0;

	if (copy_from_user(&image, uimage, sizeof(image)))
		return -EFAULT;

	// DEBUG PRINT
	//pr_err("\nimage.size: %dBytes (0x%x)", image.size, image.size);
	//pr_err("image.source_address: 0x%x\n", image.source_address);
	//pr_err("image.target_address: 0x%x\n", image.target_address);
	//pr_err("image.padding: %dBytes (0x%x)\n", image.padding, image.padding);

	if (image.size == 0)
		return 0;

	mem = cell->memory_regions;
	for (regions = cell->num_memory_regions; regions > 0; regions--) {
		image_offset = image.target_address - mem->virt_start;
		//pr_err("Check if image target > mem virt start and if image_offset < mem size \n");
		//pr_err("image.target_address:0x%x - mem->virt_start:0x%x = image_offset:0x%x\n",image.target_address, mem->virt_start, image_offset);
		if (image.target_address >= mem->virt_start &&
		    image_offset < mem->size) {
			//pr_err("Check on image size \n");
			//pr_err("image.size:0x%x > mem->size:0x%x - image_offset:0x%x\n",image.size, mem->size, image_offset);
			if (image.size > mem->size - image_offset ||
			    (mem->flags & MEM_REQ_FLAGS) != MEM_REQ_FLAGS)
				return -EINVAL;
				
			break;
		}
		mem++;
	}
	if (regions == 0)
		return -EINVAL;

	if (mem->flags & JAILHOUSE_MEM_COLORED) {
		/* Tweak the base address to request remapping of
		 * a reserved, high memory region.
		 */
		phys_start = (mem->virt_start + image_offset +
			      root_cell->color_root_map_offset) & PAGE_MASK;
	} else {
		phys_start = (mem->phys_start + image_offset) & PAGE_MASK;
	}
	page_offs = offset_in_page(image_offset);
	// pr_err("ioremap phys_start:0x%x image.size(0x%x) + page_offs(0x%x)\n",phys_start, image.size, page_offs);
	image_mem = jailhouse_ioremap(phys_start, 0,
				      PAGE_ALIGN(image.size + page_offs));
	if (!image_mem) {
		pr_err("jailhouse: Unable to map cell RAM at %08llx "
		       "for image loading\n",
		       (unsigned long long)(mem->phys_start + image_offset));
		return -EBUSY;
	}

	if (copy_from_user(image_mem + page_offs,
			   (void __user *)(unsigned long)image.source_address,
			   image.size))
		err = -EFAULT;
	/*
	 * ARMv7 and ARMv8 require to clean D-cache and invalidate I-cache for
	 * memory containing new instructions. On x86 this is a NOP.
	 */
	flush_icache_range((unsigned long)(image_mem + page_offs),
			   (unsigned long)(image_mem + page_offs) + image.size);
#ifdef CONFIG_ARM
	/*
	 * ARMv7 requires to flush the written code and data out of D-cache to
	 * allow the guest starting off with caches disabled.
	 */
	__cpuc_flush_dcache_area(image_mem + page_offs, image.size);
#endif

	vunmap(image_mem);

	return err;
}



int jailhouse_cmd_cell_load(struct jailhouse_cell_load __user *arg)
{
	struct jailhouse_preload_image __user *image = arg->image;
	struct jailhouse_preload_rcpu_image __user * rcpu_image = arg->rcpu_image;
	struct jailhouse_cell_load cell_load;
	struct cell *cell;
	unsigned int n;
	int err;

	if (copy_from_user(&cell_load, arg, sizeof(cell_load)))
		return -EFAULT;

	err = cell_management_prologue(&cell_load.cell_id, &cell);
	if (err)
		return err;

	err = jailhouse_call_arg1(JAILHOUSE_HC_CELL_SET_LOADABLE, cell->id);
	if (err)
		goto unlock_out;

	// DEBUG PRINT
	pr_info("Preparing to load %d images for Remote Processors...\n",cell_load.num_rcpu_images);
	for (n = cell_load.num_rcpu_images; n > 0; n--, rcpu_image++) {
		pr_info("Loading image: %s on rCPU: %d \n", rcpu_image->name, rcpu_image->rcpu_id);
		err = jailhouse_load_rcpu_image(cell, rcpu_image);
		if (err){
			pr_err("Unable to load rcpu image %s\n",rcpu_image->name);
			goto unlock_out;
		}
	}

	// DEBUG PRINT
	pr_err("cell_load.num_preload_images: %d\n",cell_load.num_preload_images);
	for (n = cell_load.num_preload_images; n > 0; n--, image++) {
		err = load_image(cell, image);
		if (err)
			break;
	}

unlock_out:
	mutex_unlock(&jailhouse_lock);

	return err;
}

int jailhouse_cmd_cell_start(const char __user *arg)
{
	struct jailhouse_cell_id cell_id;
	struct cell *cell;
	int err;

	if (copy_from_user(&cell_id, arg, sizeof(cell_id)))
		return -EFAULT;

	err = cell_management_prologue(&cell_id, &cell);
	if (err)
		return err;

	err = jailhouse_start_rcpu(cell);
	if (err)
		pr_err("Failed to start rcpus\n");

	err = jailhouse_call_arg1(JAILHOUSE_HC_CELL_START, cell->id);
	
	mutex_unlock(&jailhouse_lock);

	return err;
}

static int cell_destroy(struct cell *cell)
{
	unsigned int cpu;
	int err;

	err = jailhouse_call_arg1(JAILHOUSE_HC_CELL_DESTROY, cell->id);
	if (err)
		return err;

	err = jailhouse_rcpus_remove(cell);
	if (err)
		pr_err("Jailhouse: failed to remove rcpus\n");

	err = jailhouse_fpga_regions_remove(cell);
	if (err)
		pr_err("Jailhouse: failed to remove fpga regions\n");
		
	for_each_cpu(cpu, &cell->cpus_assigned) {
		if (cpumask_test_cpu(cpu, &offlined_cpus)) {
			if (add_cpu(cpu) != 0)
				pr_err("Jailhouse: failed to bring CPU %d "
				       "back online\n", cpu);
			cpumask_clear_cpu(cpu, &offlined_cpus);
		}
		cpumask_set_cpu(cpu, &root_cell->cpus_assigned);
	}

	jailhouse_pci_do_all_devices(cell, JAILHOUSE_PCI_TYPE_DEVICE,
	                             JAILHOUSE_PCI_ACTION_RELEASE);

	pr_info("Destroyed Jailhouse cell \"%s\"\n", cell->name);

	cell_delete(cell);

	return 0;
}

int jailhouse_cmd_cell_destroy(const char __user *arg)
{
	struct jailhouse_cell_id cell_id;
	struct cell *cell;
	int err;

	if (copy_from_user(&cell_id, arg, sizeof(cell_id)))
		return -EFAULT;

	err = cell_management_prologue(&cell_id, &cell);
	if (err)
		return err;

	err = cell_destroy(cell);

	mutex_unlock(&jailhouse_lock);

	return err;
}

int jailhouse_cmd_cell_destroy_non_root(void)
{
	struct cell *cell, *tmp;
	int err;

	list_for_each_entry_safe(cell, tmp, &cells, entry) {
		if (cell == root_cell)
			continue;
		err = cell_destroy(cell);
		if (err) {
			pr_err("Jailhouse: failed to destroy cell \"%s\"\n", cell->name);
			return err;
		}
	}

	return 0;
}
