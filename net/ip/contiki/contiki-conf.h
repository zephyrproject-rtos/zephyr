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

/* No TCP yet */
#define UIP_CONF_TCP 0

/* We do not want to be a router */
#define UIP_CONF_ROUTER 0

/* No Rime */
#define NETSTACK_CONF_WITH_RIME 0

#ifdef SICSLOWPAN_CONF_ENABLE
/* Min and Max compressible UDP ports */
#define SICSLOWPAN_UDP_PORT_MIN                     0xF0B0
#define SICSLOWPAN_UDP_PORT_MAX                     0xF0BF   /* F0B0 + 15 */
#else
#define NETSTACK_CONF_COMPRESS null_compression
#define NETSTACK_CONF_FRAGMENT null_fragmentation
#endif /* SICSLOWPAN_CONF_ENABLE */

#define NETSTACK_CONF_FRAMER	framer_802154
#define NETSTACK_CONF_LLSEC	nullsec_driver
#define NETSTACK_CONF_MAC	csma_driver
#define NETSTACK_CONF_RDC	sicslowmac_driver

#ifndef NETSTACK_CONF_RADIO
/* #error "No radio configured, cannot continue!" */
#endif

#endif /* __CONTIKI_CONF_H__ */
