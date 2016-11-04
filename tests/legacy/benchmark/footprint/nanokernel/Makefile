# default configuration
TEST ?= min

BOARD ?= qemu_x86
CONF_FILE = $(or $(wildcard $(TEST)/$(BOARD).conf), \
                 $(wildcard $(TEST)/$(ARCH).conf))
ifeq ($(TEST),float)
CFLAGS += -DTEST_max
else
CFLAGS += -DTEST_$(TEST)
endif

include $(ZEPHYR_BASE)/Makefile.inc

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
