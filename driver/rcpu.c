#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>

#include "rcpu.h"

static const struct of_device_id rproc_of_match[] = {
	{ .compatible = "xlnx,zynqmp-r5-remoteproc", },
	{},
};

static struct rproc **rproc;
static int num_rprocs;

// check if the number of requested rcpus is less or equal than the available rcpus
int jailhouse_rcpus_check(struct cell *cell){
	int required_rcpus = 0;
	required_rcpus = cpumask_weight(&cell->rcpus_assigned);

	//DEBUG
	pr_info("Required rcpus: %d\n", required_rcpus);
	pr_info("Available rcpus: %d\n", num_rprocs);

	if (required_rcpus > num_rprocs)
		return -EINVAL;

	return 0;
}

// free the rproc array
int jailhouse_rcpus_remove(){
	int i;
	for (i = 0; i < num_rprocs; i++) {
		kfree(rproc[i]);
	}
	//DEBUG
	pr_info("Freed rproc array\n");
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
		pr_info("Number of Remote Processors: %d\n", num_rprocs);
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

	//TO DO ... check on configuration mode lockstep or split	

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
		i++;
	}

	// to remove from here
	for (i = 0; i < num_rprocs; i++) {
		err = rproc_set_firmware(rproc[i], "baremetal-demo.elf");
		if (err) {
			pr_err("Failed to set firmware for rproc[%d]\n", i);
			return err;
		}
	}

	return 0;
}