# Makefile - Bluetooth tester

#
# Copyright (c) 2015-2016 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

# DESCRIPTION
# Makefile for the Bluetooth tester application
BOARD ?= arduino_101

CONF_FILE ?= default.conf

# UART for Bluetooth
QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr

# UART for Tester
QEMU_EXTRA_FLAGS += -serial pipe:/tmp/bt-stack-tester

PRJ_INCPATH += ${ZEPHYR_BASE}/include/drivers

include $(ZEPHYR_BASE)/Makefile.test
