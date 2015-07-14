# default configuration
TEST ?= min

KERNEL_TYPE = nano
PLATFORM_CONFIG ?= generic_pc_atom_n28xx
CONF_FILE = prj_$(TEST)_$(ARCH).conf
CFLAGS += -DTEST_$(TEST)

include $(ZEPHYR_BASE)/Makefile.inc

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
