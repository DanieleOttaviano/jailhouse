/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for Xilinx ZynqMP Kria KV260 board
 *
 * Copyright (c) Siemens AG, 2016
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *  Daniele Ottaviano <danieleottaviano97@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Reservation via device tree: 
 * 	reserved-memory {
 *		#address-cells = <2>;
 *		#size-cells = <2>;
 *		ranges;
 *		jailhouse_reserved: jailhouse@7e000000 {
 *			no-map;
 *			reg = <0x0 0x7e000000 0x0 0x2000000>;
 *		}; 
 *	};
 */
#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>
#include <asm/qos-400.h>
#include <zynqmp-qos-config.h>

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	__u64 rcpus[1];
	__u64 fpga_regions[1];
	struct jailhouse_memory mem_regions[14];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[2];
	union jailhouse_stream_id stream_ids[3];
	struct jailhouse_qos_device qos_devices[35];
	struct jailhouse_rcpu_device rcpu_devices[2];
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
			.address = 0xff010000,
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_XUARTPS,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			.pci_mmconfig_base = 0xfc000000,
			.pci_mmconfig_end_bus = 0,

			.fpga_configuration_base = 0x80000000,
			
			.pci_is_virtual = 1,
			.pci_domain = 1,
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
			.qos = {
				.nic_base = 0xfd700000,
				/* 1MiB Aperture */
				.nic_size = 0x100000,
			},
			.fpga = {
				.fpga_base_bitstream = "pico32_tg_wrapper.bit",//"bitstream_base.bit",
				.fpga_base_addr = 0x80000000,
				.fpga_flags = JAILHOUSE_FPGA_PARTIAL,
				.fpga_max_regions = 3,
			},
		},

		.root_cell = {
			.name = "ZynqMP-KV260",

			.cpu_set_size = sizeof(config.cpus),
			.rcpu_set_size = sizeof(config.rcpus), 
			.fpga_regions_size = sizeof(config.fpga_regions),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			.num_stream_ids = ARRAY_SIZE(config.stream_ids),
			.num_qos_devices = ARRAY_SIZE(config.qos_devices),
			.num_rcpu_devices = ARRAY_SIZE(config.rcpu_devices),
			.vpci_irq_base = 136-32,
		},
	},

	.cpus = {
		0xf,
	},

	.rcpus = {
		0x3, // RPU0, RPU1
	},

	.fpga_regions = {
		0x7, // 3 FPGA regions
	},

	.mem_regions = {
		/* IVSHMEM shared memory region for 0001:00:00.0 */
		JAILHOUSE_SHMEM_NET_REGIONS(0x07e000000, 0),
		/* IVSHMEM shared memory region for 0001:00:01.0 */
		JAILHOUSE_SHMEM_NET_REGIONS(0x07e100000, 0),
		/* FPGA configuration ports */ {
			.phys_start = 0x80000000,
			.virt_start = 0x80000000,
			.size = 0x00100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
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
			.size = 0x7e000000,
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

		/* Peripherials in LPD with QoS Support */
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
			.name = "afifm6",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_AFIFM6_BASE,
		},

		{
			.name = "dap",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_DAP_BASE,
		},

		{
			.name = "usb0",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_USB0_BASE,
		},

		{
			.name = "usb1",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_USB1_BASE,
		},

		{
			.name = "intiou",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INTIOU_BASE,
		},

		{
			.name = "intcsupmu",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INTCSUPMU_BASE,
		},

		{
			.name = "intlpdinbound",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INTLPDINBOUND_BASE,
		},

		{
			.name = "intlpdocm",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INTLPDOCM_BASE,
		},

		{
			.name = "ib5",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_IB5_BASE,
		},

		{
			.name = "ib6",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_IB6_BASE,
		},
		
		{
			.name = "ib8",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_IB8_BASE,
		},

		{
			.name = "ib0",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_IB0_BASE,
		},

		{
			.name = "ib11",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_IB5_BASE,
		},

		{
			.name = "ib12",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_IB5_BASE,
		},
		
		/* Peripherials in FPD with QoS Support */	
		{
			.name = "intfpdcci",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INTFPDCCI_BASE,
		},

		{
			.name = "intfpdsmmutbu3",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INTFPDSMMUTBU3_BASE,
		},

		{
			.name = "intfpdsmmutbu4",
			.flags = (FLAGS_HAS_REGUL),
			.base = M_INTFPDSMMUTBU4_BASE,
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
			.name = "intfpdsmmutbu5",
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

	.rcpu_devices = {
		{
			.rcpu_id = 0,
			.name = "r5f_0",
			.compatible = "xlnx,zynqmp-r5-remoteproc",
		},
		{
			.rcpu_id = 1,
			.name = "r5f_1",
			.compatible = "xlnx,zynqmp-r5-remoteproc",
		},
	},

};
