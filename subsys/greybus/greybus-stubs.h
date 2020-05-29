#ifndef __GREYBUS_STUBS_H__
#define __GREYBUS_STUBS_H__

#include <stdbool.h>

struct wdog_s;

#define WDOG_ISACTIVE(wd) true

void wd_cancel(struct wdog_s *wd);
void wd_delete(struct wdog_s *wd);
void wd_start(struct wdog_s *wd, unsigned long delay, void (*callback)(int, uint32_t, ...), int integer, u16_t cport);
void wd_static(struct wdog_s *wd);
void timesync_enable();
void timesync_disable();
void timesync_authoritative();
void timesync_get_last_event();
void gb_loopback_log_exit();
void pthread_attr_setstacksize();
void gb_loopback_log_entry();

#endif /* __GREYBUS_STUBS_H__ */
