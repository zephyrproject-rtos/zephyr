# Makefile - ieee802154 example that is used in testing

#
# Copyright (c) 2016 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD ?= qemu_x86
CONF_FILE ?= prj.conf

include $(ZEPHYR_BASE)/Makefile.inc
include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack

ifeq ($(CONFIG_BOARD_FRDM_K64F),y)
	include $(ZEPHYR_BASE)/samples/net/common/Makefile.common
endif
