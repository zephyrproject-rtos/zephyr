BOARD ?= qemu_x86

ifneq ($(BOARD), unit_testing)
	CONF_FILE ?= prj_verbose_0.conf

	include $(ZEPHYR_BASE)/Makefile.inc
else
	OBJECTS = src/main.o
	include $(ZEPHYR_BASE)/tests/unit/Makefile.unittest
endif
