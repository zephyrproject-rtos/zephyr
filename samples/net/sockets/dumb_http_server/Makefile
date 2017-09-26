# Makefile - dumb socket-based http server

#
# Copyright (c) 2017 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD ?= qemu_x86
CONF_FILE ?= prj.conf

include $(ZEPHYR_BASE)/Makefile.inc

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
	QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr
else
	include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack
endif

# Support for bin to header conversion
SRC = $(ZEPHYR_BASE)/samples/net/sockets/dumb_http_server/src
include $(ZEPHYR_BASE)/scripts/Makefile.gen
