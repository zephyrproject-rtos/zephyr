BOARD ?= qemu_x86

ifneq ($(BOARD), unit_testing)
	KERNEL_TYPE ?= nano
	CONF_FILE ?= prj.conf

	include $(ZEPHYR_BASE)/Makefile.inc
else
	OBJECTS = src/main.o
	include $(ZEPHYR_BASE)/tests/unit/Makefile.unittest
endif
