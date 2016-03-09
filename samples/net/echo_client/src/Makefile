ccflags-y +=-I${srctree}/net/ip/contiki
ccflags-y +=-I${srctree}/net/ip/contiki/os/lib
ccflags-y +=-I${srctree}/net/ip/contiki/os
ccflags-y +=-I${srctree}/net/ip
ccflags-y +=-I${srctree}/samples/bluetooth/

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${srctree}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=0
endif

obj-y = echo-client.o

ifeq ($(CONFIG_NETWORKING_WITH_BT), y)
	obj-y += ../../../bluetooth/ipsp/src/ipss.o
endif
