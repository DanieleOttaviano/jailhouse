/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for demo inmate on Xilinx ZynqMP ZCU104 eval board:
 * 1 CPU, 64K RAM, 1 serial port
 *
 * Copyright (c) Minerva Systems, 2022
 *
 * Authors:
 *   Mirko Cangiano <mirko.cangiano@minervasys.tech>
 *   Luca Palazzi <luca.palazzi@minervasys.tech>
 *   Carlo Nonato <carlo.nonato@minervasys.tech>
 *   Donato Ferraro <donato.ferraro@minervays.tech>
 *   Fabio Spanò <fabio.spano@minervasys.tech>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[8];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.name = "inmate",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG,

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = ARRAY_SIZE(config.irqchips),
		.num_pci_devices = ARRAY_SIZE(config.pci_devices),
		.vpci_irq_base = 140-32,
		.console = {
			.address = 0xff010000,
			.type = JAILHOUSE_CON_TYPE_XUARTPS,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0x8,
	},

	.mem_regions = {
		/* IVSHMEM */
		/* STATE */ {
			.phys_start = 0x25000000,
			.virt_start = 0x25000000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* RW */ {
			.phys_start = 0x25001000,
			.virt_start = 0x25001000,
			.size = 0x9000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED | JAILHOUSE_MEM_IO,
		},
		/* Output of VM1 - Input for VM2 */ {	
			.phys_start = 0x2500a000,
			.virt_start = 0x2500a000,
			.size = 0x0E552000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* Output of VM2 - Input for VM1 */ {	
			.phys_start = 0x3355C000,
			.virt_start = 0x3355C000,
			.size = 0x0E552000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_IO |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* Output of VM3 - Input for VM1 */ {	
			.phys_start = 0x41AAE000,
			.virt_start = 0x41AAE000,
			.size = 0x0E552000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_IO |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* UART */ {
			.phys_start = 0xff010000,
			.virt_start = 0xff010000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* RAM */ {
			.phys_start = 0x50600000,
			.virt_start = 0,
			.size = 0x00500000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* communication region */ {
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
	},

	.irqchips = {
		/* GIC */ {
			.address = 0xf9010000,
			.pin_base = 32,
			.pin_bitmap = {
				(1<<(52 - 32)),
				0,
				0,
				(1 << (140 - 128)) | (1 << (141 - 128)),
			},
		},
	},

	.pci_devices = {
		{ /* IVSHMEM (demo) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 2,
			.bdf = 0 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 0,
			.shmem_dev_id = 0,
			.shmem_peers = 3,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_UNDEFINED,
		},
	},
};
