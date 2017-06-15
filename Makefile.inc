# vim: filetype=make
#

UNAME := $(shell uname)
ifeq (MINGW, $(findstring MINGW, $(UNAME)))
DQUOTE = '
# '
PROJECT_BASE ?= $(shell sh -c "pwd -W")
else
DQUOTE = "
# "
PROJECT_BASE ?= $(CURDIR)
endif

ifdef BOARD
KBUILD_DEFCONFIG_PATH=$(wildcard $(ZEPHYR_BASE)/boards/*/*/$(BOARD)_defconfig)
ifeq ($(KBUILD_DEFCONFIG_PATH),)
$(error Board $(BOARD) not found!)
endif
else
$(error BOARD is not defined!)
endif

# Choose a default output directory if one wasn't supplied.  Note that
# PRISTINE_O depends on whether this is default or not.  If building
# in-tree, we want to remove the whole outdir and not just the BOARD
# specified (thus "pristine").  Out of tree, we can obviously remove
# only what we were told to build.
ifndef O
PRISTINE_O = outdir
O = $(PROJECT_BASE)/outdir/$(BOARD)
else
PRISTINE_O = $(O)
endif

# Turn O into an absolute path; we call the main Kbuild with $(MAKE) -C
# which changes the working directory, relative paths don't work right.
# Need to create the directory first to make realpath happy

ifneq ($(MAKECMDGOALS),help)
$(shell mkdir -p $(O))
override O := $(realpath $(O))
endif

export ARCH QEMU_EXTRA_FLAGS PROJECT_BASE

override CONF_FILE := $(strip $(subst $(DQUOTE),,$(CONF_FILE)))

SOURCE_DIR ?= $(PROJECT_BASE)/src/
override SOURCE_DIR := $(realpath $(SOURCE_DIR))
override SOURCE_DIR := $(subst \,/,$(SOURCE_DIR))
override SOURCE_DIR_PARENT := $(patsubst %, %/.., $(SOURCE_DIR))
override SOURCE_DIR_PARENT := $(abspath $(SOURCE_DIR_PARENT))
override SOURCE_DIR_PARENT := $(subst \,/,$(SOURCE_DIR_PARENT))
export SOURCE_DIR SOURCE_DIR_PARENT

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

export CFLAGS

zephyrmake	= +$(MAKE) -C $(ZEPHYR_BASE)  O=$(1) \
			PROJECT=$(PROJECT_BASE) SOURCE_DIR=$(DQUOTE)$(SOURCE_DIR)$(DQUOTE) $(2)

BOARDCONFIG = $(O)/.board_$(BOARD)

DOTCONFIG = $(O)/.config

all: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

debug:	$(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

flash: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

qemugdb: debugserver

qemu: $(DOTCONFIG)
	@echo This target is deprecated, use 'make run' instead
	$(Q)$(call zephyrmake,$(O),$@)

run: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

ifeq ($(MAKECMDGOALS),debugserver)
ARCH = $(notdir $(subst /$(BOARD),,$(wildcard $(ZEPHYR_BASE)/boards/*/$(BOARD))))
-include $(ZEPHYR_BASE)/boards/$(ARCH)/$(BOARD)/Makefile.board
-include $(ZEPHYR_BASE)/scripts/Makefile.toolchain.$(ZEPHYR_GCC_VARIANT)
BOARD_NAME = $(BOARD)
export BOARD_NAME
endif

debugserver: FORCE
	$(Q)$(CONFIG_SHELL) $(ZEPHYR_BASE)/scripts/support/$(DEBUG_SCRIPT) debugserver

initconfig: $(DOTCONFIG)

$(BOARDCONFIG):
	@rm -f $(O)/.board_*
	@touch $@

ram_report: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

rom_report: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

outputexports: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

dts: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

config-sanitycheck: dts
	$(Q)$(call zephyrmake,$(O),$@)

menuconfig: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

help:
	$(Q)$(MAKE) -s -C $(ZEPHYR_BASE) $@

# Catch all
%:
	$(Q)$(call zephyrmake,$(O),$@)

OVERLAY_CONFIG += $(ZEPHYR_BASE)/kernel/configs/kernel.config

$(DOTCONFIG): $(BOARDCONFIG) $(KBUILD_DEFCONFIG_PATH) $(CONF_FILE)
	$(Q)$(CONFIG_SHELL) $(ZEPHYR_BASE)/scripts/kconfig/merge_config.sh \
		-q -m -O $(O) $(KBUILD_DEFCONFIG_PATH) $(OVERLAY_CONFIG) $(CONF_FILE) \
			      $(wildcard $(O)/*.conf)
	$(Q)$(MAKE) $(S) -C $(ZEPHYR_BASE) O=$(O) PROJECT=$(PROJECT_BASE) oldnoconfig

pristine:
	$(Q)rm -rf $(PRISTINE_O)

PHONY += FORCE initconfig
FORCE:

.PHONY: $(PHONY)
