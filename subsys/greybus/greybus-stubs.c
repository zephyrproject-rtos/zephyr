#include <errno.h>
#include <unipro/unipro.h>

#include "greybus-stubs.h"

void wd_cancel(struct wdog_s *wd) {
	(void)wd;
}
void wd_delete(struct wdog_s *wd) {
	(void)wd;
}
void wd_start(struct wdog_s *wd, unsigned long delay, void (*callback)(int, uint32_t, ...), int integer, u16_t cport) {
	(void)wd;
}
void wd_static(struct wdog_s *wd) {
	(void)wd;
}

void unipro_init(void) {
}
void unipro_init_with_event_handler(unipro_event_handler_t evt_handler) {
	(void)evt_handler;
}
void unipro_deinit(void) {
}
void unipro_set_event_handler(unipro_event_handler_t evt_handler) {
	(void)evt_handler;
}
void unipro_info(void) {
}
int unipro_send(unsigned int cportid, const void *buf, size_t len) {
	(void)cportid;
	(void)buf;
	(void)len;
	return -ENOSYS;
}
int unipro_send_async(unsigned int cportid, const void *buf, size_t len,
                      unipro_send_completion_t callback, void *priv) {
	(void)cportid;
	(void)buf;
	(void)len;
	(void)callback;
	(void)priv;
	return -ENOSYS;
}
int unipro_reset_cport(unsigned int cportid, cport_reset_completion_cb_t cb,
                       void *priv) {
	(void)cportid;
	(void)cb;
	(void)priv;
	return -ENOSYS;
}

int unipro_set_max_inflight_rxbuf_count(unsigned int cportid,
                                        size_t max_inflight_buf) {
	(void)cportid;
	(void)max_inflight_buf;
	return -ENOSYS;
}
void *unipro_rxbuf_alloc(unsigned int cportid) {
	(void)cportid;
	return NULL;
}
void unipro_rxbuf_free(unsigned int cportid, void *ptr) {
	(void)cportid;
	(void)ptr;
}
int unipro_attr_access(uint16_t attr,
                       uint32_t *val,
                       uint16_t selector,
                       int peer,
                       int write) {
	(void)attr;
	(void)val;
	(void)selector;
	(void)peer;
	(void)write;
	return -ENOSYS;
}

int unipro_disable_fct_tx_flow(unsigned int cport) {
	return -ENOSYS;
}
int unipro_enable_fct_tx_flow(unsigned int cport) {
	return -ENOSYS;
}

void timesync_enable() {
}
void timesync_disable() {
}
void timesync_authoritative() {
}
void timesync_get_last_event() {
}
void gb_loopback_log_exit() {
}
void pthread_attr_setstacksize() {
}
void gb_loopback_log_entry() {
}
