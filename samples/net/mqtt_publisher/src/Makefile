#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

obj-y += main.o

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth/
obj-y += ../../../bluetooth/gatt/ipss.o
endif
