/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for demo inmate on Xilinx ZynqMP KRIA kV260 eval board:
 * 1 Soft-Core RISCV CPU on FPGA
 *
 * Copyright (c) Siemens AG, 2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	__u64 rcpus[1];
	__u64 fpga_regions[1];
	struct jailhouse_memory mem_regions[4];
	union jailhouse_stream_id stream_ids[1];
	struct jailhouse_rcpu_device rcpu_devices[1];
	struct jailhouse_fpga_device fpga_devices[1];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.name = "inmate-demo-RISCV",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG,

		.cpu_set_size = sizeof(config.cpus),
		.rcpu_set_size = sizeof(config.rcpus),
		.fpga_regions_size = sizeof(config.fpga_regions),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = 0,
		.num_pci_devices = 0,
		.num_stream_ids = ARRAY_SIZE(config.stream_ids),
		.num_rcpu_devices = ARRAY_SIZE(config.rcpu_devices),
		.num_fpga_devices = ARRAY_SIZE(config.fpga_devices),

		.console = {
			.address = 0xff010000,
			.type = JAILHOUSE_CON_TYPE_XUARTPS,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0x0,
	},

	.rcpus = {
		0x4, // Third core is a soft-core on the FPGA
	},

    .fpga_regions = {
        0x1
    },

	.mem_regions = {
		/* UART not used yet*/ {
			.phys_start = 0xff010000,
			.virt_start = 0xff010000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* RAM */ {
			.phys_start = 0x70000000,
			.virt_start = 0,
			.size = 0x2000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* SHM */ {
			.phys_start = 0x46d00000,
			.virt_start = 0x46d00000,
			.size = 0x10000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED, 
		},
		/* communication region */ {
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
	},

	.stream_ids = {
		{
			.mmu500.id = 0x1280,	 // [14:10] TBU3 ID 00100 , [9:0] FPGA Master Stream-ID 1010xxxxx 
			.mmu500.mask_out = 0x3f, // Mask out bits 0..5
		},	
	},

	.rcpu_devices = {
		{
			.rcpu_id = 2,
			.name = "pico32",
			.compatible = "dottavia,pico32-remoteproc",
		},
	},

	.fpga_devices = {

		{
			.fpga_dto = "pico32.dtbo",						// load device tree overlay pico32_overlay
			.fpga_module = "pico32_remoteproc",				// load module pico32_remoteproc
			.fpga_bitstream = "pico32_tg_dynamic_pico.bit",	// load bitstream pico32
			.fpga_region_id = 0,							// load bitstream in region with id x
			.fpga_rcpus_set_size = 1,						// add 1 rcpu
			.fpga_conf_addr = 0x80000000,					// configuration address for region
		},

	},

};
