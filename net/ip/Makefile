UIP_MODULES = net/ip/contiki/ip \
              net/ip/contiki/ipv6 \
              net/ip/contiki/mac \
              net/ip/contiki/mac/sicslowmac \
              net/ip/contiki/llsec \
              net/ip/contiki/os/dev \
              net/ip/contiki/os/sys \
              net/ip/contiki/os/lib

EXTRA_CFLAGS += ${PROJECTINCLUDE}
EXTRA_CFLAGS += -I${srctree}/net/ip/contiki
EXTRA_CFLAGS += -I${srctree}/net/ip/contiki/os/lib
EXTRA_CFLAGS += -I${srctree}/net/ip/contiki/os
EXTRA_CFLAGS += -I${srctree}/net/ip
EXTRA_CFLAGS += -DNETSTACK_CONF_WITH_IPV6=1

ifeq ($(CONFIG_NETWORKING_WITH_RPL),y)
	EXTRA_CFLAGS += -DUIP_CONF_IPV6_RPL=1
	UIP_MODULES += ${srctree}/net/ip/contiki/rpl
else
	EXTRA_CFLAGS += -DUIP_CONF_IPV6_RPL=0
endif

MODULES_SOURCES = ${foreach d, $(UIP_MODULES), ${wildcard $(ZEPHYR_BASE)/$(d)/*.c}}
MODULES_FILES = ${foreach f, $(MODULES_SOURCES), ${subst $(ZEPHYR_BASE)/net/ip/,,${f}}}
obj-y = ${foreach f, $(MODULES_FILES), ${subst .c,.o,${f}}}

obj-y += net_init.o \
	 net_buf.o \
	 net_context.o \
	 contiki/netstack.o \
	 contiki/packetbuf.o \
	 contiki/nbr-table.o \
	 contiki/linkaddr.o \
	 contiki/queuebuf.o

ifeq ($(CONFIG_NETWORKING_WITH_6LOWPAN),y)
     EXTRA_CFLAGS += -DSICSLOWPAN_CONF_ENABLE
     obj-y += contiki/ipv6/sicslowpan.o
else
     obj-y += contiki/uip-driver.o
endif

ifeq ($(CONFIG_NETWORKING_WITH_LOGGING),y)
     EXTRA_CFLAGS += -DUIP_CONF_LOGGING=1
     obj-y += contiki/uip-log.o
endif

obj-$(CONFIG_NETWORKING_WITH_LOOPBACK) += net_driver_loopback.o

# workaround for include file location
CREATE_INCLUDE_LINK_net := $(shell ln -s ${ZEPHYR_BASE}/net/ip/contiki ${ZEPHYR_BASE}/net/ip/contiki/net > /dev/null 2>&1)
