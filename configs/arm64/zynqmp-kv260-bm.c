/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for APU demo inmate on Xilinx Kria kv260
 *
 * Copyright (C) Daniele Ottaviano, 2024
 *
 * Authors:
 *  Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 * 
 * jailhouse cell create jailhouse/configs/arm64/zynqmp-kv260-bm.cell 
 * jailhouse cell load inmate-demo-APU jailhouse/inmates/tools/arm64/linux-loader.bin -a 0x0 -s "kernel=0x47000000" -a 0x1000 kernel.bin -a 0x47000000
 * jailhouse cell start inmate-demo-APU
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[6];
	struct jailhouse_irqchip irqchips[1];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.name = "inmate-demo-APU",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG,

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = ARRAY_SIZE(config.irqchips),
		.num_pci_devices = 0,
	
		.console = {
			.address = 0xff010000,
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_XUARTPS,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0x1,
	},
	.mem_regions = {
		/* UART */ {
			.phys_start = 0xff010000,
			.virt_start = 0xff010000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* GIC-FREERTOS*/ {
			.phys_start = 0x03001000,
			.virt_start = 0x03001000,
			.size = 0x10000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* TIMER */ {
			.phys_start = 0xff110000,
			.virt_start = 0xff110000,
			.size = 0x20000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* Linux Loader */ {
			.phys_start = 0x46d00000,
			.virt_start = 0,
			.size = 0x10000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},		
		/* RAM */ {
			.phys_start = 0x47000000,
			.virt_start = 0x47000000,
			.size = 0x8000000, //0x20000
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
				0, //(1 << (54 - 32)),								 // UART Interrupt line
				(1 << (68 - 64) | 1 << (69 - 64) | 1 << (70 - 64)),  // Interrupts lines of the Timer TTC-0
				0,
				0
			},
		},
	},

};
