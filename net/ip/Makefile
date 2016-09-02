ccflags-y += -I${srctree}/net/ip/contiki
ccflags-y += -I${srctree}/net/ip/contiki/os/lib
ccflags-y += -I${srctree}/net/ip/contiki/os
ccflags-y += -I${srctree}/net/ip
ccflags-y += -I${srctree}/include/drivers

# Zephyr specific files
obj-y = net_core.o \
	ip_buf.o \
	net_context.o

obj-$(CONFIG_L2_BUFFERS) += l2_buf.o

# Contiki IP stack files
obj-y += contiki/netstack.o \
	contiki/nbr-table.o \
	contiki/linkaddr.o \
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

obj-$(CONFIG_NETWORKING_WITH_TCP) += contiki/ip/psock.o

# RPL (RFC 6550) support
ifeq ($(CONFIG_NETWORKING_WITH_RPL),y)
	ccflags-y += -DUIP_CONF_IPV6_RPL=1
	obj-y += contiki/rpl/rpl-dag.o \
		contiki/rpl/rpl-timers.o \
		contiki/rpl/rpl.o \
		contiki/rpl/rpl-ext-header.o \
		contiki/rpl/rpl-icmp6.o \
		contiki/ipv6/multicast/uip-mcast6-route.o \
		contiki/ipv6/multicast/smrf.o \
		contiki/ipv6/multicast/uip-mcast6-stats.o

	obj-$(CONFIG_RPL_WITH_OF0) += contiki/rpl/rpl-of0.o
	obj-$(CONFIG_RPL_WITH_MRHOF) += contiki/rpl/rpl-mrhof.o
else
	ccflags-y += -DUIP_CONF_IPV6_RPL=0
endif

# 6LoWPAN support
ifeq ($(CONFIG_NETWORKING_WITH_6LOWPAN),y)
     ccflags-y += -DSICSLOWPAN_CONF_ENABLE
     obj-y += contiki/sicslowpan/sicslowpan_compression.o
     obj-$(CONFIG_NETWORKING_WITH_15_4) += contiki/sicslowpan/sicslowpan_fragmentation.o
else
     obj-$(CONFIG_NETWORKING_WITH_15_4) += contiki/mac/framer-nullmac.o \
					contiki/mac/simplerdc.o \
					contiki/sicslowpan/null_compression.o \
					contiki/sicslowpan/null_fragmentation.o
endif

# RDC driver
ifeq ($(CONFIG_NETWORKING_WITH_15_4_RDC_SICSLOWMAC),y)
     obj-$(CONFIG_NETWORKING_WITH_15_4) += contiki/mac/sicslowmac/sicslowmac.o
else
     ifeq ($(CONFIG_NETWORKING_WITH_15_4_RDC_SIMPLE),y)
          obj-$(CONFIG_NETWORKING_WITH_15_4) += contiki/mac/simplerdc.o
     endif
endif

ccflags-$(CONFIG_NETWORKING_WITH_LOGGING) += -DUIP_CONF_LOGGING=1

obj-$(CONFIG_NETWORKING_WITH_LOGGING) += contiki/uip-log.o

obj-$(CONFIG_NETWORKING_WITH_LOOPBACK) += net_driver_loopback.o
obj-$(CONFIG_NETWORKING_WITH_15_4_LOOPBACK) += dummy_15_4_radio.o
obj-$(CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART) += dummy_15_4_radio.o
obj-$(CONFIG_NETWORKING_WITH_15_4) += net_driver_15_4.o \
					contiki/packetbuf.o \
					contiki/queuebuf.o \
					contiki/mac/mac.o \
					contiki/mac/framer.o \
					contiki/mac/frame802154.o \
					contiki/mac/framer-802154.o \
					contiki/mac/mac-sequence.o

obj-$(CONFIG_NETWORKING_WITH_15_4_MAC_NULL) += contiki/mac/nullmac.o
obj-$(CONFIG_NETWORKING_WITH_15_4_MAC_CSMA) += contiki/mac/csma.o

obj-$(CONFIG_15_4_BEACON_SUPPORT) += contiki/mac/handler-802154.o

obj-$(CONFIG_NETWORKING_WITH_BT) += net_driver_bt.o \
					contiki/packetbuf.o \
					contiki/queuebuf.o \
					contiki/mac/mac.o \
					contiki/mac/framer.o \
					contiki/mac/framer-nullmac.o \
					contiki/sicslowpan/null_fragmentation.o

# At the moment we only need nullsec driver for 802.15.4
#obj-$(CONFIG_NETWORKING_WITH_15_4) += contiki/llsec/ccm-star.o \
#					contiki/llsec/ccm-star.o \
#					contiki/llsec/anti-replay.o

ifeq ($(CONFIG_NETWORKING_WITH_15_4),y)
     obj-y += contiki/mac/nullmac.o \
		contiki/mac/framer-nullmac.o \
		contiki/mac/simplerdc.o
endif

obj-$(CONFIG_NETWORKING_UART) += contiki/os/dev/slip.o \
				contiki/os/dev/slip-arch.o \
				net_driver_slip.o

obj-$(CONFIG_ETHERNET) += net_driver_ethernet.o

ccflags-$(CONFIG_TINYDTLS) += -DCONTIKI_TARGET_ZEPHYR=1
ccflags-$(CONFIG_TINYDTLS) += -DWITH_SHA256=1
ccflags-$(CONFIG_TINYDTLS) += -DDTLS_TICKS_PER_SECOND=sys_clock_ticks_per_sec
ccflags-$(CONFIG_TINYDTLS) += -I${srctree}/net/ip/contiki/os/sys
ccflags-$(CONFIG_TINYDTLS) += -I${srctree}/net/ip/tinydtls

obj-$(CONFIG_TINYDTLS) += tinydtls/dtls.o \
			tinydtls/crypto.o \
			tinydtls/hmac.o \
			tinydtls/aes/rijndael.o \
			tinydtls/sha2/sha2.o \
			tinydtls/ccm.o \
			tinydtls/netq.o \
			tinydtls/dtls_time.o \
			tinydtls/peer.o \
			tinydtls/session.o \
			tinydtls/ecc/ecc.o


ifeq ($(CONFIG_TINYDTLS_DEBUG),)
	ccflags-y += -DNDEBUG
endif
obj-$(CONFIG_TINYDTLS_DEBUG) += tinydtls/debug.o

ccflags-$(CONFIG_ER_COAP) += -I${srctree}/net/ip/rest-engine
ccflags-$(CONFIG_ER_COAP) += -I${srctree}/net/ip/er-coap

obj-$(CONFIG_ER_COAP) += er-coap/er-coap.o \
			er-coap/er-coap-engine.o \
			er-coap/er-coap-transactions.o \
			er-coap/er-coap-observe.o \
			er-coap/er-coap-separate.o \
			er-coap/er-coap-res-well-known-core.o \
			er-coap/er-coap-block1.o \
			er-coap/er-coap-context.o \
			rest-engine/rest-engine.o

obj-$(CONFIG_ER_COAP_CLIENT) += er-coap/er-coap-observe-client.o

obj-$(CONFIG_DHCP) += contiki/ip/dhcpc.o

obj-$(CONFIG_NETWORKING_WITH_TRICKLE) += \
	contiki/trickle/trickle-timer.o
