#
# Jailhouse, a Linux-based partitioning hypervisor
#
# Omnivisor demo RPU Makefile
#
# Copyright (c) Daniele Ottaviano, 2024
#
# Authors:
#   Daniele Ottaviano <danieleottaviano97@gmail.com>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#

DIR_NAME := $(notdir $(CURDIR))  # Name of the current directory
INMATE := $(patsubst src_%, %, $(DIR_NAME))  # Remove "src_" prefix for the target name

# Directories
LIBARMR5 := ../../../lib/armr5
LIBARMR5_OBJ :=  $(LIBARMR5)/lib/libxil.a $(LIBARMR5)/lib/libfreertos.a
SRCS = $(wildcard *.c) $(wildcard bench/$(BENCH)/*.c)  
OBJS = $(SRCS:.c=.o)
LD_SRCS = lscript.ld
INC = -I$(LIBARMR5)/include
INC += -Ibench/$(BENCH)
INC += -DBENCHNAME=$(BENCH)

# Flags
LDFLAGS = -Wl,-T -Wl,$(LD_SRCS) -L$(LIBARMR5)/lib 
CFLAGS = -mcpu=cortex-r5  -mfloat-abi=hard  -mfpu=vfpv3-d16 
EXT_CFLAGS = -DARMR5 -Wall -O2 -DNDEBUG -g3 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -MT"$@" 
LIBS = 	-Wl,--start-group,-lxil,-lfreertos,-lgcc,-lc,--end-group

# Toolchain
COMPILER_PREFIX = $(REMOTE_COMPILE)
CC = $(COMPILER_PREFIX)gcc 
LD = $(COMPILER_PREFIX)ld
AR = $(COMPILER_PREFIX)ar
AS = $(COMPILER_PREFIX)as
OBJDUMP = $(COMPILER_PREFIX)objdump
OBJCOPY = $(COMPILER_PREFIX)objcopy

# Misc
RM = rm -rf # Remove recursively command

# All Target
all: $(patsubst %,%-demo.elf,$(INMATE))  

# Each subdirectory must supply rules for building sources it contributes
$(LIBARMR5_OBJ): $(shell find $(LIBARMR5)/libsrc -type f -name '*.{c,h}')
	$(MAKE) -C $(LIBARMR5)

# Benchmarks
%.o: bench/$(BENCH)/%.c
	$(CC)  -o $@ $< $(INC) $(CFLAGS) $(EXT_CFLAGS)

%.o: %.c $(LIBARMR5_OBJ)
	$(CC)  -o $@ $< $(INC) $(CFLAGS) $(EXT_CFLAGS)

%.elf: $(OBJS) $(LD_SRCS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS)

# Other Targets
clean:
	-$(RM) *.elf
	-$(RM) *.o
	-$(RM) *.d
	-@echo 'Cleaned'

clean_lib:
	-$(RM) $(LIBARMR5)/lib/*.o
	-$(RM) $(LIBARMR5)/lib/*.d
	-$(RM) $(LIBARMR5)/lib/*.a
	-@echo 'Lib cleaned'
	
clean_bench:
	-$(RM) *.o
	-$(RM) *.d
	-@echo ' '

.PHONY: all clean clean_bench clean_lib 