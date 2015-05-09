PROJECT_BASE ?= $(shell pwd)
ARCH?=x86

export ARCH VPFILE

ifdef BSP
ifdef KERNEL_TYPE
KBUILD_DEFCONFIG=$(KERNEL_TYPE)_$(BSP)_defconfig
export KBUILD_DEFCONFIG
endif
endif

ifdef KBUILD_DEFCONFIG
CONFIG_DEPS=initconfig
else
CONFIG_DEPS=FORCE
endif

ifndef SOURCE_DIR
SOURCE_DIR=$(PROJECT_BASE)/src/
endif

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

all: $(CONFIG_DEPS)
	$(Q)$(MAKE) -C $(TIMO_BASE)  O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(SOURCE_DIR)

rm-files:= final-linker.cmd linker.cmd modules.order \
	staticIdt.o System.map *kernel.lnk \
	*kernel.map *kernel.elf
rm-objects:= *.o
rm-dirs := arch drivers include kernel lib misc \
	scripts source

cmd_clean_inner_files = \
	 $(shell cd $(PROJECT_BASE);rm $(rm-files) -f; rm $(rm-dirs) -rf)

clean: FORCE
	@rm $(SOURCE_DIR)$(rm-objects) -f
	$(call cmd_clean_inner_files)

mrproper: FORCE
	$(call cmd_clean_inner_files)
	$(Q)$(MAKE) -C $(TIMO_BASE) \
	PROJECT=$(PROJECT_BASE) mrproper

%config: FORCE
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) $@

qemu: $(CONFIG_DEPS)
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) qemu

cmd_defconfig = \
	$(Q)test -s .config || $(Q) $(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) defconfig

cmd_merge_config = set -e; \
	$(call cmd_defconfig) \
	$(Q)test -s $(CONF_OVERLAY) || \
	$(Q)$(CONFIG_SHELL) $(TIMO_BASE)/scripts/kconfig/merge_config.sh \
	-m -O $(PROJECT_BASE) $(PROJECT_BASE)/.config $(CONF_OVERLAY)

initconfig:
	$(call cmd_merge_config)
	$(Q)yes "" | $(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) oldconfig


PHONY += FORCE clean mrproper
FORCE:

.PHONY: $(PHONY)
