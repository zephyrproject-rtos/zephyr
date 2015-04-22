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

/* No reassembly yet */
#define SICSLOWPAN_CONF_FRAG 0

#define NETSTACK_CONF_NETWORK	uip_driver
#define NETSTACK_CONF_FRAMER	framer_802154
#define NETSTACK_CONF_LLSEC	nullsec_driver
#define NETSTACK_CONF_MAC	csma_driver
#define NETSTACK_CONF_RDC	sicslowmac_driver

#ifndef NETSTACK_CONF_RADIO
/* #error "No radio configured, cannot continue!" */
#endif

#endif /* __CONTIKI_CONF_H__ */
