ccflags-y +=-I${ZEPHYR_BASE}/net/ip
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os/lib
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os/sys
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/tinydtls
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/er-coap
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/rest-engine
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth/

obj-y = coap-observe-client.o

ifeq ($(CONFIG_NETWORKING_WITH_BT), y)
	obj-y += ../../../bluetooth/gatt/ipss.o
endif

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=0
endif
