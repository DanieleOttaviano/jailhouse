/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Boston University, 2021
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * * * * * * Description * * * * * *
 *
 * Configuration for Xilinx ZynqMP ZCU102 eval board for colored
 * configuration to be used for dynamic recoloring of the
 * root-cell. This configuration supports partition sizes from 2 to 16
 * cache partitions with carved-out space for 3 mem bombs.
 *
 * Physical Memory Layout:
 * NOTE: this platforms has 2GB @ 0x0_0000_0000 + 2GB @0x8_0000_0000
 *
 * Jailhouse:   0x8_7f00_0000 -> 0x8_8000_0000 (size: 0x0_0100_0000)
 *
 * Comm Reg1:   0x8_7e00_0000 -> 0x8_7f00_0000 (size: 0x0_0100_0000)
 *
 * Comm Reg2:   0x8_7d00_0000 -> 0x8_7e00_0000 (size: 0x0_0100_0000)
 *
 * Bombs Cmd:   0x8_7c00_0000 -> 0x8_7d00_0000 (size: 0x0_0100_0000)
 *
 * Bombs Mem:   0x8_0000_0000 -> 0x8_7c00_0000 (size: 0x0_7c00_0000)
 *
 * SysMem LO:   0x0_0000_0000 -> 0x0_1000_0000 (size: 0x0_1000_0000)
 *              COL: 0x0003,END: 0x0_8000_0000
 *              COL: 0x000f,END: 0x0_4000_0000
 *              VIRT: 0x0_0000_0000 -> 0x0_1000_0000
 *
 * * * * * * Required DTS entry to match JH configuration * * * * * *
 *
 * / {
 * 	reserved-memory {
 * 		#address-cells = <2>;
 * 		#size-cells = <2>;
 * 		ranges;
 *
 * 		jailhouse_mem: jailhouse_mem@87c000000 {
 * 			 no-map;
 * 		         reg = <0x8 0x7c000000 0x0 0x04000000>;
 * 		};
 *
 *		private_hi: private_hi@800000000 {
 * 		         reg = <0x8 0x00000000 0x0 0x80000000>;
 * 		};
 *
 * 		private_lo: private_lo@010000000 {
 * 		         reg = <0x0 0x10000000 0x0 0x70000000>;
 * 		};
 *
 * 	};
 * };
 *
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>
#include <asm/qos-400.h>
#include <zynqmp-qos-config.h>

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[23];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[2];
	union jailhouse_stream_id stream_ids[3];
	struct jailhouse_qos_device qos_devices[35];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x87f000000,
			.size =       0x001000000,
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
				/* .way_size = 0x10000, */
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
			.qos = {
				.nic_base = 0xfd700000,
				/* 1MiB Aperture */
				.nic_size = 0x100000,
			},
		},

		.root_cell = {
			.name = "ZynqMP-ZCU102 Col16",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			.num_stream_ids = ARRAY_SIZE(config.stream_ids),
			.num_qos_devices = ARRAY_SIZE(config.qos_devices),

			.vpci_irq_base = 136-32,
		},
	},

	.cpus = {
		0xf,
	},

	.mem_regions = {
		/* IVSHMEM shared memory region for 0001:00:00.0 */
		JAILHOUSE_SHMEM_NET_REGIONS(0x87d000000, 0),
		/* IVSHMEM shared memory region for 0001:00:01.0 */
		JAILHOUSE_SHMEM_NET_REGIONS(0x87e000000, 0),
		/* MMIO (permissive) */ {
			.phys_start = 0xfd000000,
			.virt_start = 0xfd000000,
			.size =	      0x03000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* SysMem LO */ {
			.phys_start = 0x0,
			.virt_start = 0x0,
			// Limit size to 256 M
			.size = 0x010000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_COLORED,
			.colors=0x0003,
		},
		/* Bombs Mem */ {
			.phys_start = 0x800000000,
			.virt_start = 0x800000000,
			.size = 0x087c00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
			        JAILHOUSE_MEM_EXECUTE,
		},
		/* Bombs Comm Region */ {
		        .phys_start = 0x87c000000,
			.virt_start = 0x87c000000,
			.size = 0x001000000,
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
			.size = 0x100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE,
		},

		/* DDR 1 region for the R5 */ {
			.phys_start = 0x3ed40000,
			.virt_start = 0x3ed40000,
			.size = 0x100000,
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

	.qos_devices = {
		{
			.name = "rpu0",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_RPU0_BASE,
		},

		{
			.name = "rpu1",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_RPU1_BASE,
		},

		{
			.name = "adma",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_ADMA_BASE,
		},

		{
			.name = "afifm0",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_AFIFM0_BASE,
		},
		{
			.name = "afifm1",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_AFIFM1_BASE,
		},

		{
			.name = "afifm2",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_AFIFM2_BASE,
		},

		{
			.name = "smmutbu5",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INITFPDSMMUTBU5_BASE,
		},

		{
			.name = "dp",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_DP_BASE,
		},

		{
			.name = "afifm3",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_AFIFM3_BASE,
		},

		{
			.name = "afifm4",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_AFIFM4_BASE,
		},

		{
			.name = "afifm5",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_AFIFM5_BASE,
		},

		{
			.name = "gpu",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_GPU_BASE,
		},

		{
			.name = "pcie",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_PCIE_BASE,
		},

		{
			.name = "gdma",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_GDMA_BASE,
		},

		{
			.name = "sata",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_SATA_BASE,
		},

		{
			.name = "coresight",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_CORESIGHT_BASE,
		},

		{
			.name = "issib2",
			.flags = (FLAGS_HAS_REGUL),
			.base = ISS_IB2_BASE,
		},
		{
			.name = "issib6",
			.flags = (FLAGS_HAS_REGUL),
			.base = ISS_IB6_BASE,
		},
	},
};
