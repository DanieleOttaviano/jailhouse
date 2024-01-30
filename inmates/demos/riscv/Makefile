# TO do ... (add multiple inmates to compile)
INMATES := riscv-demo

SOURCE	= ./src/riscv-demo.c
OUTPUT_NAME = riscv-demo
CONFIG = rv32i
ABI = ilp32
LINKER = ./src/firmware.ld
#CROSS = /opt/rv32/bin/riscv32-unknown-elf-

# Flags
CROSSCFLAGS = -O0 -Wno-int-conversion -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  
CROSSLDFLAGS = -march=$(CONFIG) -mabi=$(ABI) -ffreestanding -nostdlib  -Wl,-M 

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

obj/$(OUTPUT_NAME).o: $(SOURCE)
	$(CC) $(CROSSCFLAGS) -c -o $@  $< -lgcc
	
$(OUTPUT_NAME).elf: obj/$(OUTPUT_NAME).o
	$(CC) $(CROSSLDFLAGS) -T $(LINKER) -o $@   ./asm/Reset_Handler.S  $< ./asm/muldi3.S ./asm/div.S 
	
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
