# Directories to build
SUBDIRS := $(wildcard src_*/)

# Toolchain
COMPILER := /opt/rv32/bin/riscv32-unknown-elf-

# Targets
all:
	$(foreach dir, $(SUBDIRS), $(MAKE) -C $(dir) COMPILER_PREFIX=$(COMPILER);)

clean:
	$(foreach dir, $(SUBDIRS), $(MAKE) -C $(dir) clean;)

.PHONY: all clean