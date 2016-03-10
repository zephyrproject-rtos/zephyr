ccflags-y +=-I${srctree}/net/ip/contiki
ccflags-y +=-I${srctree}/net/ip/contiki/os/lib
ccflags-y +=-I${srctree}/net/ip/contiki/os
ccflags-y +=-I${srctree}/net/ip/contiki/os/sys
ccflags-y +=-I${srctree}/net/ip

ifeq ($(CONFIG_NET_TESTING), y)
ccflags-y +=-I${srctree}/samples/net/common/
ccflags-y +=-DNET_TESTING_SERVER=0
endif

obj-y = dtls-client.o
