# vim: filetype=make
#

DQUOTE = "
# "

ARCH ?= x86

PROJECT_BASE ?= $(CURDIR)
O ?= $(PROJECT_BASE)/outdir

# Turn O into an absolute path; we call the main Kbuild with $(MAKE) -C
# which changes the working directory, relative paths don't work right.
# Need to create the directory first to make realpath happy

ifneq ($(MAKECMDGOALS),help)
$(shell mkdir -p $(O))
override O := $(realpath $(O))
endif

export ARCH MDEF_FILE QEMU_EXTRA_FLAGS PROJECT_BASE

KERNEL_TYPE ?= micro
override CONF_FILE := $(strip $(subst $(DQUOTE),,$(CONF_FILE)))

ifdef BOARD
KBUILD_DEFCONFIG_PATH=$(ZEPHYR_BASE)/boards/$(BOARD)/$(BOARD)_defconfig
export KBUILD_DEFCONFIG_PATH
else
$(error BOARD is not defined!)
endif

SOURCE_DIR ?= $(PROJECT_BASE)/src/
# Kbuild doesn't work correctly if this is an absolute path
# FIXME Do not depend on python
override SOURCE_DIR := $(shell python -c "import os.path; print(\"%s\" % os.path.relpath(os.path.realpath('$(SOURCE_DIR)'), os.path.realpath('$(ZEPHYR_BASE)')))")/
override SOURCE_DIR := $(subst \,/,$(SOURCE_DIR))
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

zephyrmake	= @$(MAKE) -C $(ZEPHYR_BASE)  O=$(1) CFLAGS=$(CFLAGS) \
			PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(SOURCE_DIR) $(2)

DOTCONFIG = $(O)/.config

all: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

ifeq ($(findstring qemu_,$(BOARD)),)
qemu:
	@echo "Emulation not available for this platform"
else
qemu: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)
endif


flash: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

initconfig: $(DOTCONFIG)

menuconfig: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

help:
	$(Q)$(MAKE) -s -C $(ZEPHYR_BASE) $@

# Catch all
%:
	$(Q)$(call zephyrmake,$(O),$@)

KERNEL_CONFIG = $(ZEPHYR_BASE)/kernel/configs/$(KERNEL_TYPE).config

$(DOTCONFIG): $(KBUILD_DEFCONFIG_PATH) $(CONF_FILE)
	$(Q)$(CONFIG_SHELL) $(ZEPHYR_BASE)/scripts/kconfig/merge_config.sh \
		-q -m -O $(O) $(KBUILD_DEFCONFIG_PATH) $(KERNEL_CONFIG) $(CONF_FILE)
	$(Q)$(MAKE) $(S) -C $(ZEPHYR_BASE) O=$(O) PROJECT=$(PROJECT_BASE) oldnoconfig

pristine:
	$(Q)rm -rf $(O)

PHONY += FORCE initconfig
FORCE:

.PHONY: $(PHONY)
