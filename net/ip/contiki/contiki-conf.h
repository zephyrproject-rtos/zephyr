/* contiki-conf.h - These settings override the default configuration */

#include <stdint.h>

#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__

#define PACK_ALIAS_STRUCT __attribute__((__packed__,__may_alias__))

#define CCIF
#define CLIF

typedef uint32_t clock_time_t;
typedef unsigned int uip_stats_t;

#endif /* __CONTIKI_CONF_H__ */
