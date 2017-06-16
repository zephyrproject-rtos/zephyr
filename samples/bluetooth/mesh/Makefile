BOARD ?= qemu_x86

ifeq ($(BOARD),bbc_microbit)
CONF_FILE ?= microbit.conf
else ifeq ($(BOARD),nrf51_blenano)
CONF_FILE ?= nrf51-16k.conf
else
CONF_FILE ?= prj.conf
endif

QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr -s

include $(ZEPHYR_BASE)/Makefile.inc
