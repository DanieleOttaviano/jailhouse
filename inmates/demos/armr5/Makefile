# TO do ... (add multiple inmates to compile)
INMATE := baremetal-demo

# Directories
LIBARMR5 := ../../lib/armr5
DEMARMR5 := .
SRCS = $(wildcard $(DEMARMR5)/src/*.c)
OBJ = $(DEMARMR5)/obj/
OBJS = 	$(addprefix $(OBJ), $(notdir $(SRCS:.c=.o)))
LD_SRCS = $(DEMARMR5)/src/lscript.ld
INC = -I$(LIBARMR5)/include
LIBSRC_MAKEFILES := $(wildcard $(LIBARMR5)/libsrc/*/src)


# Flags
LDFLAGS = -Wl,-T -Wl,$(LD_SRCS) -L$(LIBARMR5)/lib
CFLAGS = -mcpu=cortex-r5  -mfloat-abi=hard  -mfpu=vfpv3-d16 
EXT_CFLAGS = -DARMR5 -Wall -O0 -g3 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -MT"$@" 
LIBS = 	-Wl,--start-group,-lxil,-lfreertos,-lgcc,-lc,--end-group

# Toolchain
#COMPILER_PREFIX = armr5-none-eabi-
COMPILER_PREFIX = $(REMOTE_COMPILE)
CC = $(COMPILER_PREFIX)gcc 
LD = $(COMPILER_PREFIX)ld
AR = $(COMPILER_PREFIX)ar
AS = $(COMPILER_PREFIX)as
OBJDUMP = $(COMPILER_PREFIX)objdump
OBJCOPY = $(COMPILER_PREFIX)objcopy

# Misc
RM      = rm -rf 					# Remove recursively command

# All Target
all: $(INMATE)_tcm.bin $(INMATE).bin

# Each subdirectory must supply rules for building sources it contributes
libs:
	$(MAKE) -C $(LIBARMR5)

$(OBJ)%.o: src/%.c libs
	$(CC)  -o $@ $< $(INC) $(CFLAGS) $(EXT_CFLAGS)

$(INMATE).elf: $(OBJS) $(LD_SRCS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS)

$(INMATE)_tcm.bin: $(INMATE).elf
	$(OBJCOPY) -O binary --only-section=.vectors --only-section=.bootdata $^ $@ 

$(INMATE).bin: $(INMATE).elf
	$(OBJCOPY) -O binary --remove-section=.note.gnu.property --remove-section=.vectors --remove-section=.bootdata $^ $@

# Other Targets
clean:
	-$(RM) $(DEMARMR5)/*.bin
	-$(RM) $(DEMARMR5)/*.elf
	-$(RM) $(LIBARMR5)/lib/*.o
	-$(RM) $(LIBARMR5)/lib/*.d
	-$(RM) $(LIBARMR5)/lib/*.a
	-$(RM) $(OBJS)
	-$(RM) $(OBJ)*.d
	-@echo ' '

.PHONY: all clean