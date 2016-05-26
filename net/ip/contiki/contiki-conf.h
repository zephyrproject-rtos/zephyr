/* contiki-conf.h - These settings override the default configuration */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <sys_clock.h>

#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__

#define PACK_ALIAS_STRUCT __attribute__((__packed__,__may_alias__))

#define CCIF
#define CLIF

typedef uint32_t clock_time_t;
typedef unsigned int uip_stats_t;

#define CLOCK_CONF_SECOND sys_clock_ticks_per_sec

/* It is either IPv6 or IPv4 but not both at the same time. */
#ifdef CONFIG_NETWORKING_WITH_IPV6
#define NETSTACK_CONF_WITH_IPV6 1
#define UIP_CONF_ICMP6 1
#elif CONFIG_NETWORKING_WITH_IPV4
#define NETSTACK_CONF_WITH_IPV6 0
#else
#error "Either IPv6 or IPv4 needs to be supported."
#endif

/* The actual MTU size is defined in uipopt.h */
#define UIP_CONF_BUFFER_SIZE UIP_LINK_MTU

#ifdef CONFIG_NETWORKING_WITH_TCP
#define UIP_CONF_TCP 1

#ifdef CONFIG_TCP_DISABLE_ACTIVE_OPEN
#define UIP_CONF_ACTIVE_OPEN 0
#else /* CONFIG_TCP_DISABLE_ACTIVE_OPEN */
#define UIP_CONF_ACTIVE_OPEN 1
#endif /* CONFIG_TCP_DISABLE_ACTIVE_OPEN */

#if CONFIG_TCP_MSS > 0
#define UIP_CONF_TCP_MSS CONFIG_TCP_MSS
#endif /* CONFIG_TCP_MSS */

#if CONFIG_TCP_RECEIVE_WINDOW > 0
#define UIP_CONF_RECEIVE_WINDOW CONFIG_TCP_RECEIVE_WINDOW
#endif /* CONFIG_TCP_RECEIVE_WINDOW */

#else
#define UIP_CONF_TCP 0
#endif

/* We do not want to be a router */
#define UIP_CONF_ROUTER 0

/* No Rime */
#define NETSTACK_CONF_WITH_RIME 0

/* How many IPv6 addresses will be allocated for the user (default is 2).
 * Increased this setting so that user can specify multicast addresses.
 */
#define UIP_CONF_DS6_ADDR_NBU 4

/* The queuebuf count defines how many fragments we are able to
 * receive. Value 13 means that we can receive full IPv6 data
 * (1280 bytes), we need also some extra buffers for temp use.
 */
#define QUEUEBUF_CONF_NUM (13 + 5)

/* Do not just drop packets because of some other packet is sent.
 * So queue the packet and send it later.
 */
#define UIP_CONF_IPV6_QUEUE_PKT 1

#ifdef SICSLOWPAN_CONF_ENABLE
/* Min and Max compressible UDP ports */
#define SICSLOWPAN_UDP_PORT_MIN                     0xF0B0
#define SICSLOWPAN_UDP_PORT_MAX                     0xF0BF   /* F0B0 + 15 */
#define NETSTACK_CONF_COMPRESS sicslowpan_compression
#ifdef CONFIG_NETWORKING_WITH_BT
#define NETSTACK_CONF_FRAGMENT null_fragmentation
#else
#define NETSTACK_CONF_FRAGMENT sicslowpan_fragmentation
#endif
#else
#define NETSTACK_CONF_COMPRESS null_compression
#define NETSTACK_CONF_FRAGMENT null_fragmentation
#endif /* SICSLOWPAN_CONF_ENABLE */

#ifdef CONFIG_NETWORKING_WITH_15_4
#ifdef CONFIG_NETWORKING_WITH_15_4_PAN_ID
#define IEEE802154_CONF_PANID CONFIG_NETWORKING_WITH_15_4_PAN_ID
#endif /* CONFIG_NETWORKING_WITH_15_4_PAN_ID */
#define NETSTACK_CONF_FRAMER	framer_802154
#ifdef CONFIG_NETWORKING_WITH_6LOWPAN
#if defined(CONFIG_NETWORKING_WITH_15_4_RDC_SICSLOWMAC)
#define NETSTACK_CONF_RDC	sicslowmac_driver
#elif defined(CONFIG_NETWORKING_WITH_15_4_RDC_SIMPLE)
#define NETSTACK_CONF_RDC	simplerdc_driver

/* Simple RDC config*/
#ifdef CONFIG_TI_CC2520_AUTO_ACK
#define SIMPLERDC_802154_AUTOACK	1
#else
#define SIMPLERDC_802154_SEND_ACK	1
#endif

#ifdef CONFIG_NETWORKING_WITH_15_4_ALWAYS_ACK
#define SIMPLERDC_802154_ACK_REQ	1
#endif
#define SIMPLERDC_MAX_RETRANSMISSIONS	3

#endif /* RDC driver */
#endif /* CONFIG_NETWORKING_WITH_6LOWPAN */
#ifdef CONFIG_NETWORKING_WITH_15_4_MAC_NULL
#define NETSTACK_CONF_MAC	nullmac_driver
#endif
#ifdef CONFIG_NETWORKING_WITH_15_4_MAC_CSMA
#define NETSTACK_CONF_MAC	csma_driver
#endif
#define LINKADDR_CONF_SIZE      8
#define UIP_CONF_LL_802154	1
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 1
#ifdef CONFIG_6LOWPAN_COMPRESSION_IPHC
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_IPHC
#else /* 6lowpan compression method */
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_IPV6
#endif /* 6lowpan compression method */
#ifdef CONFIG_15_4_BEACON_SUPPORT
#define FRAMER_802154_HANDLER handler_802154_frame_received
#endif /* CONFIG_15_4_BEACON_SUPPORT */
#ifdef CONFIG_15_4_BEACON_STATS
#define HANDLER_802154_CONF_STATS 1
#endif /* CONFIG_15_4_BEACON_STATS */
#else /* CONFIG_NETWORKING_WITH_15_4 */
#define NETSTACK_CONF_FRAMER	framer_nullmac
#define NETSTACK_CONF_RDC	simplerdc_driver
#define NETSTACK_CONF_MAC	nullmac_driver
#define LINKADDR_CONF_SIZE      6
#ifdef CONFIG_NETWORKING_WITH_BT
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_IPHC
#endif /* CONFIG_NETWORKING_WITH_BT */
#endif /* CONFIG_NETWORKING_WITH_15_4 */

#define NETSTACK_CONF_LLSEC	nullsec_driver

#ifdef CONFIG_NETWORKING_WITH_15_4_TI_CC2520
#define NETSTACK_CONF_RADIO cc2520_15_4_radio_driver
#endif

#ifdef CONFIG_NETWORKING_WITH_RPL
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_SMRF
#define UIP_CONF_IPV6_MULTICAST 1
#ifdef CONFIG_RPL_WITH_MRHOF
#define RPL_CONF_OF rpl_mrhof
#else
#define RPL_CONF_OF rpl_of0
#endif /* CONFIG_RPL_WITH_MRHOF */
#ifdef CONFIG_RPL_PROBING
#define RPL_CONF_WITH_PROBING 1
#else
#define RPL_CONF_WITH_PROBING 0
#endif /* CONFIG_RPL_PROBING */
#ifdef CONFIG_RPL_STATS
#define RPL_CONF_STATS 1
#else
#define RPL_CONF_STATS 0
#endif /* CONFIG_RPL_STATS */
#else /* CONFIG_NETWORKING_WITH_RPL */
#define RPL_CONF_STATS 0
#endif

#if defined(CONFIG_NETWORKING_STATISTICS) && defined(CONFIG_L2_BUFFERS)
#define NET_MAC_CONF_STATS 1
#else
#define NET_MAC_CONF_STATS 0
#endif

#if defined(CONFIG_COAP_STATS)
#define NET_COAP_CONF_STATS 1
#define NET_COAP_STAT(code) (net_coap_stats.code)
#else
#define NET_COAP_CONF_STATS 0
#define NET_COAP_STAT(code)
#endif

#ifdef CONFIG_NETWORKING_IPV6_NO_ND
/* Disabling ND will simplify the IPv6 address assignment.
 * This should only be done in testing phase.
 */
#define UIP_CONF_ND6_SEND_NA 0
#else
#define UIP_CONF_ND6_SEND_NA 1
#endif

#ifndef NETSTACK_CONF_RADIO
/* #error "No radio configured, cannot continue!" */
#endif

#ifdef CONFIG_ER_COAP
#ifndef REST
#define REST REGISTERED_ENGINE_ERBIUM
#endif
#endif

#ifdef CONFIG_ER_COAP_WITH_DTLS
#define ER_COAP_WITH_DTLS 1
#else
#define ER_COAP_WITH_DTLS 0
#endif

#ifdef CONFIG_ER_COAP_CLIENT
#define COAP_OBSERVE_CLIENT 1
#else
#undef COAP_OBSERVE_CLIENT
#endif

#ifdef CONFIG_NETWORKING_STATISTICS
#define UIP_CONF_STATISTICS 1
#endif

#ifdef CONFIG_ETHERNET
#define UIP_CONF_LLH_LEN 14
#endif

#if defined(CONFIG_UDP_MAX_CONNECTIONS)
#define UIP_CONF_UDP_CONNS CONFIG_UDP_MAX_CONNECTIONS
#endif

#if defined(CONFIG_TCP_MAX_CONNECTIONS)
#define UIP_CONF_MAX_CONNECTIONS CONFIG_TCP_MAX_CONNECTIONS
#endif

#if defined(CONFIG_NETWORKING_MAX_NEIGHBORS)
#define NBR_TABLE_CONF_MAX_NEIGHBORS CONFIG_NETWORKING_MAX_NEIGHBORS
#endif

#endif /* __CONTIKI_CONF_H__ */
