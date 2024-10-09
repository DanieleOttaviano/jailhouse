/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for RK3588 on Rock5B
 *
 * Copyright (c) Minerva Systems, 2024
 *
 * Authors:
 *  Filippo Fontana <filippo.fontana@minervasys.tech>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Reservation via device tree (arch/arm64/boot/dts/rockchip/rk3588-rock-5b.dts):
 *
 * 	reserved-memory {
 *		#address-cells = <2>;
 *		#size-cells = <2>;
 *		ranges;
 *
 *		jailhouse@20000000 {
 *			no-map;
 *			reg = <0x0 0x20000000 0x0 0x0D000000>;
 *		};
 *	};
 *
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[10];
	struct jailhouse_irqchip irqchips[4];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.architecture = JAILHOUSE_ARM64,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x20000000,
			.size = 0x600000,  // 6 MB
		},
		// uart2: serial@feb50000
		.debug_console = {
			.address = 0xfeb50000,
			// NOTE: MMIO errors with the real size of 0x100
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			// Address used by Jailhouse while remapping PCI
			.pci_mmconfig_base = 0xf0000000,
			.pci_mmconfig_end_bus = 0,
			.pci_is_virtual = 1,
			.pci_domain = -1,

			.arm = {
				.maintenance_irq = 25,
				.gic_version = 3,
				.gicd_base = 0xfe600000,
				.gicr_base = 0xfe680000,
			},

			.memguard = {
				/* For this SoC we have:
				   - 16 PPIs
				   - 480 SPIs
				   ------ Total = 496
				   */
				.num_irqs = 496,
				.hv_timer = 26,
				.num_pmu_irq = 8,
				/* One PMU irq per CPU */
				.pmu_cpu_irq = {
					23, 23, 23, 23, 23, 23, 23, 23,
				},
			},

		},
		.root_cell = {
			.name = "RADXA ROCK5B",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			// 454 is the first unused IRQ (from TRM)
			.vpci_irq_base = 454-32,
		},
	},

	.cpus = {
		0xff,
	},

	.mem_regions = {
		// NOTE: not listed in /proc/iomem,
		// without this region we get "Unhandled data write at 0x101000(4)"
		// while interacting with memory bombs from the root cell...
		{
			.phys_start = 0,
			.virt_start = 0,
			.size = 0x0010f000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		// 0010f000 - 001fffff : sram@10f000 (mmio-sram and ramoops)
		{
			.phys_start = 0x0010f000,
			.virt_start = 0x0010f000,
			.size = 0xf1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		// 00200000-1fffffff : System RAM 'till Jailhouse
		{
			.phys_start = 0x00200000,
			.virt_start = 0x00200000,
			.size = 0x20000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		// 20000000-20600000 : System RAM reserved by Jailhouse
		// {
		// 	.phys_start = 0x20000000,
		// 	.virt_start = 0x20000000,
		// 	.size = 0x00600000,
		// 	.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
		// 		JAILHOUSE_MEM_EXECUTE,
		// },
		// 20600000 - 2d000000 : System RAM for the Inmates
		{
			.phys_start = 0x20600000,
			.virt_start = 0x20600000,
			.size = 0x0ca00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		// 2d000000 - efffffff : System RAM
		{
			.phys_start = 0x2d000000,
			.virt_start = 0x2d000000,
			.size = 0xc3000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		// f0000000 - ff0effff : IO
		{
			.phys_start = 0xf0000000,
			.virt_start = 0xf0000000,
			.size = 0xf0f0000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		// 100000000-1ffffffff : System RAM
		{
			.phys_start = 0x100000000,
			.virt_start = 0x100000000,
			.size = 0x100000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		// 2f0000000-2ffffffff : System RAM
		{
			.phys_start = 0x2f0000000,
			.virt_start = 0x2f0000000,
			.size = 0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		// a00000000-a3fffffff : pcie@fe190000
		{
			.phys_start = 0xa00000000,
			.virt_start = 0xa00000000,
			.size = 0x40000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		// a41000000-a413fffff : fe190000.pcie pcie-dbi
		{
			.phys_start = 0xa41000000,
			.virt_start = 0xa41000000,
			.size = 0x400000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
	},

	.irqchips = {
		/* GIC */ {
			.address = 0xfe600000,
			.pin_base = 32,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			},
		},
		/* GIC */ {
			.address = 0xfe600000,
			.pin_base = 160,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			},
		},
		/* GIC */ {
			.address = 0xfe600000,
			.pin_base = 288,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
			},
		},
		/* GIC */ {
			.address = 0xfe600000,
			.pin_base = 416,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xc,
			},
		},
	},
};
