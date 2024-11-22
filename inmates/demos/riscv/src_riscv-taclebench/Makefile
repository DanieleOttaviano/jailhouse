#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Omnivisor demo RISC-V Makefile
#
# Copyright (c) Daniele Ottaviano, 2024
#
# Authors:
#   Daniele Ottaviano <danieleottaviano97@gmail.com>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#

INMATE := $(BENCH)  # Remove "src_" prefix for the target name

# Directories
SRCS = $(wildcard *.c) $(wildcard bench/$(BENCH)/*.c)
OBJS = 	$(SRCS:.c=.o)
LD_SRCS = firmware.ld

# Include
INC = -Iinc
INC += -Ibench/$(BENCH)
INC += -DBENCHNAME=$(BENCH)

# Architecture
CONFIG = rv32i
ABI = ilp32

# Flags
CFLAGS = -O0 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  
LDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  -Wl,-M 
LIBS = -Wl,-lgcc -static-libgcc

# Toolchain
CC = $(COMPILER_PREFIX)gcc 
LD = $(COMPILER_PREFIX)ld
AR = $(COMPILER_PREFIX)ar
AS = $(COMPILER_PREFIX)as
OBJDUMP = $(COMPILER_PREFIX)objdump
OBJCOPY = $(COMPILER_PREFIX)objcopy

all: $(patsubst %,riscv-%-demo.bin,$(INMATE))	

%.o: bench/$(BENCH)/%.c
	$(CC) -c -o $@ $< $(INC) $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(INC) $(CFLAGS)
	
%.elf: $(OBJS)
	$(CC) $(LDFLAGS) -T $(LD_SRCS) -o $@  ../asm/Reset_Handler.S  $^ ../asm/muldi3.S ../asm/div.S $(LIBS)
	
%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

dump: $(OUTPUT_NAME).o $(OUTPUT_NAME).elf
	$(OBJDUMP) -S $(OUTPUT_NAME).o | less > $(OUTPUT_NAME).o.dump
	$(OBJDUMP) -S $(OUTPUT_NAME).elf | less > $(OUTPUT_NAME).elf.dump
	
clean:
	-rm obj/*.o
	-rm *.elf
	-rm *.bin
	-rm *.dump

.PHONY: all clean dump
