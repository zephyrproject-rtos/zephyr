# Makefile - echo client test application

#
# Copyright (c) 2015 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD ?= qemu_x86
CONF_FILE ?= prj_$(BOARD).conf

include $(ZEPHYR_BASE)/Makefile.inc

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
	QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr
else
	include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack
endif
