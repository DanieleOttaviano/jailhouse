/*
 * Memguard for Jailhouse, not implemented stub.
 *
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <jailhouse/memguard.h>

int memguard_set(
	struct memguard *memguard __attribute__((unused)),
	unsigned long params_address __attribute__((unused)))
{
	mg_print("Memguard not implemented on this architecture\n");
	return 0;
}
