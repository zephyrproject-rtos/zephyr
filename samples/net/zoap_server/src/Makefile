ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=1
endif

obj-y = zoap-server.o

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
ccflags-y += -I${ZEPHYR_BASE}/samples/bluetooth/
obj-y += ../../../bluetooth/gatt/ipss.o
endif

ifeq ($(CONFIG_NET_L2_IEEE802154), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ifeq ($(CONFIG_NET_APP_SETTINGS), y)
obj-y += ../../common/ieee802154_settings.o
endif
endif
