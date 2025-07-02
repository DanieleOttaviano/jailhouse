#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Copyright (c) Siemens AG, 2013-2015
# Copyright (c) Daniele Ottaviano, 2024
#
# Authors:
#  Jan Kiszka <jan.kiszka@siemens.com>
#  Benjamin Block <bebl@mageta.org>
#  Daniele Ottaviano <danieleottaviano97@gmail.com>
#
# dottavia, 2024-07-01: Omnivisor support
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#

obj-m := jailhouse.o

ccflags-y := -I$(src)/../hypervisor/arch/$(SRCARCH)/include \
	     -I$(src)/../hypervisor/include \
	     -I$(src)/../include/arch/$(SRCARCH) \
	     -I$(src)/../include

jailhouse-y := cell.o main.o sysfs.o
jailhouse-$(CONFIG_FPGA_MGR_ZYNQMP_FPGA) += fpga.o	# TODO: Daniele Ottaviano should depend on CONFIG_OMNV_FPGA
jailhouse-$(CONFIG_REMOTEPROC) += rcpu.o			# TODO: Daniele Ottaviano should depend on CONFIG_OMNIVISOR
jailhouse-$(CONFIG_PCI) += pci.o
jailhouse-$(CONFIG_OF) += vpci_template.dtb.o

targets += vpci_template.dtb vpci_template.dtb.S

.SECONDARY: \
        $(obj)/vpci_template.dtb.S \
        $(obj)/vpci_template.dtb
