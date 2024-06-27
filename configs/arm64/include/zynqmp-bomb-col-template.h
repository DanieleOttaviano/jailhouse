/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration file template for inmate-bombs. Don't include directly.
 *
 * Copyright (C) Boston University, MA, USA, 2020
 * Copyright (C) Technical University of Munich, 2020 - 2021
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[5];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.name = "col-mem-bomb-" xstr(BOMB_ID),
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG,

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = 0,
		.num_pci_devices = 0,

		.console = {
			.address = 0xff010000,
			.type = JAILHOUSE_CON_TYPE_XUARTPS,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		BOMB_CPU,
	},

	.mem_regions = {
		/* UART */ {
			.phys_start = 0xff010000,
			.virt_start = 0xff010000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},

		/* RAM */
		{
			.phys_start = MAIN_PHYS_START,
			.virt_start = 0,
			.size = MAIN_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE |
				JAILHOUSE_MEM_COLORED,
			.colors=BOMB_COLORS,
		},

		/* RAM Buffer */
		{
			.phys_start = MEM_PHYS_START,
			.virt_start = MEM_VIRT_START,
			.size = MEM_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				 JAILHOUSE_MEM_COLORED,
			.colors=BOMB_COLORS,
		},

		/* Control interface */ {
			.phys_start = COMM_PHYS_ADDR,
			.virt_start = COMM_VIRT_ADDR,
			.size = COMM_SINGLE_SIZE,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
		                 JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},

		/* communication region */ {
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
	},
};
