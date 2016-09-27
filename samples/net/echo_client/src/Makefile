ccflags-$(CONFIG_NET_UIP) +=-I${ZEPHYR_BASE}/net/ip
ccflags-$(CONFIG_NETWORKING_WITH_BT) +=-I${ZEPHYR_BASE}/samples/bluetooth/

ifeq ($(CONFIG_NET_YAIP), y)
obj-y = echo-client-yaip.o
else
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os/lib
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os
obj-y = echo-client.o
endif

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=0
endif

ifeq ($(CONFIG_NETWORKING_WITH_BT), y)
	obj-y += ../../../bluetooth/gatt/ipss.o
endif
