#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Copyright (c) Siemens AG, 2013-2017
#
# Authors:
#  Jan Kiszka <jan.kiszka@siemens.com>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#

include $(ALWAYS_COMPAT_MK)

-include $(GEN_CONFIG_MK)

# includes installation-related variables and definitions
include $(src)/../scripts/include.mk

LD = $(CC) $(KBUILD_CFLAGS)
NOSTDINC_FLAGS :=
LINUXINCLUDE := -I$(src)/../driver -I$(src)/../include
KBUILD_CFLAGS := -g -O3 -DLIBEXECDIR=\"$(libexecdir)\" \
	-Wall -Wextra -Wmissing-declarations -Wmissing-prototypes -Werror \
	-D__LINUX_COMPILER_TYPES_H \
	-DJAILHOUSE_VERSION=\"$(shell cat $(src)/../VERSION)\" $(EXTRA_CFLAGS)
KBUILD_CPPFLAGS :=
# prior to 4.19
LDFLAGS :=
# since 4.19
KBUILD_LDFLAGS :=

# force no-pie for distro compilers that enable pie by default
KBUILD_CFLAGS += $(call cc-option, -fno-pie)
KBUILD_CFLAGS += $(call cc-option, -no-pie)

BINARIES := jailhouse demos/ivshmem-demo
targets += jailhouse.o demos/ivshmem-demo.o

ifeq ($(ARCH),x86)
BINARIES += demos/cache-timings
targets += demos/cache-timings.o
endif # $(ARCH),x86

ifeq ($(ARCH),arm64)
BINARIES += membomb utilstress
targets += membomb.o utilstress.o
endif

always-y := $(BINARIES)

HAS_PYTHON_MAKO := \
	$(shell $(PYTHON3) -c "from mako.template import Template" 2>/dev/null \
	&& echo yes)

ifeq ($(strip $(HAS_PYTHON_MAKO)), yes)
always-y += jailhouse-config-collect
HELPERS := jailhouse-config-collect

else  # !HAS_PYTHON_MAKO

HELPERS :=
$(info WARNING: Could not create the helper script to generate \
	   configurations on remote machines \
	   ("jailhouse-config-collect"). You need Python and the \
	   Mako library for it.)
endif # !HAS_PYTHON_MAKO

ifeq ($(strip $(PYTHON_PIP_USABLE)), yes)
HELPERS += \
	jailhouse-cell-linux \
	jailhouse-cell-stats \
	jailhouse-config-create \
	jailhouse-config-check \
	jailhouse-hardware-check
TEMPLATES := jailhouse-config-collect.tmpl root-cell-config.c.tmpl

install-libexec: $(HELPERS) $(DESTDIR)$(libexecdir)/jailhouse
	$(INSTALL_PROGRAM) $^
	$(Q)$(call patch_dirvar,libexecdir,$(lastword $^)/jailhouse-cell-linux)
	$(Q)$(call patch_dirvar,datadir,$(lastword $^)/jailhouse-config-create)
	$(Q)$(call patch_pyjh_import,$(lastword $^)/jailhouse-cell-linux)
	$(Q)$(call patch_pyjh_import,$(lastword $^)/jailhouse-config-check)

install-data: $(TEMPLATES) $(DESTDIR)$(datadir)/jailhouse
	$(INSTALL_DATA) $^

else  # !PYTHON_PIP_USABLE

install-libexec install-data:
	@

install::
	$(info WARNING: Could not install Python-based helpers. You need \
		   Python and pip in order to install them.)
endif # !PYTHON_PIP_USABLE

MAN8_PAGES := jailhouse.8 jailhouse-cell.8 jailhouse-enable.8

define patch_dirvar
	sed -i 's|^$1 = None|$1 = "$($1)"|' $2
endef

define patch_pyjh_import
	sed -i '/^sys.path\[0\] = .*/d' $1
endef

quiet_cmd_gen_collect = GEN     $@
define cmd_gen_collect
	$< -g $@; \
	chmod +x $@
endef

quiet_cmd_gen_man = GEN     $@
define cmd_gen_man
	sed 's/$${VERSION}/$(shell cat $(src)/../VERSION)/g' $< > $@
endef

$(obj)/%: $(obj)/%.o FORCE
	$(call if_changed,ld)

CFLAGS_jailhouse-gcov-extract.o	:= -I$(src)/../hypervisor/include \
	-I$(src)/../hypervisor/arch/$(SRCARCH)/include
# just change ldflags not cflags, we are not profiling the tool
LDFLAGS_jailhouse-gcov-extract := -lgcov -fprofile-arcs

targets += jailhouse-gcov-extract.o
always-y += jailhouse-gcov-extract

$(obj)/jailhouse-config-collect: $(src)/jailhouse-config-create $(src)/jailhouse-config-collect.tmpl FORCE
	$(call if_changed,gen_collect)

targets += $(MAN8_PAGES)
always-y +=  $(MAN8_PAGES)

$(obj)/%.8: $(src)/%.8.in FORCE
	$(call if_changed,gen_man)

install-bin: $(BINARIES) $(DESTDIR)$(sbindir)
	$(INSTALL_PROGRAM) $^

install-completion: jailhouse-completion.bash $(DESTDIR)$(completionsdir)
	$(INSTALL_DATA) $< $(DESTDIR)$(completionsdir)/jailhouse

install-man8: $(MAN8_PAGES) $(DESTDIR)$(man8dir)
	$(INSTALL_DATA) $^

install:: install-bin install-libexec install-data install-completion \
	 install-man8

.PHONY: install install-bin install-libexec install-data install-completion
