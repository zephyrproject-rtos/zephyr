#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD ?= qemu_x86
CONF_FILE ?= prj.conf

include $(ZEPHYR_BASE)/Makefile.test

ifeq ($(BOARD), qemu_x86)
	include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack
endif
