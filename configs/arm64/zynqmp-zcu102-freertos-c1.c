/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for 2nd linux-demo inmate on ZynqMP ZCU102:
 * 1 CPU, 112M RAM, serial port 2
 *
 * Corresponds to configs/arm64/dts/inmate-zynqmp-zcu102_core2_uart1.dts
 *
 * Copyright (c) Siemens AG, 2014-2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *  Gero Schwaericke <gero.schwaericke@tum.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

 #include <jailhouse/types.h>
 #include <jailhouse/cell-config.h>
 
 struct {
	 struct jailhouse_cell_desc cell;
	 __u64 cpus[1];
	 struct jailhouse_memory mem_regions[3];
	 struct jailhouse_irqchip irqchips[1];
	 struct jailhouse_pci_device pci_devices[1];
 } __attribute__((packed)) config = {
	 .cell = {
		 .signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		 .revision = JAILHOUSE_CONFIG_REVISION,
		 .architecture = JAILHOUSE_ARM64,
		 .name = "FreeRTOS-Core1",
		 .flags = JAILHOUSE_CELL_PASSIVE_COMMREG |
			  JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,
 
		 .cpu_set_size = sizeof(config.cpus),
		 .num_memory_regions = ARRAY_SIZE(config.mem_regions),
		 .num_irqchips = ARRAY_SIZE(config.irqchips),
		 .num_pci_devices = 0,// ARRAY_SIZE(config.pci_devices),
 
		//  .vpci_irq_base = 140-32,
 
		 .console = {
			 .address = 0xff010000,
			 .type = JAILHOUSE_CON_TYPE_XUARTPS,
			 .flags = JAILHOUSE_CON_ACCESS_MMIO |
				  JAILHOUSE_CON_REGDIST_4,
		 },
	 },
 
	 .cpus = {
		 0x2, /* Core 1 */
	 },
 
	 .mem_regions = {
		 /* communication region : 0x8000_0000 - 0x8000_1000 */ {
			 .virt_start = 0x80000000,
			 .size = 0x00001000,
			 .flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				  JAILHOUSE_MEM_COMM_REGION,
		 },
		 /* RAM : 0x8_1000_0000 - 0x8_2000_0000 (256 MB) */ {
			 .phys_start = 0x810000000,
			 .virt_start = 0x0,
			 .size = 0x10000000,
			 .flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				  JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA |
				  JAILHOUSE_MEM_LOADABLE,
		 },
 
		 /* Devices */
 
		 /* UART 0 : 0xff00_0000 - 0xff00_1000 */ {
			 .phys_start = 0xff010000,
			 .virt_start = 0xff010000,
			 .size = 0x1000,
			 .flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				  JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		 },
	 },
 
	 .irqchips = {
		 /* GIC */ {
			 .address = 0xf9010000,
			 .pin_base = 32,
			 .pin_bitmap = {
				 /* IRQ 54 = UART 1 */
				 1 << (54 - 32),
				 0,
				 0,
				 /* IRQ 140 = PL to PS interrupt, used as virtual
				  * pci controller interrupt vpci_irq_base (see
				  * above), will be used by virtio socket device
				  */
				 0, //(1 << (140 - 128)),
			 },
		 },
	 },
 
	//  .pci_devices = {
	// 	 /* Virtio Socket RT - 00:12.0 */ {
	// 		 .type = JAILHOUSE_PCI_TYPE_VIRTIO,
	// 		 .bdf = 12 << 3,
	// 		 .bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
	// 		 .virtio_device_type_id = VIRTIO_DEV_SOCKET,
	// 		 .vsock_guest_cid = 3, // must be > 2
	// 	 },
	//  },
 };
 