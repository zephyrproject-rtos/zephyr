# default configuration
TEST ?= min
BOARD ?= qemu_x86


ARCH = $(notdir $(subst /$(BOARD),,$(wildcard $(ZEPHYR_BASE)/boards/*/$(BOARD))))

MDEF_FILE = micro_${TEST}.mdef
CONF_FILE = $(or $(wildcard $(TEST)/$(BOARD).conf), \
                 $(wildcard $(TEST)/$(ARCH).conf))
ifeq ($(TEST),float)
CFLAGS += -DTEST_max
else
CFLAGS += -DTEST_$(TEST)
endif

include ${ZEPHYR_BASE}/Makefile.test

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
