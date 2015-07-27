/* contiki-conf.h - These settings override the default configuration */

#include <stdint.h>

#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__

#define PACK_ALIAS_STRUCT __attribute__((__packed__,__may_alias__))

#define CCIF
#define CLIF

typedef uint32_t clock_time_t;
typedef unsigned int uip_stats_t;

#define CLOCK_CONF_SECOND 100

/* It is either IPv6 or IPv4 but not both at the same time. */
#ifdef CONFIG_NETWORKING_WITH_IPV6
#define NETSTACK_CONF_WITH_IPV6 1
#elif CONFIG_NETWORKING_WITH_IPV4
#define NETSTACK_CONF_WITH_IPV6 0
#else
#error "Either IPv6 or IPv4 needs to be supported."
#endif

/* No TCP yet */
#define UIP_CONF_TCP 0

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

#ifdef SICSLOWPAN_CONF_ENABLE
/* Min and Max compressible UDP ports */
#define SICSLOWPAN_UDP_PORT_MIN                     0xF0B0
#define SICSLOWPAN_UDP_PORT_MAX                     0xF0BF   /* F0B0 + 15 */
#define NETSTACK_CONF_COMPRESS sicslowpan_compression
#define NETSTACK_CONF_FRAGMENT sicslowpan_fragmentation
#else
#define NETSTACK_CONF_COMPRESS null_compression
#define NETSTACK_CONF_FRAGMENT null_fragmentation
#endif /* SICSLOWPAN_CONF_ENABLE */

#ifdef CONFIG_NETWORKING_WITH_15_4
#define NETSTACK_CONF_FRAMER	framer_802154
#define NETSTACK_CONF_RDC	sicslowmac_driver
#define NETSTACK_CONF_MAC	csma_driver
#define LINKADDR_CONF_SIZE      8
#define UIP_CONF_LL_802154	1
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 1
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_IPV6
#else
#define NETSTACK_CONF_FRAMER	framer_nullmac
#define NETSTACK_CONF_RDC	nullrdc_driver
#define NETSTACK_CONF_MAC	nullmac_driver
#define LINKADDR_CONF_SIZE      6
#endif
#define NETSTACK_CONF_LLSEC	nullsec_driver

#ifdef CONFIG_NETWORKING_WITH_RPL
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_SMRF
#define UIP_CONF_IPV6_MULTICAST 1
#endif

/* This will disable DAD for now in order to simplify the
 * IPv6 address assignment. This needs to be re-enabled
 * at some point.
 */
#define UIP_CONF_ND6_SEND_NA 0

#ifndef NETSTACK_CONF_RADIO
/* #error "No radio configured, cannot continue!" */
#endif

#endif /* __CONTIKI_CONF_H__ */
