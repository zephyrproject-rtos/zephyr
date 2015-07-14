# default configuration
TEST ?= min

KERNEL_TYPE = nano
KBUILD_DEFCONFIG ?= $(KERNEL_TYPE)_generic_pc_atom_n28xx_defconfig
CONF_FILE = prj_$(TEST)_$(ARCH).conf
CFLAGS += -DTEST_$(TEST)

include $(ZEPHYR_BASE)/Makefile.inc

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
