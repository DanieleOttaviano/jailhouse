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

#ifdef CONFIG_OMNIVISOR

int jailhouse_rcpus_check(struct cell *cell);
int jailhouse_rcpus_setup(void);
int jailhouse_load_rcpu_image(struct cell *cell, 
                    struct jailhouse_preload_rcpu_image __user *rcpu_uimage);
int jailhouse_start_rcpu(unsigned int rcpu_id);
int jailhouse_stop_rcpu(unsigned int rcpu_id);
int jailhouse_rcpus_remove(void);

#else /* !CONFIG_OMNIVISOR */

static inline int jailhouse_rcpus_check(struct cell *cell)
{
    return 0;
}

// The next functions are called only if there are rcpus in the configuration.
// if CONFIG_OMNIVISOR is not set, calling is an error.
static inline int jailhouse_rcpus_setup(void)
{
    pr_err("ERROR: CONFIG_OMNIVISOR is not set in Jailhouse\n");
    return -1;
}

static inline int jailhouse_load_rcpu_image(struct cell *cell, 
                    struct jailhouse_preload_rcpu_image __user *rcpu_uimage)
{
    return -1;
}

static inline int jailhouse_start_rcpu(unsigned int rcpu_id)
{
    return -1;
}

static inline int jailhouse_stop_rcpu(unsigned int rcpu_id)
{
    return -1;
}

static inline int jailhouse_rcpus_remove(void)
{
    return -1;
}
#endif /* CONFIG_OMNIVISOR */

#endif /* !_JAILHOUSE_DRIVER_SYSFS_H */