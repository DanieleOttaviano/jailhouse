/*
 * Memguard for Jailhouse: parameters setup.
 *
 * Copyright (c) Czech Technical University in Prague, 2018
 * Copyright (C) Boston University, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Joel Matějka <matejjoe@fel.cvut.cz>
 *  Michal Sojka <michal.sojka@cvut.cz>
 *  Přemysl Houdek <houdepre@fel.cvut.cz>
 *  Renato Mancuso (BU) <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_MEMGUARD_H
#define _JAILHOUSE_MEMGUARD_H

#include <jailhouse/memguard-data.h>
#include <jailhouse/printk.h>

#ifdef CONFIG_DEBUG
#define mg_print(fmt, ...)			\
	printk("[MG] " fmt, ##__VA_ARGS__)
#else
#define mg_print(fmt, ...) do { } while (0)
#endif

/** Set memguard parameters for the current CPU */
int memguard_set(struct memguard *memguard, unsigned long params_address);

#endif
