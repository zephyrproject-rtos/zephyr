# default configuration
TEST ?= min
BOARD ?= qemu_x86


ARCH = $(notdir $(subst /$(BOARD),,$(wildcard $(ZEPHYR_BASE)/boards/*/$(BOARD))))

CONF_FILE = $(or $(wildcard $(TEST)/$(BOARD).conf), \
                 $(wildcard $(TEST)/$(ARCH).conf))
ifeq ($(TEST),float)
CFLAGS += -DTEST_max
else
CFLAGS += -DTEST_$(TEST)
endif

include ${ZEPHYR_BASE}/Makefile.test
