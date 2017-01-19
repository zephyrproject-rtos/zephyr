# Makefile - Bluetooth shell

#
# Copyright (c) 2015-2016 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

# DESCRIPTION
# Makefile for the console shell with Bluetooth commands
BOARD ?= qemu_x86
QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr

ifeq ($(BOARD),arduino_101)
CONF_FILE = arduino_101.conf
else
CONF_FILE = prj.conf
endif

PRJ_INCPATH += ${ZEPHYR_BASE}/include/drivers

include $(ZEPHYR_BASE)/Makefile.test
# console handler
