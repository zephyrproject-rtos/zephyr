# default configuration
TEST ?= min
PLATFORM_CONFIG ?= generic_pc_atom_n28xx

MDEF_FILE = micro_${TEST}.mdef
KERNEL_TYPE = micro
CONF_FILE = prj_$(TEST)_$(ARCH).conf
CFLAGS += -DTEST_${TEST}

include ${ZEPHYR_BASE}/Makefile.inc

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
