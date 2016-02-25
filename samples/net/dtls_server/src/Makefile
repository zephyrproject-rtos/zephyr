ccflags-y +=-I${srctree}/net/ip/contiki
ccflags-y +=-I${srctree}/net/ip/contiki/os/lib
ccflags-y +=-I${srctree}/net/ip/contiki/os
ccflags-y +=-I${srctree}/net/ip/contiki/os/sys
ccflags-y +=-I${srctree}/net/ip
ccflags-y +=-I${srctree}/samples/bluetooth/

obj-y = dtls-server.o

ifeq ($(CONFIG_NETWORKING_WITH_BT), y)
	obj-y += ../../../bluetooth/ipsp/src/ipss.o
endif
