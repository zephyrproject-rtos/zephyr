#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

ccflags-$(CONFIG_NET_L2_BLUETOOTH) += -I${ZEPHYR_BASE}/samples/bluetooth/

obj-$(CONFIG_NET_APP) += init.o
obj-$(CONFIG_NET_APP_SERVER) += server.o
obj-$(CONFIG_NET_APP_CLIENT) += client.o

ifeq ($(CONFIG_NET_APP_SERVER),y)
	obj-y += net_app.o
else
	ifeq ($(CONFIG_NET_APP_CLIENT),y)
		obj-y += net_app.o
	endif
endif

ifeq ($(CONFIG_NET_APP_SETTINGS),y)
	obj-$(CONFIG_NET_L2_IEEE802154) += ieee802154_settings.o
endif
