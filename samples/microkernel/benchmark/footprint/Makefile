# default configuration
TEST ?= min
BSP ?= generic_pc

VPFILE = micro_${TEST}.vpf
KERNEL_TYPE = micro
CONF_FILE = prj_$(TEST)_$(ARCH).conf
CFLAGS += -DTEST_${TEST}

include ${TIMO_BASE}/Makefile.inc

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
