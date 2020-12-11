/*
 * Memguard for Jailhouse
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
#ifndef _JAILHOUSE_MEMGUARD_DATA_H
#define _JAILHOUSE_MEMGUARD_DATA_H

#include <jailhouse/types.h>

/** Per-CPU memguard parameter structure */
struct memguard {
	/** Period-related parameters and time budget */
	u64 start_time;
	u64 last_time;
	u64 budget_time;
	/** Memory budget */
	u32 budget_memory;
	/** Blocking state machine */
	volatile u32 block;
};

#endif
