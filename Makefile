#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Omnivisor branch for remote core virtualization
#
# Copyright (c) Siemens AG, 2013, 2014
# Copyright (c) Daniele Ottaviano, 2024
#
# Authors:
#  Jan Kiszka <jan.kiszka@siemens.com>
#  Daniele Ottaviano <danieleottaviano97@gmail.com>
#
# dottavia, 2024-07-01: Compiler for remote cores demos on Cortex-R5 and RISC-V
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#

# Check make version
need := 3.82
ifneq ($(need),$(firstword $(sort $(MAKE_VERSION) $(need))))
$(error Too old make version $(MAKE_VERSION), at least $(need) required)
endif

# no recipes above this one (also no includes)
all: modules

# includes installation-related variables and definitions
include scripts/include.mk

# out-of-tree build for our kernel-module, firmware and inmates
KDIR ?= /lib/modules/`uname -r`/build

INSTALL_MOD_PATH ?= $(DESTDIR)
export INSTALL_MOD_PATH

DOXYGEN ?= doxygen

kbuild = -C $(KDIR) M=$$PWD $@

ifneq ($(DESTDIR),)
PIP_ROOT = --root=$(shell readlink -f $(DESTDIR))
endif

modules clean:
	$(Q)$(MAKE) $(kbuild)

remote: remote_armr5 remote_riscv32

clean-remote: clean-remote_armr5 clean-remote_riscv32

remote_armr5:
	$(Q)$(MAKE) -C inmates/demos/armr5

clean-remote_armr5:
	$(Q)$(MAKE) -C inmates/demos/armr5 clean
	
remote_riscv32:	
	$(Q)$(MAKE) -C inmates/demos/riscv

clean-remote_riscv32:
	$(Q)$(MAKE) -C inmates/demos/riscv clean

# documentation, build needs to be triggered explicitly
docs:
	$(DOXYGEN) Documentation/Doxyfile

modules_install: modules
	$(Q)$(MAKE) $(kbuild)

firmware_install: $(DESTDIR)$(firmwaredir) modules
	$(INSTALL_DATA) hypervisor/jailhouse*.bin $<

tool_inmates_install: $(DESTDIR)$(libexecdir)/jailhouse
	$(INSTALL_DATA) inmates/tools/$(ARCH)/*.bin $<

install: modules_install firmware_install tool_inmates_install
	$(Q)$(MAKE) -C tools $@ src=.
ifeq ($(strip $(PYTHON_PIP_USABLE)), yes)
	$(PIP) install --upgrade --force-reinstall $(PIP_ROOT) .
endif

.PHONY: modules_install install clean firmware_install modules tools docs \
	docs_clean remote clean-remote
