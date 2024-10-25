/*
 * Jailhouse, a Linux-based partitioning hypervisor
 * 
 * Omnivisor branch for remote core virtualization 
 * 
 * Copyright (c) Siemens AG, 2013-2015
 * Copyright (c) Daniele Ottaviano, 2024
 * 
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *  Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_CELL_H
#define _JAILHOUSE_CELL_H

#include <jailhouse/mmio.h>
#include <jailhouse/paging.h>
#include <jailhouse/pci.h>
#include <asm/cell.h>
#include <asm/spinlock.h>

#include <jailhouse/cell-config.h>
#include <jailhouse/hypercall.h>

/** Cell-related states. */
struct cell {
	union {
		/** Communication region. */
		struct jailhouse_comm_region comm_region;
		/** Padding to full page size. */
		u8 padding[PAGE_SIZE];
	} __attribute__((aligned(PAGE_SIZE))) comm_page;
	/**< Page containing the communication region (shared with cell). */

	/** Architecture-specific fields. */
	struct arch_cell arch;

	/** Number of pages used for storing cell-specific states and
	 * configuration data. */
	unsigned int data_pages;
	/** Pointer to static cell description. */
	struct jailhouse_cell_desc *config;

	/** Pointer to cell's CPU set. */
	struct cpu_set *cpu_set;
	/** Stores the cell's CPU set if small enough. */
	struct cpu_set small_cpu_set;

#if defined(CONFIG_OMNIVISOR)
	/** Pointer to cell's rCPU set. */
	struct cpu_set *rcpu_set;	
	/** Stores the cell's rCPU set if small enough. */
	struct cpu_set small_rcpu_set;
#endif /* CONFIG_OMNIVISOR */

#if defined (CONFIG_OMNV_FPGA)
	/** Pointer to cell's regions . */
	struct fpga_region_set *fpga_region_set;	
	/** Stores the cell's regions if small enough. */
	struct fpga_region_set small_fpga_region_set;
#endif 
	/** True while the cell can be loaded by the root cell. */
	bool loadable;

	/** Pointer to next cell in the system. */
	struct cell *next;

	/** List of PCI devices assigned to this cell. */
	struct pci_device *pci_devices;

	/** Lock protecting changes to mmio_locations, mmio_handlers, and
	 * num_mmio_regions. */
	spinlock_t mmio_region_lock;
	/** Generation counter of mmio_locations, mmio_handlers, and
	 * num_mmio_regions. */
	volatile unsigned long mmio_generation;
	/** MMIO region description table. */
	struct mmio_region_location *mmio_locations;
	/** MMIO region handler table. */
	struct mmio_region_handler *mmio_handlers;
	/** Number of MMIO regions in use. */
	unsigned int num_mmio_regions;
	/** Maximum number of MMIO regions. */
	unsigned int max_mmio_regions;
};

extern struct cell root_cell;

#endif /* !_JAILHOUSE_CELL_H */
