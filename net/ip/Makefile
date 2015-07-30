EXTRA_CFLAGS += ${PROJECTINCLUDE}
EXTRA_CFLAGS += -I${srctree}/net/ip/contiki
EXTRA_CFLAGS += -I${srctree}/net/ip/contiki/os/lib
EXTRA_CFLAGS += -I${srctree}/net/ip/contiki/os
EXTRA_CFLAGS += -I${srctree}/net/ip
EXTRA_CFLAGS += -I${srctree}/include/drivers

ifneq ($(CONFIG_NET_BUF_SIZE),)
EXTRA_CFLAGS += -DNET_NUM_BUFS=$(CONFIG_NET_BUF_SIZE)
endif

# Zypher specific files
obj-y = net_init.o \
	 net_buf.o \
	 net_context.o

# Contiki IP stack files
obj-y += contiki/netstack.o \
	contiki/packetbuf.o \
	contiki/nbr-table.o \
	contiki/linkaddr.o \
	contiki/queuebuf.o \
	contiki/ip/uip-debug.o \
	contiki/ip/uip-packetqueue.o \
	contiki/ip/uip-udp-packet.o \
	contiki/ip/udp-socket.o \
	contiki/ip/simple-udp.o \
	contiki/ip/uiplib.o \
	contiki/ip/uip-nameserver.o \
	contiki/ip/tcpip.o \
	contiki/os/sys/process.o \
	contiki/os/sys/stimer.o \
	contiki/os/sys/etimer.o \
	contiki/os/sys/timer.o \
	contiki/os/sys/arg.o \
	contiki/os/sys/ctimer.o \
	contiki/os/sys/rtimer.o \
	contiki/os/sys/clock.o \
	contiki/os/dev/nullradio.o \
	contiki/os/lib/mmem.o \
	contiki/os/lib/memb.o \
	contiki/os/lib/list.o \
	contiki/os/lib/random.o \
	contiki/os/lib/aes-128.o \
	contiki/mac/mac.o \
	contiki/mac/framer.o \
	contiki/mac/mac-sequence.o \
	contiki/llsec/nullsec.o

obj-$(CONFIG_NETWORKING_WITH_IPV6) += \
	contiki/ipv6/uip6.o \
	contiki/ipv6/uip-icmp6.o \
	contiki/ipv6/uip-ds6.o \
	contiki/ipv6/uip-nd6.o \
	contiki/ipv6/uip-ds6-route.o \
	contiki/ipv6/uip-ds6-nbr.o

obj-$(CONFIG_NETWORKING_WITH_IPV4) += \
	contiki/ipv4/uip_arp.o \
	contiki/ipv4/uip.o \
	contiki/ipv4/uip-neighbor.o

# RPL (RFC 6550) support
ifeq ($(CONFIG_NETWORKING_WITH_RPL),y)
	EXTRA_CFLAGS += -DUIP_CONF_IPV6_RPL=1
	obj-y += contiki/rpl/rpl-of0.o \
		contiki/rpl/rpl-mrhof.o \
		contiki/rpl/rpl-dag.o \
		contiki/rpl/rpl-timers.o \
		contiki/rpl/rpl.o \
		contiki/rpl/rpl-ext-header.o \
		contiki/rpl/rpl-icmp6.o \
		contiki/ipv6/multicast/uip-mcast6-route.o \
		contiki/ipv6/multicast/smrf.o \
		contiki/ipv6/multicast/uip-mcast6-stats.o
else
	EXTRA_CFLAGS += -DUIP_CONF_IPV6_RPL=0
endif

# 6LoWPAN support
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
endif
obj-$(CONFIG_NETWORKING_WITH_LOGGING) += contiki/uip-log.o

obj-$(CONFIG_NETWORKING_WITH_LOOPBACK) += net_driver_loopback.o
obj-$(CONFIG_NETWORKING_WITH_15_4_LOOPBACK) += dummy_15_4_radio.o
obj-$(CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART) += dummy_15_4_radio.o
obj-$(CONFIG_NETWORKING_WITH_15_4) += net_driver_15_4.o \
					contiki/mac/frame802154.o \
					contiki/mac/framer-802154.o \
					contiki/mac/csma.o \
					contiki/mac/mac-sequence.o \
					contiki/mac/sicslowmac/sicslowmac.o

# At the moment we only need nullsec driver for 802.15.4
#obj-$(CONFIG_NETWORKING_WITH_15_4) += contiki/llsec/ccm-star.o \
#					contiki/llsec/ccm-star.o \
#					contiki/llsec/anti-replay.o

ifeq ($(CONFIG_NETWORKING_WITH_15_4),)
     obj-y += contiki/mac/nullmac.o \
		contiki/mac/framer-nullmac.o
endif

obj-$(CONFIG_NETWORKING_UART) += contiki/os/dev/slip.o \
				contiki/os/dev/slip-arch.o \
				net_driver_slip.o

# workaround for include file location
CREATE_INCLUDE_LINK_net := $(shell ln -s ${ZEPHYR_BASE}/net/ip/contiki ${ZEPHYR_BASE}/net/ip/contiki/net > /dev/null 2>&1)
