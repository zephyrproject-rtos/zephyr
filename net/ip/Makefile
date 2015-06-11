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
EXTRA_CFLAGS += -I${srctree}/include/drivers
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
     obj-y += contiki/sicslowpan/sicslowpan_compression.o
     obj-y += contiki/sicslowpan/sicslowpan_fragmentation.o
else
     obj-y += contiki/sicslowpan/null_compression.o
     obj-y += contiki/sicslowpan/null_fragmentation.o
endif

ifeq ($(CONFIG_NETWORKING_WITH_LOGGING),y)
     EXTRA_CFLAGS += -DUIP_CONF_LOGGING=1
     obj-y += contiki/uip-log.o
endif

obj-$(CONFIG_NETWORKING_WITH_15_4) += net_driver_15_4.o
obj-$(CONFIG_NETWORKING_WITH_15_4_LOOPBACK) += dummy_15_4_radio.o
obj-$(CONFIG_NETWORKING_WITH_LOOPBACK) += net_driver_loopback.o

ifneq ($(CONFIG_NETWORKING_WITH_15_4),y)
     # remove files that are not needed
     obj-y := $(filter-out contiki/mac/frame802154.o \
		contiki/mac/framer-802154.o \
		contiki/mac/csma.o \
		contiki/mac/mac-sequence.o \
		contiki/mac/sicslowmac/sicslowmac.o,$(obj-y))
else
     obj-y := $(filter-out contiki/mac/nullmac.o \
		contiki/mac/framer-nullmac.o ,$(obj-y))
endif

ifneq ($(CONFIG_NETWORKING_UART),y)
     obj-y := $(filter-out contiki/os/dev/slip.o \
		contiki/os/dev/slip-arch.o,$(obj-y))
endif

# At the moment we only need nullsec driver
obj-y := $(filter-out contiki/llsec/ccm-star.o \
		contiki/llsec/ccm-star.o \
		contiki/llsec/anti-replay.o,$(obj-y))

# workaround for include file location
CREATE_INCLUDE_LINK_net := $(shell ln -s ${ZEPHYR_BASE}/net/ip/contiki ${ZEPHYR_BASE}/net/ip/contiki/net > /dev/null 2>&1)
