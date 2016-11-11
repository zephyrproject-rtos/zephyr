ccflags-$(CONFIG_NETWORKING_WITH_BT) +=-I${ZEPHYR_BASE}/samples/bluetooth/

obj-y = echo-server.o

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=1
endif

ifeq ($(CONFIG_NETWORKING_WITH_BT), y)
	obj-y += ../../../bluetooth/gatt/ipss.o
endif
