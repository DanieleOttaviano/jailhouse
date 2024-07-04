/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for Xilinx ZynqMP ZCU106 eval board
 *
 * Copyright (c) Minerva Systems, 2024
 *
 * Authors:
 *   Luca Palazzi <luca.palazzi@minervasys.tech>
 *   Donato Ferraro <donato.ferraro@minervays.tech>
 *   Filippo Fontana <filippo.fontana@minervasys.tech>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Reservation via device tree:
 *       memory@0 {
 *               device_type = "memory";
 *               reg = <0x0 0x0 0x0 0x80000000 0x8 0x0 0x0 0x80000000>;
 *       };
 *
 *       reserved-memory {
 *               #address-cells = <0x2>;
 *               #size-cells = <0x2>;
 *               ranges;
 *
 *               jailhouse@0x7e000000 {
 *                       no-map;
 *                       reg = <0x0 0x7e000000 0x0 0x2000000>;
 *               };
 *
 *               inmates@0x25000000 {
 *                       no-map;
 *                       reg = <0x0 0x25000000 0x0 0x37000000>;
 *               };
 *       };
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>
#include <asm/qos-400.h>
#include <zynqmp-qos-config.h>

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[15];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
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
				/* in debug mode, the way_size is autodetected
				 * if it is not specified */
				.way_size = 0x10000,
				.root_map_offset = 0x0C000000000,
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
			.name = "ZynqMP-ZCU106",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			// .num_pci_devices = ARRAY_SIZE(config.pci_devices),

			.vpci_irq_base = 136-32,
		},
	},

	.cpus = {
		0xf,
	},

	.mem_regions = {
		JAILHOUSE_SHMEM_NET_REGIONS(0x50400000, 0),

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
			// Limit size to 256 M
			.size = 0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_COLORED,
			.colors=0x000f,
		},
		/* RAM (until SHMEM_NET region) */ {
			.phys_start = 0x10000000,
			.virt_start = 0x10000000,
			.size = 0x50400000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* RAM (until 0x7f000000) */ {
			.phys_start = 0x50500000,
			.virt_start = 0x50500000,
			.size = 0x2eb00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
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
		/* TCM region for the R5 */ {
			.phys_start = 0xffe00000,
			.virt_start = 0xffe00000,
			.size = 0xC0000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE,
		},
		/* DDR 0 region for the R5 */ {
			.phys_start = 0x3ed00000,
			.virt_start = 0x3ed00000,
			.size = 0x40000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE,
		},
		/* DDR 1 region for the R5 */ {
			.phys_start = 0x3ed40000,
			.virt_start = 0x3ed40000,
			.size = 0x40000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE,
		},
		/* proc 0 region for the R5 */ {
			.phys_start = 0xff9a0100,
			.virt_start = 0xff9a0100,
			.size = 0x100,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE,
		},
		/* proc 1 region for the R5 */ {
			.phys_start = 0xff9a0200,
			.virt_start = 0xff9a0200,
			.size = 0x100,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE,
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
		{ /* IVSHMEM 0000:00:01.0 (networking) */
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 0,
			.bdf = 1 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 0,
			.shmem_dev_id = 0,
			.shmem_peers = 1,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},
};
