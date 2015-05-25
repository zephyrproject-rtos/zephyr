PROJECT_BASE ?= $(shell pwd)
ARCH?=x86

export ARCH VPFILE QEMU_EXTRA_FLAGS PROJECT_BASE

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

O ?= $(PROJECT_BASE)/outdir

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

all: $(CONFIG_DEPS) $(O)/include/config/.dir
	$(Q)$(MAKE) -C $(TIMO_BASE)  O=$(O) \
	PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(SOURCE_DIR) CFLAGS=$(CFLAGS)

rm-files:= .config
rm-objects:= *.o
rm-dirs := $(O)

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

%config: $(O)/include/config/.dir FORCE
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(O) \
	PROJECT=$(PROJECT_BASE) $@

qemu: $(CONFIG_DEPS) $(O)/include/config/.dir
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(O) \
	PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(SOURCE_DIR) CFLAGS=$(CFLAGS) qemu

initconfig: $(O)/include/config/.dir $(O)/include/generated/.dir
	set -e;
	$(Q)test -s $(O)/.config || $(Q) $(MAKE) -C $(TIMO_BASE) O=$(O) \
	PROJECT=$(PROJECT_BASE) defconfig
ifneq (($strip $(CONF_FILE)),)
	$(Q)$(CONFIG_SHELL) $(TIMO_BASE)/scripts/kconfig/merge_config.sh \
	-q -m -O $(O) $(O)/.config $(CONF_FILE)
endif
	$(Q)yes "" | $(MAKE) -C $(TIMO_BASE) O=$(O) \
	PROJECT=$(PROJECT_BASE) oldconfig

%/.dir:
	set -e;
	$(Q)test -s $(abspath $(dir $@)) || $(Q)mkdir $(abspath $(dir $@)) -p
	@touch $@

help:
	$(Q)$(MAKE) -C $(TIMO_BASE) help

PHONY += FORCE clean mrproper
FORCE:

.PHONY: $(PHONY)
.PRECIOUS: %/.dir
