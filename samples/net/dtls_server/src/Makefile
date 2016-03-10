ccflags-y +=-I${srctree}/net/ip/contiki
ccflags-y +=-I${srctree}/net/ip/contiki/os/lib
ccflags-y +=-I${srctree}/net/ip/contiki/os
ccflags-y +=-I${srctree}/net/ip/contiki/os/sys
ccflags-y +=-I${srctree}/net/ip
ccflags-y +=-I${srctree}/samples/bluetooth/

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${srctree}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=1
endif

obj-y = dtls-server.o

ifeq ($(CONFIG_NETWORKING_WITH_BT), y)
	obj-y += ../../../bluetooth/gatt/ipss.o
endif
