/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for NXP S32V234 SBC with QoS configuration
 *
 * Copyright (C) 2016 Evidence Srl
 * Copyright (C) Boston University, MA, USA, 2020
 * Copyright (C) Technical University of Munich, 2020 - 2021
 *
 * Authors:
 *  Claudio Scordino <claudio@evidence.eu.com>
 *  Bruno Morelli <b.morelli@evidence.eu.com>
 *  Renato Mancuso <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * NOTE: Add "mem=1024M" to the kernel command line.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>
#include <asm/qos-400.h>

/* List QoS register offsets here, so that we don't have to search
 * them in the manuals each time...
 */
#define M_FASTDMA1_BASE    (0x2380)
#define M_GPU0_BASE        (0x2480)
#define M_H264DEC_BASE     (0x2580)
#define M_GPU1_BASE        (0x2680)
#define M_CORES_BASE       (0x2780)
#define M_PDI0_BASE        (0x3180)

#define PCI_IB19_BASE      (0x6280)
#define APEX1_IB15_BASE    (0x6380)
#define APEX0_IB16_BASE    (0x6480)
#define H264_IB25_BASE     (0x6580)
#define ENET_IB12_BASE     (0x6680)
#define AXBS_IB36_BASE     (0x6A80)


struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[10];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
	struct jailhouse_qos_device qos_devices[12];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0xfc000000,
			.size = 0x3f00000,
		},
		.debug_console = {
			.address = 0x40053000,
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_LINFLEX,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
			         JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			.pci_mmconfig_base = 0x7e100000,
			.pci_mmconfig_end_bus = 0,
			.pci_is_virtual = 1,
			.pci_domain = -1,
			.arm = {
				.gic_version = 2,
				.gicd_base = 0x7d001000,
				.gicc_base = 0x7d002000,
				.gich_base = 0x7d004000,
				.gicv_base = 0x7d006000,
				.maintenance_irq = 25,
			},
			.memguard = {
				/* For this SoC we have:
				   - 32 SGIs and PPIs
				   - 8 SPIs
				   - 16 on-platform vectors
				   - 152 off-platform vectors
				   ------ Total = 208
				   */
				.num_irqs = 208,
				.hv_timer = 26,
				/* All levels are implemented */
				.irq_prio_min = 0xff,
				.irq_prio_max = 0x00,
				/* secure = NS */
				.irq_prio_step = 0x01,
				.irq_prio_threshold = 0x10,
				.num_pmu_irq = 4,
				/* One PMU irq per CPU */
				.pmu_cpu_irq = {
					195, 196, 197, 198,
				},
			},
			.qos = {
				.nic_base = 0x40010000,
				/* 64KB Aperture */
				.nic_size = 0x10000,
			},
		},
		.root_cell = {
			.name = "NXP S32V234",
			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),
			.num_qos_devices = ARRAY_SIZE(config.qos_devices),
			/* The GICv2 supports up to 480
			 * interrupts. The S32 uses up to 207.
			 * The root cell will use from 212 to 217.
			 * Note: Jailhouse adds 32 (GIC's SPI) to the
			 * .vpci_irq_base, so 180 is the base value
			 */
			.vpci_irq_base = 180,
		},
	},

	.cpus = {
		0xf,
	},

	.mem_regions = {
		/* IVSHMEM shared memory region for 0001:00:00.0 */
		JAILHOUSE_SHMEM_NET_REGIONS(0xfff00000, 0),

		/* MMIO (permissive) */ {
			.phys_start = 0x40000000,
			.virt_start = 0x40000000,
			.size =	      0x00100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},

		/* DDR0 256 */ {
			.phys_start = 0x80000000,
			.virt_start = 0x80000000,
			.size =       0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* DDR0 256 */ {
			.phys_start = 0x90000000,
			.virt_start = 0x90000000,
			.size =       0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* DDR0 256 */ {
			.phys_start = 0xa0000000,
			.virt_start = 0xa0000000,
			.size =       0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* DDR0 256 */ {
			.phys_start = 0xb0000000,
			.virt_start = 0xb0000000,
			.size =       0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* Inmates High DDR1 */ {
			.phys_start = 0xc0000000,
			.virt_start = 0xc0000000,
			.size =       0x40000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
	},
	.irqchips = {
		/* GIC */ {
			.address = 0x7d001000,
			.pin_base = 32,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		}
	},

	.pci_devices = {
		/* 0001:00:00.0 */ {
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 1,
			.bdf = 0x00,
			.bar_mask = {
				0xffffff00, 0xffffffff, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.shmem_regions_start = 0,
			.shmem_dev_id = 0,
			.shmem_peers = 1,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
		},
	},

	.qos_devices = {
		{
			.name = "fastdma1",
			.flags = (FLAGS_HAS_RWQOS | FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = M_FASTDMA1_BASE,
		},
		{
			.name = "gpu0",
			.flags = (FLAGS_HAS_RWQOS | FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = M_GPU0_BASE,
		},
		{
			.name = "h264dec0",
			.flags = (FLAGS_HAS_RWQOS | FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = M_H264DEC_BASE,
		},
		{
			.name = "gpu1",
			.flags = (FLAGS_HAS_RWQOS | FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = M_GPU1_BASE,
		},
		{
			.name = "cores",
			.flags = (FLAGS_HAS_RWQOS | FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = M_CORES_BASE,
		},
		{
			.name = "pdi0",
			.flags = (FLAGS_HAS_RWQOS | FLAGS_HAS_REGUL),
			.base = M_PDI0_BASE,
		},
		{
			.name = "pci",
			.flags = (FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = PCI_IB19_BASE,
		},
		{
			.name = "apex1",
			.flags = (FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = APEX1_IB15_BASE,
		},
		{
			.name = "apex0",
			.flags = (FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = APEX0_IB16_BASE,
		},
		{
			.name = "h264dec1",
			.flags = (FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = H264_IB25_BASE,
		},
		{
			.name = "enet",
			.flags = (FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = ENET_IB12_BASE,
		},
		{
			.name = "axbs",
			.flags = (FLAGS_HAS_REGUL | FLAGS_HAS_DYNQOS),
			.base = AXBS_IB36_BASE,
		},
	},
};
