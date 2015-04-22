/* FIXME - dummy file to be replaced/removed at some point */

#ifndef __RTIMER_ARCH_H__
#define __RTIMER_ARCH_H__

#include "contiki-conf.h"
#include "sys/clock.h"

#define RTIMER_ARCH_SECOND CLOCK_CONF_SECOND

// FIXME - implement these

static inline rtimer_clock_t rtimer_arch_now(void) { return 0; }
int rtimer_arch_check(void);
int rtimer_arch_pending(void);
rtimer_clock_t rtimer_arch_next(void);
static inline void rtimer_arch_init(void) {}

#endif /* __RTIMER_ARCH_H__ */
