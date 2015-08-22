# vim: filetype=make

PROJECT_BASE ?= $(shell pwd)
O ?= $(PROJECT_BASE)/outdir

ARCH ?= x86

# Turn O into an absolute path; we call the main Kbuild with $(MAKE) -C
# which changes the working directory, relative paths don't work right.
# Need to create the directory first to make readlink happy
$(shell mkdir -p $(O))
override O := $(shell readlink -f $(O))

export ARCH MDEF_FILE QEMU_EXTRA_FLAGS PROJECT_BASE

# FIXME: Simplify this, very ugly
ifdef PLATFORM_CONFIG
ifndef KERNEL_TYPE
$(error KERNEL_TYPE is not defined!  Set it to either micro or nano)
endif
ifndef KBUILD_DEFCONFIG
KBUILD_DEFCONFIG=$(KERNEL_TYPE)_$(PLATFORM_CONFIG)_defconfig
KBUILD_DEFCONFIG_PATH=$(ZEPHYR_BASE)/arch/$(ARCH)/configs/$(KBUILD_DEFCONFIG)
else
KBUILD_DEFCONFIG_PATH=$(KBUILD_DEFCONFIG)
endif
export KBUILD_DEFCONFIG
endif

ifdef KBUILD_DEFCONFIG
CONFIG_DEPS=$(O)/.initconfig
else
CONFIG_DEPS=FORCE
endif

SOURCE_DIR ?= $(PROJECT_BASE)/src/
# Kbuild doesn't work correctly if this is an absolute path
# FIXME Do not depend on python
override SOURCE_DIR := $(shell python -c "import os.path; print(\"%s\" % os.path.relpath(os.path.realpath('$(SOURCE_DIR)'), os.path.realpath('$(ZEPHYR_BASE)')))")/
export SOURCE_DIR



ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  Q =
  S =
else
  Q = @
  S = -s
endif

BEENTHERE = $(O)/.$(ARCH)-$(PLATFORM_CONFIG)-$(KERNEL_TYPE).env

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

zephyrmake	= @$(MAKE) -C $(ZEPHYR_BASE)  O=$(1) CFLAGS="$(CFLAGS)" \
			PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(SOURCE_DIR) $(2)

all: $(CONFIG_DEPS) $(O)/.dir
	$(Q)$(call zephyrmake,$(O),$@)

qemu: $(CONFIG_DEPS) $(O)/.dir
	$(Q)$(call zephyrmake,$(O),$@)


# FIXME: Use defconfig to create directory and copy file
$(O)/.config: $(O)/.dir $(BEENTHERE)
	$(Q)cp $(KBUILD_DEFCONFIG_PATH) $(O)/.config

$(O)/.initconfig: mergeconfig
	$(Q)yes "" | $(MAKE) $(S) -C $(ZEPHYR_BASE) O=$(O) \
		PROJECT=$(PROJECT_BASE) oldconfig
	$(Q)touch $@


# FIXME: Use provided mergeconfig target in kconfig/Makefile
#
ifneq ($(strip $(CONF_FILE)),)
mergeconfig: $(O)/.config $(CONF_FILE)
	$(Q)$(CONFIG_SHELL) $(ZEPHYR_BASE)/scripts/kconfig/merge_config.sh \
		-q -m -O $(O) $(O)/.config $(CONF_FILE)
else
mergeconfig: defconfig $(CONF_FILE)
endif


pristine:
	$(Q)rm -rf $(O)

%/.dir:
	$(Q)set -e;
	$(Q)test -s $(abspath $(dir $@)) || mkdir $(abspath $(dir $@)) -p
	$(Q)touch $@


$(BEENTHERE): FORCE
	$(Q)rm -rf $(O)/.*.env
	$(Q)touch $@


# Catch all
%:
	$(Q)$(call zephyrmake,$(O),$@)

.PHONY: $(PHONY)
.PRECIOUS: %/.dir
