# TO do ... (add multiple inmates to compile)
INMATES := riscv-demo

# Directories
DEMRISCV := .
ASMS = $(wildcard $(DEMRISCV)/asm/*.S)  
SRCS = $(wildcard $(DEMRISCV)/src/*.c) $(wildcard $(DEMRISCV)/src/bench/$(BENCH)/*.c)
OBJ = $(DEMRISCV)/obj/
OBJS = 	$(addprefix $(OBJ), $(notdir $(SRCS:.c=.o)))
LD_SRCS = $(DEMRISCV)/src/firmware.ld
OUTPUT_NAME = riscv-demo

# Include
INC = -Iinc
INC += -I$(DEMRISCV)/src/bench/$(BENCH)
INC += -DBENCHNAME=$(BENCH)

# Architecture
CONFIG = rv32i
ABI = ilp32

# Flags
CFLAGS = -O0 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  
LDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  -Wl,-M 
LIBS = -Wl,-lgcc -static-libgcc

# Toolchain
COMPILER_PREFIX = /opt/rv32/bin/riscv32-unknown-elf-
CC = $(COMPILER_PREFIX)gcc 
LD = $(COMPILER_PREFIX)ld
AR = $(COMPILER_PREFIX)ar
AS = $(COMPILER_PREFIX)as
OBJDUMP = $(COMPILER_PREFIX)objdump
OBJCOPY = $(COMPILER_PREFIX)objcopy

.PHONY: all
all:	$(OUTPUT_NAME).bin

$(OBJ)%.o: src/bench/$(BENCH)/%.c
	$(CC) -c -o $@ $< $(INC) $(CFLAGS)

$(OBJ)%.o: src/%.c
	$(CC) -c -o $@ $< $(INC) $(CFLAGS)
	
$(OUTPUT_NAME).elf: $(OBJS)
	$(CC) $(LDFLAGS) -T $(LD_SRCS) -o $@  ./asm/Reset_Handler.S  $^ ./asm/muldi3.S ./asm/div.S $(LIBS)
	
$(OUTPUT_NAME).bin: $(OUTPUT_NAME).elf
	$(OBJCOPY) -O binary $^ $@

.PHONY: dump
dump: $(OUTPUT_NAME).o $(OUTPUT_NAME).elf
	$(OBJDUMP) -S $(OUTPUT_NAME).o | less > $(OUTPUT_NAME).o.dump
	$(OBJDUMP) -S $(OUTPUT_NAME).elf | less > $(OUTPUT_NAME).elf.dump
	
.PHONY: clean
clean:
	-rm obj/*.o
	-rm *.elf
	-rm *.bin
	-rm *.dump
