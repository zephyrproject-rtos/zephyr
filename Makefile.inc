# vim: filetype=make
#

ARCH ?= x86

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
KBUILD_DEFCONFIG_PATH=$(wildcard $(ZEPHYR_BASE)/boards/*/$(BOARD)_defconfig)
ifeq ($(KBUILD_DEFCONFIG_PATH),)
$(error Board $(BOARD) not found!)
endif
else
$(error BOARD is not defined!)
endif

SOURCE_DIR ?= $(PROJECT_BASE)/src/
override SOURCE_DIR := $(abspath $(SOURCE_DIR))
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

ifeq ($(findstring qemu_,$(BOARD)),)
qemu:
	@echo "Emulation not available for this platform"
qemugdb: qemu
else
qemu: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)
qemugdb: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)
endif

debug:	$(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

flash: $(DOTCONFIG)
	$(Q)$(call zephyrmake,$(O),$@)

ifeq ($(MAKECMDGOALS),debugserver)
-include $(ZEPHYR_BASE)/boards/$(BOARD)/Makefile.board
-include $(ZEPHYR_BASE)/scripts/Makefile.toolchain.$(ZEPHYR_GCC_VARIANT)
BOARD_NAME = $(BOARD)
export BOARD_NAME
endif
debugserver: FORCE
	$(Q)$(CONFIG_SHELL) $(ZEPHYR_BASE)/scripts/support/$(FLASH_SCRIPT) debugserver


initconfig: $(DOTCONFIG)

$(BOARDCONFIG):
	@rm -f $(O)/.board_*
	@touch $@


ram_report: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

rom_report: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

menuconfig: initconfig
	$(Q)$(call zephyrmake,$(O),$@)

help:
	$(Q)$(MAKE) -s -C $(ZEPHYR_BASE) $@

# Catch all
%:
	$(Q)$(call zephyrmake,$(O),$@)

KERNEL_CONFIG = $(ZEPHYR_BASE)/kernel/configs/$(KERNEL_TYPE).config

$(DOTCONFIG): $(BOARDCONFIG) $(KBUILD_DEFCONFIG_PATH) $(CONF_FILE)
	$(Q)$(CONFIG_SHELL) $(ZEPHYR_BASE)/scripts/kconfig/merge_config.sh \
		-q -m -O $(O) $(KBUILD_DEFCONFIG_PATH) $(KERNEL_CONFIG) $(CONF_FILE)
	$(Q)$(MAKE) $(S) -C $(ZEPHYR_BASE) O=$(O) PROJECT=$(PROJECT_BASE) oldnoconfig

pristine:
	$(Q)rm -rf $(O)

PHONY += FORCE initconfig
FORCE:

.PHONY: $(PHONY)
