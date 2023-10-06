/*
 * Hypervisor timer for Jailhouse
 *
 * Copyright (C) Technical University of Munich, 2020
 *
 * Author:
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef ARM_TIMER_H

#if defined(__aarch64__)
#include <asm/timer64.h>
#else
static inline bool timer_isr_handler(void)
{
	/* Note, this is the irq handler for the hypervisor timer.
	 * Just fake that we're "processing" it.
	 */
	return true;
}
#endif

#endif
