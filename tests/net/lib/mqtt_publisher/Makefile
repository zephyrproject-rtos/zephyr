BOARD ?= qemu_x86
CONF_FILE ?= prj_$(BOARD).conf

include $(ZEPHYR_BASE)/Makefile.test

ifeq ($(BOARD), qemu_x86)
	include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack
endif
