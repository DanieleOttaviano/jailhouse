# TO do ... (add multiple inmates to compile)
INMATES := baremetal-demo

# Directories
LIBARMR5 := ../../lib/armr5
LIBARMR5_OBJ :=  $(LIBARMR5)/lib/libxil.a $(LIBARMR5)/lib/libfreertos.a
DEMARMR5 := .
SRCS = $(wildcard $(DEMARMR5)/src/*.c) $(wildcard $(DEMARMR5)/src/bench/$(BENCH)/*.c)  
OBJ = $(DEMARMR5)/obj/
OBJS = 	$(addprefix $(OBJ), $(notdir $(SRCS:.c=.o)))
LD_SRCS = $(DEMARMR5)/src/lscript.ld
INC = -I$(LIBARMR5)/include
INC += -Isrc/bench/$(BENCH)
INC += -DBENCHNAME=$(BENCH)
LIBSRC_MAKEFILES := $(wildcard $(LIBARMR5)/libsrc/*/src)


# Flags
LDFLAGS = -Wl,-T -Wl,$(LD_SRCS) -L$(LIBARMR5)/lib
CFLAGS = -mcpu=cortex-r5  -mfloat-abi=hard  -mfpu=vfpv3-d16 
EXT_CFLAGS = -DARMR5 -Wall -O0 -g3 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -MT"$@" 
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
RM = rm -rf 					# Remove recursively command

# All Target
all: $(patsubst %,%_tcm.bin,$(INMATES)) $(patsubst %,%.bin,$(INMATES)) $(patsubst %,%.elf,$(INMATES)) #baremetal-demo_tcm.bin baremetal-demo.bin 

# Each subdirectory must supply rules for building sources it contributes
 $(LIBARMR5_OBJ): $(shell find $(LIBARMR5)/libsrc -type f -name '*.{c,h}')
	$(MAKE) -C $(LIBARMR5)

# Benchmarks
$(OBJ)%.o: src/bench/$(BENCH)/%.c
	$(CC)  -o $@ $< $(INC) $(CFLAGS) $(EXT_CFLAGS)

$(OBJ)%.o: src/%.c $(LIBARMR5_OBJ)
	$(CC)  -o $@ $< $(INC) $(CFLAGS) $(EXT_CFLAGS)

%.elf: $(OBJS) $(LD_SRCS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS)

%_tcm.bin: %.elf
	$(OBJCOPY) -O binary --only-section=.vectors --only-section=.bootdata $^ $@ 

%.bin: %.elf
	$(OBJCOPY) -O binary --remove-section=.note.gnu.property --remove-section=.vectors --remove-section=.bootdata $^ $@

# Other Targets
clean:
	-$(RM) $(DEMARMR5)/*.bin
	-$(RM) $(DEMARMR5)/*.elf
	-$(RM) $(OBJ)%.o
	-$(RM) $(OBJ)*.d
	-@echo ' '

clean_lib:
	-$(RM) $(LIBARMR5)/lib/*.o
	-$(RM) $(LIBARMR5)/lib/*.d
	-$(RM) $(LIBARMR5)/lib/*.a

clean_bench:
	-$(RM) $(OBJ)*.o
	-$(RM) $(OBJ)*.d
	-@echo ' '

.PHONY: all clean clean_bench clean_lib 