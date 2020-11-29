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

/** Memguard parameters.
 * Used from hypervisor, linux driver and userspace.
 * Do not use implicit includes for u64, u32.
 */
struct memguard_params {
	/** Budget time in microseconds */
	unsigned long long budget_time;
	/** Memory budget (number of cache misses / equivalent PMU info) */
	unsigned int budget_memory;
	/** Flags: ignored and currently always set to periodic enforcing */
	unsigned int flags;
};
