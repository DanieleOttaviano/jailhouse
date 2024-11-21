# Directories to build
SUBDIRS := $(wildcard src_*/)

# Targets
all:
	$(foreach dir, $(SUBDIRS), $(MAKE) -C $(dir);)

clean:
	$(foreach dir, $(SUBDIRS), $(MAKE) -C $(dir) clean;)

.PHONY: all clean