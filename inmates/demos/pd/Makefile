# pathsearch = $(firstword $(wildcard $(addsuffix /$(1),$(subst :, ,$(PATH)))))

# LS := $(call pathsearch,ls)
# map = $(foreach a,$(2),$(call $(1),$(a)))

# all:
# 	echo $(LS)
# 	echo $(map)
# 	echo $(wildcard $(addsuffix /ls,$(subst :, ,$(PATH))))
	
# 	# echo $($(addsuffix /$(1),$(subst :, ,$(PATH))))

include $(INMATES_LIB)/Makefile.lib

INMATES := gic-demo.bin uart-demo.bin ivshmem-demo.bin boot-demo.bin

gic-demo-y	:= ../arm/gic-demo.o
uart-demo-y	:= ../arm/uart-demo.o
ivshmem-demo-y	:= ../ivshmem-demo.o
boot-demo-y	:= ../arm/boot-demo.o

all:
	echo $(call DECLARE_TARGETS,$(INMATES))