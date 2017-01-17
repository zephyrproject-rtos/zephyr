# default configuration
TEST ?= min

ARCH = $(notdir $(subst /$(BOARD),,$(wildcard $(ZEPHYR_BASE)/boards/*/$(BOARD))))

BOARD ?= qemu_x86
CONF_FILE ?= $(or $(wildcard $(TEST)/$(BOARD).conf), \
                 $(wildcard $(TEST)/$(ARCH).conf))
ifeq ($(CONF_FILE),)
CONF_FILE = $(TEST)/default.conf
endif

ifeq ($(TEST),float)
CFLAGS += -DTEST_max
else
CFLAGS += -DTEST_$(TEST)
endif

include $(ZEPHYR_BASE)/Makefile.test

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
