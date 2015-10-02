# default configuration
TEST ?= min

KERNEL_TYPE = nano
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = $(or $(wildcard prj_$(TEST)_$(ARCH)_$(PLATFORM_CONFIG).conf), \
                 $(wildcard prj_$(TEST)_$(ARCH).conf))
CFLAGS += -DTEST_$(TEST)

include $(ZEPHYR_BASE)/Makefile.inc

ifeq ($(ARCH),ARM)
  ifneq ($(TEST),min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
