PROJECT_BASE ?= $(shell pwd)
ARCH?=x86

export ARCH VPFILE QEMU_EXTRA_FLAGS

ifdef BSP_VARIANT
VARIANT_STR=_$(BSP_VARIANT)
endif

ifdef BSP
ifdef KERNEL_TYPE
KBUILD_DEFCONFIG=$(KERNEL_TYPE)_$(BSP)$(VARIANT_STR)_defconfig
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
export SOURCE_DIR
endif

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

all: $(CONFIG_DEPS)
	$(Q)$(MAKE) -C $(TIMO_BASE)  O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(SOURCE_DIR) CFLAGS=$(CFLAGS)

rm-files:= final-linker.cmd linker.cmd modules.order \
	staticIdt.o System.map *kernel.lnk \
	*kernel.map *kernel.elf qemu.pid \
	.tinymountain.cmd
rm-objects:= *.o
rm-dirs := arch drivers include kernel lib misc \
	scripts source net .tmp_versions

cmd_clean_inner_files = \
	 $(shell cd $(PROJECT_BASE);rm $(rm-files) -f; rm $(rm-dirs) -rf)

clean: FORCE
	@rm $(SOURCE_DIR)$(rm-objects) -f
	@rm $(SOURCE_DIR)/modules.order -f
	$(call cmd_clean_inner_files)

pristine: distclean

distclean: clean
	@rm $(PROJECT_BASE)/.config -f
	@rm $(PROJECT_BASE)/.config.old -f
	@rm $(PROJECT_BASE)/.version -f

mrproper: FORCE
	$(call cmd_clean_inner_files)
	$(Q)$(MAKE) -C $(TIMO_BASE) \
	PROJECT=$(PROJECT_BASE) mrproper

%config: FORCE
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) $@

qemu: $(CONFIG_DEPS)
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(SOURCE_DIR) qemu

initconfig:
	set -e;
	$(Q)test -s .config || $(Q) $(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) defconfig
ifneq (($strip $(CONF_FILE)),)
	$(Q)$(CONFIG_SHELL) $(TIMO_BASE)/scripts/kconfig/merge_config.sh \
	-m -O $(PROJECT_BASE) $(PROJECT_BASE)/.config $(CONF_FILE)
endif
	$(Q)yes "" | $(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) oldconfig


PHONY += FORCE clean mrproper
FORCE:

.PHONY: $(PHONY)
