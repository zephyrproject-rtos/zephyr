ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os/lib
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os
ccflags-y +=-I${ZEPHYR_BASE}/net/ip/contiki/os/sys
ccflags-y +=-I${ZEPHYR_BASE}/net/ip

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=0
endif

obj-y = dtls-client.o
