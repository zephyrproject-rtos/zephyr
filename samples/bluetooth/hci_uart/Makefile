BOARD ?= nrf52_pca10040

ifeq ($(BOARD),bbc_microbit)
CONF_FILE ?= microbit.conf
else
CONF_FILE ?= nrf5.conf
endif

include $(ZEPHYR_BASE)/Makefile.inc
