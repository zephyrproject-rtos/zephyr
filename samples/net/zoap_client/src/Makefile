ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=1
endif

obj-y = zoap-client.o

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth/
obj-y += ../../../bluetooth/gatt/ipss.o
endif
