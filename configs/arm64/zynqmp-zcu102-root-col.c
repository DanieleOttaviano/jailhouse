/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for Xilinx ZynqMP ZCU102 eval board Colored Root cell.
 *
 * Copyright (C) Boston University, MA, USA, 2020
 * Copyright (C) University of Illinois at Urbana-Champaign, IL, USA, 2020
 * Copyright (C) Technical University of Munich, 2020 - 2021
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *  Rohan Tabish <rtabish@illinois.com;rohantabish@gmail.com>
 *  Andrea Bastoni <andrea.bastoni@tum.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Reserve memory with mem=512M
 */
#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[24];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[2];
	union jailhouse_stream_id stream_ids[3];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x7f000000,
			.size =       0x01000000,
		},
		.debug_console = {
			.address = 0xff000000,
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_XUARTPS,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			.pci_mmconfig_base = 0xfc000000,
			.pci_mmconfig_end_bus = 0,

			.pci_is_virtual = 1,
			.pci_domain = -1,
			.color = {
				.way_size = 0x10000,
				.root_map_offset = 0x0C000000000,
			},
			.iommu_units = {
				{
					.type = JAILHOUSE_IOMMU_ARM_MMU500,
					.base = 0xfd800000,
					.size = 0x20000,
				},
			},
			.arm = {
				.gic_version = 2,
				.gicd_base = 0xf9010000,
				.gicc_base = 0xf902f000,
				.gich_base = 0xf9040000,
				.gicv_base = 0xf906f000,
				.maintenance_irq = 25,
			},
			.memguard = {
				/* For this SoC we have:
				   - 32 SGIs and PPIs
				   - 8 SPIs
				   - 148 system interrupts
				   ------ Total = 188
				   */
				.num_irqs = 188,
				.hv_timer = 26,
				.irq_prio_min = 0xf0,
				.irq_prio_max = 0x00,
				.irq_prio_step = 0x10,
				.irq_prio_threshold = 0x10,
				.num_pmu_irq = 4,
				/* One PMU irq per CPU */
				.pmu_cpu_irq = {
					175, 176, 177, 178,
				},
			},
		},

		.root_cell = {
			.name = "ZynqMP-ZCU102",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			.num_stream_ids = ARRAY_SIZE(config.stream_ids),

			.vpci_irq_base = 136-32,
		},
	},

	.cpus = {
		0xf,
	},

	.mem_regions = {
		/* IVSHMEM shared memory region for 0001:00:00.0 */
		JAILHOUSE_SHMEM_NET_REGIONS(0x060000000, 0),
		/* IVSHMEM shared memory region for 0001:00:01.0 */
		JAILHOUSE_SHMEM_NET_REGIONS(0x060100000, 0),
		/* DSB */ {
			.phys_start = 0x7d000000,
			.virt_start = 0x7d000000,
			.size =	      0x01000000,
			.flags = JAILHOUSE_MEM_READ,
		},
		/* MMIO (permissive) */ {
			.phys_start = 0xfd000000,
			.virt_start = 0xfd000000,
			.size =	      0x03000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* RAM */ {
			.phys_start = 0x0,
			.virt_start = 0x0,
			// Limit size to 512 M
			.size = 0x20000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_COLORED,
			.colors=0x0fff,
		},
		/* RAM */ {
			.phys_start = 0x800000000,
			.virt_start = 0x800000000,
			.size = 0x080000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* PCI host bridge */ {
			.phys_start = 0x8000000000,
			.virt_start = 0x8000000000,
			.size = 0x1000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
	},

	.irqchips = {
		/* GIC */ {
			.address = 0xf9010000,
			.pin_base = 32,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			},
		},
	},

	.pci_devices = {
		/* 0001:00:01.0 */ {
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 1,
			.bdf = 1 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 0,
			.shmem_dev_id = 0,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
		/* 0001:00:02.0 */ {
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 1,
			.bdf = 2 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 4,
			.shmem_dev_id = 0,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},

	.stream_ids = {
		{
			.mmu500.id = 0x860,
			.mmu500.mask_out = 0x0,
		},
		{
			.mmu500.id = 0x861,
			.mmu500.mask_out = 0x0,
		},
		{
			.mmu500.id = 0x870,
			.mmu500.mask_out = 0xf,
		},
	},
};
