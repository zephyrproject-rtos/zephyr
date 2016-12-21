BOARD ?= qemu_x86

ifneq ($(BOARD), unit_testing)
	CONF_FILE ?= prj.conf

	include $(ZEPHYR_BASE)/Makefile.test
else
	OBJECTS = src/main.o
	include $(ZEPHYR_BASE)/tests/unit/Makefile.unittest
endif
