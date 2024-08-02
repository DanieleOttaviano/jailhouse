/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2014-2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _JAILHOUSE_CELL_CONFIG_H
#define _JAILHOUSE_CELL_CONFIG_H

#include <jailhouse/console.h>
#include <jailhouse/pci_defs.h>
#include <jailhouse/qos-common.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])
#endif

/*
 * Supported architectures of Jailhouse. Used in the header of system and cell
 * configurations, as well as in python tooling for automatic architecture
 * detection.
 */
#define JAILHOUSE_X86		0
#define JAILHOUSE_ARM		1
#define JAILHOUSE_ARM64		2

/*
 * Incremented on any layout or semantic change of system or cell config.
 * Also update formats and HEADER_REVISION in pyjailhouse/config_parser.py.
 */
#define JAILHOUSE_CONFIG_REVISION	14

#define JAILHOUSE_CELL_NAME_MAXLEN	31

#define JAILHOUSE_CELL_PASSIVE_COMMREG	0x00000001
#define JAILHOUSE_CELL_TEST_DEVICE	0x00000002
#define JAILHOUSE_CELL_AARCH32		0x00000004

/*
 * The flag JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED allows inmates to invoke
 * the dbg putc hypercall.
 *
 * If JAILHOUSE_CELL_VIRTUAL_CONSOLE_ACTIVE is set, inmates should use the
 * virtual console. This flag implies JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED.
 */
#define JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED	0x40000000
#define JAILHOUSE_CELL_VIRTUAL_CONSOLE_ACTIVE		0x80000000

#define CELL_FLAGS_VIRTUAL_CONSOLE_ACTIVE(flags) \
	!!((flags) & JAILHOUSE_CELL_VIRTUAL_CONSOLE_ACTIVE)
#define CELL_FLAGS_VIRTUAL_CONSOLE_PERMITTED(flags) \
	!!((flags) & JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED)

#define JAILHOUSE_CELL_DESC_SIGNATURE	"JHCLL"

/**
 * The jailhouse cell configuration.
 *
 * @note Keep Config._HEADER_FORMAT in jailhouse-cell-linux in sync with this
 * structure.
 */
struct jailhouse_cell_desc {
	char signature[5];
	__u8 architecture;
	__u16 revision;

	char name[JAILHOUSE_CELL_NAME_MAXLEN+1];
	__u32 id; /* set by the driver */
	__u32 flags;

	__u32 cpu_set_size;
	__u32 num_memory_regions;
	__u32 num_cache_regions;
	__u32 num_irqchips;
	__u32 num_pio_regions;
	__u32 num_pci_devices;
	__u32 num_pci_caps;
	__u32 num_stream_ids;
	__u32 num_qos_devices;

	__u32 vpci_irq_base;

	__u64 cpu_reset_address;
	__u64 msg_reply_timeout;

	struct jailhouse_console console;
} __attribute__((packed));

#define JAILHOUSE_MEM_READ		0x0001
#define JAILHOUSE_MEM_WRITE		0x0002
#define JAILHOUSE_MEM_EXECUTE		0x0004
#define JAILHOUSE_MEM_DMA		0x0008
#define JAILHOUSE_MEM_IO		0x0010
#define JAILHOUSE_MEM_COMM_REGION	0x0020
#define JAILHOUSE_MEM_LOADABLE		0x0040
#define JAILHOUSE_MEM_ROOTSHARED	0x0080
#define JAILHOUSE_MEM_NO_HUGEPAGES	0x0100
#define JAILHOUSE_MEM_COLORED		0x0200
#define JAILHOUSE_MEM_COLORED_NO_COPY	0x0400
/* Set internally for remap_to/unmap_from root ops */
#define JAILHOUSE_MEM_TMP_ROOT_REMAP	0x0800
#define JAILHOUSE_MEM_IO_UNALIGNED	0x8000
#define JAILHOUSE_MEM_IO_WIDTH_SHIFT	16 /* uses bits 16..19 */
#define JAILHOUSE_MEM_IO_8		(1 << JAILHOUSE_MEM_IO_WIDTH_SHIFT)
#define JAILHOUSE_MEM_IO_16		(2 << JAILHOUSE_MEM_IO_WIDTH_SHIFT)
#define JAILHOUSE_MEM_IO_32		(4 << JAILHOUSE_MEM_IO_WIDTH_SHIFT)
#define JAILHOUSE_MEM_IO_64		(8 << JAILHOUSE_MEM_IO_WIDTH_SHIFT)

struct jailhouse_memory {
	__u64 phys_start;
	__u64 virt_start;
	__u64 size;
	__u64 flags;
	/* only meaningful with JAILHOUSE_MEM_COLORED */
	__u64 colors;
} __attribute__((packed));

struct jailhouse_coloring {
	/* Size of a way to use for coloring */
	__u64 way_size;
	/* Temp offset in the root cell to simplify loading of colored cells */
	__u64 root_map_offset;
} __attribute__((packed));

#define JAILHOUSE_SHMEM_NET_REGIONS(start, dev_id)			\
	{								\
		.phys_start = start,					\
		.virt_start = start,					\
		.size = 0x1000,						\
		.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_ROOTSHARED,	\
	},								\
	{ 0 },								\
	{								\
		.phys_start = (start) + 0x1000,				\
		.virt_start = (start) + 0x1000,				\
		.size = 0x7f000,					\
		.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_ROOTSHARED | \
			((dev_id == 0) ? JAILHOUSE_MEM_WRITE : 0),	\
	},								\
	{								\
		.phys_start = (start) + 0x80000,			\
		.virt_start = (start) + 0x80000,			\
		.size = 0x7f000,					\
		.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_ROOTSHARED | \
			((dev_id == 1) ? JAILHOUSE_MEM_WRITE : 0),	\
	}

#define JAILHOUSE_MEMORY_IS_SUBPAGE(mem)	\
	((mem)->virt_start & PAGE_OFFS_MASK || (mem)->size & PAGE_OFFS_MASK)

#define JAILHOUSE_CACHE_L3_CODE		0x01
#define JAILHOUSE_CACHE_L3_DATA		0x02
#define JAILHOUSE_CACHE_L3		(JAILHOUSE_CACHE_L3_CODE | \
					 JAILHOUSE_CACHE_L3_DATA)

#define JAILHOUSE_CACHE_ROOTSHARED	0x0001

struct jailhouse_cache {
	__u32 start;
	__u32 size;
	__u8 type;
	__u8 padding;
	__u16 flags;
} __attribute__((packed));

struct jailhouse_irqchip {
	__u64 address;
	__u32 id;
	__u32 pin_base;
	__u32 pin_bitmap[4];
} __attribute__((packed));

#define JAILHOUSE_PCI_TYPE_DEVICE	0x01
#define JAILHOUSE_PCI_TYPE_BRIDGE	0x02
#define JAILHOUSE_PCI_TYPE_IVSHMEM	0x03

#define JAILHOUSE_SHMEM_PROTO_UNDEFINED		0x0000
#define JAILHOUSE_SHMEM_PROTO_VETH		0x0001
#define JAILHOUSE_SHMEM_PROTO_CUSTOM		0x4000	/* 0x4000..0x7fff */
#define JAILHOUSE_SHMEM_PROTO_VIRTIO_FRONT	0x8000	/* 0x8000..0xbfff */
#define JAILHOUSE_SHMEM_PROTO_VIRTIO_BACK	0xc000	/* 0xc000..0xffff */

#define VIRTIO_DEV_NET				1
#define VIRTIO_DEV_BLOCK			2
#define VIRTIO_DEV_CONSOLE			3

struct jailhouse_pci_device {
	__u8 type;
	__u8 iommu;
	__u16 domain;
	__u16 bdf;
	__u32 bar_mask[6];
	__u16 caps_start;
	__u16 num_caps;
	__u8 num_msi_vectors;
	__u8 msi_64bits:1;
	__u8 msi_maskable:1;
	__u16 num_msix_vectors;
	__u16 msix_region_size;
	__u64 msix_address;
	/** First memory region index of shared memory device. */
	__u32 shmem_regions_start;
	/** ID of shared memory device (0..shmem_peers-1). */
	__u8 shmem_dev_id;
	/** Maximum number of peers connected via this shared memory device. */
	__u8 shmem_peers;
	/** PCI subclass and interface ID of shared memory device. */
	__u16 shmem_protocol;
} __attribute__((packed));

#define JAILHOUSE_IVSHMEM_BAR_MASK_INTX			\
	{						\
		0xfffff000, 0x00000000, 0x00000000,	\
		0x00000000, 0x00000000, 0x00000000,	\
	}

#define JAILHOUSE_IVSHMEM_BAR_MASK_MSIX			\
	{						\
		0xfffff000, 0xfffff000, 0x00000000,	\
		0x00000000, 0x00000000, 0x00000000,	\
	}

#define JAILHOUSE_IVSHMEM_BAR_MASK_INTX_64K		\
	{						\
		0xffff0000, 0x00000000, 0x00000000,	\
		0x00000000, 0x00000000, 0x00000000,	\
	}

#define JAILHOUSE_IVSHMEM_BAR_MASK_MSIX_64K		\
	{						\
		0xffff0000, 0xffff0000, 0x00000000,	\
		0x00000000, 0x00000000, 0x00000000,	\
	}

#define JAILHOUSE_PCI_EXT_CAP		0x8000

#define JAILHOUSE_PCICAPS_WRITE		0x0001

struct jailhouse_pci_capability {
	__u16 id;
	__u16 start;
	__u16 len;
	__u16 flags;
} __attribute__((packed));

#define JAILHOUSE_APIC_MODE_AUTO	0
#define JAILHOUSE_APIC_MODE_XAPIC	1
#define JAILHOUSE_APIC_MODE_X2APIC	2

#define JAILHOUSE_MAX_IOMMU_UNITS	8

#define JAILHOUSE_IOMMU_AMD		1
#define JAILHOUSE_IOMMU_INTEL		2
#define JAILHOUSE_IOMMU_SMMUV3		3
#define JAILHOUSE_IOMMU_PVU		4
#define JAILHOUSE_IOMMU_ARM_MMU500	5

struct jailhouse_iommu {
	__u32 type;
	__u64 base;
	__u32 size;

	union {
		struct {
			__u16 bdf;
			__u8 base_cap;
			__u8 msi_cap;
			__u32 features;
		} __attribute__((packed)) amd;

		struct {
			__u64 tlb_base;
			__u32 tlb_size;
		} __attribute__((packed)) tipvu;
	};
} __attribute__((packed));

union jailhouse_stream_id {
	__u32 id;
	struct {
		/* Note: both mask_out and id are only 15 bits wide. */
		__u16 id;
		/* Mask out irrelevant bits in id:
		 * if mask_out[i] == 1, then id[i] is ignored.
		 */
		__u16 mask_out;
	} __attribute__((packed)) mmu500;
} __attribute__((packed));

struct jailhouse_pio {
	__u16 base;
	__u16 length;
} __attribute__((packed));

#define PIO_RANGE(__base, __length)	\
	{				\
		.base = __base,		\
		.length = __length,	\
	}

#define JAILHOUSE_SYSTEM_SIGNATURE	"JHSYS"

/*
 * The flag JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE allows the root cell to read
 * from the virtual console.
 */
#define JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE	0x0001

#define SYS_FLAGS_VIRTUAL_DEBUG_CONSOLE(flags) \
	!!((flags) & JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE)

/**
 * Memguard total number of PMU Interrupts (one for each CPU).
 */
#define JAILHOUSE_MAX_PMU2CPU_IRQ	8

/**
 * Memguard platform support.
 * Currently only for ARMv8 and GICv2.
 */
struct jailhouse_memguard_config {
	/** Total number of interrupt lines for the SoC.
	 *  SGI + PPI + SPI + system/platform interrupts.
	 */
	__u32 num_irqs;
	/** Hypervisor timer to be used as memguard timer.
	 *  CNTHP-associated timer event.
	 */
	__u32 hv_timer;
	/** GICv2 priority range and step for the SoC. */
	__u8 irq_prio_min;
	__u8 irq_prio_max;
	/** Prio increment step considering secure/non-secure modes */
	__u8 irq_prio_step;
	/** Min separation threshold */
	__u8 irq_prio_threshold;
	/** Size of PMU 2 CPU and FIQ 2 CPU mapping */
	__u32 num_pmu_irq;
	/** PMU 2 CPU interrupt mapping */
	__u32 pmu_cpu_irq[JAILHOUSE_MAX_PMU2CPU_IRQ];
} __attribute__((packed));

struct jailhouse_qos {
	__u64 nic_base;
	__u64 nic_size;
} __attribute__((packed));

struct jailhouse_qos_device {
	char name [QOS_DEV_NAMELEN];
	__u8 flags;
	__u32 base;
} __attribute__((packed));


/**
 * General descriptor of the system.
 */
struct jailhouse_system {
	char signature[5];
	__u8 architecture;
	__u16 revision;

	__u32 flags;

	/** Jailhouse's location in memory */
	struct jailhouse_memory hypervisor_memory;
	struct jailhouse_console debug_console;
	struct {
		__u64 pci_mmconfig_base;
		__u8 pci_mmconfig_end_bus;
		__u8 pci_is_virtual;
		__u16 pci_domain;
		/* Disable spectre (CVE-2017-5715) mitigations */
		__u32 no_spectre_mitigation;
		struct jailhouse_iommu iommu_units[JAILHOUSE_MAX_IOMMU_UNITS];
		struct jailhouse_coloring color;
		struct jailhouse_memguard_config memguard;
		struct jailhouse_qos qos;
		union {
			struct {
				__u16 pm_timer_address;
				__u8 apic_mode;
				__u8 padding;
				__u32 vtd_interrupt_limit;
				__u32 tsc_khz;
				__u32 apic_khz;
			} __attribute__((packed)) x86;
			struct {
				u8 maintenance_irq;
				u8 gic_version;
				u8 padding[2];
				u64 gicd_base;
				u64 gicc_base;
				u64 gich_base;
				u64 gicv_base;
				u64 gicr_base;
			} __attribute__((packed)) arm;
		} __attribute__((packed));
	} __attribute__((packed)) platform_info;
	struct jailhouse_cell_desc root_cell;
} __attribute__((packed));

static inline __u32
jailhouse_cell_config_size(struct jailhouse_cell_desc *cell)
{
	return sizeof(struct jailhouse_cell_desc) +
		cell->cpu_set_size +
		cell->num_memory_regions * sizeof(struct jailhouse_memory) +
		cell->num_cache_regions * sizeof(struct jailhouse_cache) +
		cell->num_irqchips * sizeof(struct jailhouse_irqchip) +
		cell->num_pio_regions * sizeof(struct jailhouse_pio) +
		cell->num_pci_devices * sizeof(struct jailhouse_pci_device) +
		cell->num_pci_caps * sizeof(struct jailhouse_pci_capability) +
		cell->num_stream_ids * sizeof(__u32) +
		cell->num_qos_devices * sizeof(struct jailhouse_qos_device);
}

static inline __u32
jailhouse_system_config_size(struct jailhouse_system *system)
{
	return sizeof(*system) - sizeof(system->root_cell) +
		jailhouse_cell_config_size(&system->root_cell);
}

static inline const unsigned long *
jailhouse_cell_cpu_set(const struct jailhouse_cell_desc *cell)
{
	return (const unsigned long *)((const void *)cell +
		sizeof(struct jailhouse_cell_desc));
}

static inline const struct jailhouse_memory *
jailhouse_cell_mem_regions(const struct jailhouse_cell_desc *cell)
{
	return (const struct jailhouse_memory *)
		((void *)jailhouse_cell_cpu_set(cell) + cell->cpu_set_size);
}

static inline const struct jailhouse_cache *
jailhouse_cell_cache_regions(const struct jailhouse_cell_desc *cell)
{
	return (const struct jailhouse_cache *)
		((void *)jailhouse_cell_mem_regions(cell) +
		 cell->num_memory_regions * sizeof(struct jailhouse_memory));
}

static inline const struct jailhouse_irqchip *
jailhouse_cell_irqchips(const struct jailhouse_cell_desc *cell)
{
	return (const struct jailhouse_irqchip *)
		((void *)jailhouse_cell_cache_regions(cell) +
		 cell->num_cache_regions * sizeof(struct jailhouse_cache));
}

static inline const struct jailhouse_pio *
jailhouse_cell_pio(const struct jailhouse_cell_desc *cell)
{
	return (const struct jailhouse_pio *)
		((void *)jailhouse_cell_irqchips(cell) +
		cell->num_irqchips * sizeof(struct jailhouse_irqchip));
}

static inline const struct jailhouse_pci_device *
jailhouse_cell_pci_devices(const struct jailhouse_cell_desc *cell)
{
	return (const struct jailhouse_pci_device *)
		((void *)jailhouse_cell_pio(cell) +
		 cell->num_pio_regions * sizeof(struct jailhouse_pio));
}

static inline const struct jailhouse_pci_capability *
jailhouse_cell_pci_caps(const struct jailhouse_cell_desc *cell)
{
	return (const struct jailhouse_pci_capability *)
		((void *)jailhouse_cell_pci_devices(cell) +
		 cell->num_pci_devices * sizeof(struct jailhouse_pci_device));
}

static inline const union jailhouse_stream_id *
jailhouse_cell_stream_ids(const struct jailhouse_cell_desc *cell)
{
	return (const union jailhouse_stream_id *)
		((void *)jailhouse_cell_pci_caps(cell) +
		cell->num_pci_caps * sizeof(struct jailhouse_pci_capability));
}

static inline const struct jailhouse_qos_device *
jailhouse_cell_qos_devices(const struct jailhouse_cell_desc *cell)
{
	return (const struct jailhouse_qos_device *)
		((void *)jailhouse_cell_stream_ids(cell) +
		 cell->num_stream_ids * sizeof(union jailhouse_stream_id));
}

#endif /* !_JAILHOUSE_CELL_CONFIG_H */
