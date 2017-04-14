#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

obj-y += main.o
obj-y += http_utils.o
obj-y += http_server.o
obj-y += http_write_utils.o
ifdef CONFIG_MBEDTLS
obj-y += https_server.o ssl_utils.o
endif

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth/
obj-y += ../../../bluetooth/gatt/ipss.o
endif
