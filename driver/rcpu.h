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

#include <jailhouse/config.h>
#include "cell.h"

int jailhouse_rcpus_setup(void);
int jailhouse_rcpus_check(struct cell *cell);
int jailhouse_load_rcpu_image(struct cell *cell, 
                    struct jailhouse_preload_rcpu_image __user *rcpu_uimage);
int jailhouse_start_rcpu(unsigned int rcpu_id);
int jailhouse_stop_rcpu(unsigned int rcpu_id);
int jailhouse_rcpus_remove(void);

#endif /* !_JAILHOUSE_DRIVER_SYSFS_H */