/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_PANIC_H
#define _JAILHOUSE_PANIC_H

extern volatile unsigned long panic_in_progress;
extern unsigned long panic_cpu;

void __attribute__((noreturn)) panic_stop(void);
void panic_park(void);

/**
 * Performs the architecture-specifc steps to stop the current CPU on panic.
 *
 * @note This function never returns.
 *
 * @see panic_stop
 */
void __attribute__((noreturn)) arch_panic_stop(void);

/**
 * Performs the architecture-specific steps to park the current CPU on panic.
 *
 * @note This function only marks the CPU as parked and then returns to the
 * caller.
 *
 * @see panic_park
 */
void arch_panic_park(void);

#endif
