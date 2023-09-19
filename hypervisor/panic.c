/*
 * Jailhouse panic and assert functionalities.
 *
 * Copyright (c) Siemens AG, 2013-2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <jailhouse/panic.h>
#include <jailhouse/assert.h>
#include <jailhouse/printk.h>
#include <jailhouse/percpu.h>
#include <jailhouse/control.h>
#include <jailhouse/processor.h>

/**
 * Stops the current CPU on panic and prevents any execution on it until the
 * system is rebooted.
 *
 * @note This service should be used when facing an unrecoverable error of the
 * hypervisor.
 *
 * @see panic_park
 */
void __attribute__((noreturn)) panic_stop(void)
{
	struct cell *cell = this_cell();

	panic_printk("Stopping CPU %d (Cell: \"%s\")\n", this_cpu_id(),
		     cell && cell->config ? cell->config->name : "<UNSET>");

	if (phys_processor_id() == panic_cpu)
		panic_in_progress = 0;

	arch_panic_stop();
}

/**
 * Parks the current CPU on panic, allowing to restart it by resetting the
 * cell's CPU state.
 *
 * @note This service should be used when facing an error of a cell CPU, e.g. a
 * cell boundary violation.
 *
 * @see panic_stop
 */
void panic_park(void)
{
	struct cell *cell = this_cell();
	bool cell_failed = true;
	unsigned int cpu;

	panic_printk("Parking CPU %d (Cell: \"%s\")\n", this_cpu_id(),
		     cell->config->name);

	this_cpu_public()->failed = true;
	for_each_cpu(cpu, cell->cpu_set)
		if (!public_per_cpu(cpu)->failed) {
			cell_failed = false;
			break;
		}
	if (cell_failed)
		cell->comm_page.comm_region.cell_state = JAILHOUSE_CELL_FAILED;

	arch_panic_park();

	if (phys_processor_id() == panic_cpu)
		panic_in_progress = 0;
}

#ifdef CONFIG_DEBUG
/* debug-only runtime assertion */
void __assert_fail(
		const char *file,
		unsigned int line,
		const char *func,
		const char *expr)
{
	panic_printk("%s:%u: %s assertion '%s' failed.\n",
		     file, line, func, expr);
	panic_stop();
}
#endif
