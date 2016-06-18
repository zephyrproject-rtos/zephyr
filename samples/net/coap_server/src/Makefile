ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os/lib
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os/sys
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/tinydtls
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/er-coap
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/rest-engine
ccflags-y +=-I${ZEPHYR_BASE}/net/ip
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth/

obj-y = coap-server.o

ifeq ($(CONFIG_NETWORKING_WITH_BT), y)
	obj-y += ../../../bluetooth/gatt/ipss.o
endif

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=1
endif

obj-y += \
	resources/res-plugtest-create1.o \
	resources/res-plugtest-create2.o \
	resources/res-plugtest-create3.o \
	resources/res-plugtest-large.o \
	resources/res-plugtest-large-create.o \
	resources/res-plugtest-large-update.o \
	resources/res-plugtest-links.o \
	resources/res-plugtest-locquery.o \
	resources/res-plugtest-longpath.o \
	resources/res-plugtest-multi.o \
	resources/res-plugtest-obs.o \
	resources/res-plugtest-path.o \
	resources/res-plugtest-query.o \
	resources/res-plugtest-separate.o \
	resources/res-plugtest-test.o \
	resources/res-plugtest-validate.o
