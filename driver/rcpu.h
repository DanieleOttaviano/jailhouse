/*
* Jailhouse, a Linux-based partitioning hypervisor
*
* Omnivisor remote core virtualization support
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
#ifndef _JAILHOUSE_RCPU_H
#define _JAILHOUSE_RCPU_H

#include "cell.h"

#define JAILHOUSE_RCPU_IMAGE_NAMELEN 31

#define for_each_rcpu(rcpu, set) \
	for_each_cpu(rcpu, set)

struct rcpu_info {
    struct rproc *rproc;
    unsigned int id;
	char name[JAILHOUSE_RCPU_IMAGE_NAMELEN];
	char compatible[JAILHOUSE_RCPU_IMAGE_NAMELEN];
};

#ifdef CONFIG_OMNIVISOR

int jailhouse_root_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config);
int jailhouse_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config);
int jailhouse_load_rcpu_image(struct cell *cell, 
                    struct jailhouse_preload_rcpu_image __user *rcpu_uimage);
int jailhouse_start_rcpu(struct cell *cell);
int jailhouse_root_rcpus_remove(void);
int jailhouse_rcpus_remove(struct cell *cell);

#else /* !CONFIG_OMNIVISOR */


// The next functions are called only if there are rcpus in the configuration.
// if CONFIG_OMNIVISOR is not set, calling is an error.
static inline int jailhouse_root_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
    pr_err("ERROR: CONFIG_OMNIVISOR is not set in Jailhouse\n");
    return -1;
}

static inline int jailhouse_rcpus_setup(struct cell *cell, const struct jailhouse_cell_desc *config)
{
    pr_err("ERROR: CONFIG_OMNIVISOR is not set in Jailhouse\n");
    return -1;
}

static inline int jailhouse_load_rcpu_image(struct cell *cell, 
                    struct jailhouse_preload_rcpu_image __user *rcpu_uimage)
{
    return -1;
}

static inline int jailhouse_start_rcpu(struct cell *cell)
{
    return -1;
}

static inline int jailhouse_root_rcpus_remove()
{
    return -1;
}

static inline int jailhouse_rcpus_remove(struct cell *cell)
{
    return -1;
}

#endif /* CONFIG_OMNIVISOR */

#endif /* !_JAILHOUSE_DRIVER_SYSFS_H */