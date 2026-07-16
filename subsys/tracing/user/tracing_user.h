/*
 * Copyright (c) 2020 Lexmark International, Inc.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_USER_H
#define _TRACE_USER_H
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#ifdef __cplusplus
extern "C" {
#endif

void sys_trace_thread_create_user(struct k_thread *thread);
void sys_trace_thread_abort_user(struct k_thread *thread);
void sys_trace_thread_suspend_user(struct k_thread *thread);
void sys_trace_thread_resume_user(struct k_thread *thread);
void sys_trace_thread_name_set_user(struct k_thread *thread);
void sys_trace_thread_switched_in_user(void);
void sys_trace_thread_switched_out_user(void);
void sys_trace_thread_info_user(struct k_thread *thread);
void sys_trace_thread_priority_set_user(struct k_thread *thread, int prio);
void sys_trace_thread_sched_ready_user(struct k_thread *thread);
void sys_trace_thread_pend_user(struct k_thread *thread);
void sys_trace_isr_enter_user(void);
void sys_trace_isr_exit_user(void);
void sys_trace_idle_user(void);
void sys_trace_sys_init_enter_user(const struct init_entry *entry, int level);
void sys_trace_sys_init_exit_user(const struct init_entry *entry, int level, int result);
void sys_trace_k_thread_sleep_enter_user(k_timeout_t timeout);
void sys_trace_k_thread_sleep_exit_user(k_timeout_t timeout, int32_t ret);
void sys_trace_k_thread_msleep_enter_user(int32_t ms);
void sys_trace_k_thread_msleep_exit_user(int32_t ms, int32_t ret);
void sys_trace_k_thread_usleep_enter_user(int32_t us);
void sys_trace_k_thread_usleep_exit_user(int32_t us, int32_t ret);

void sys_trace_thread_create(struct k_thread *thread);
void sys_trace_thread_abort(struct k_thread *thread);
void sys_trace_thread_suspend(struct k_thread *thread);
void sys_trace_thread_resume(struct k_thread *thread);
void sys_trace_thread_name_set(struct k_thread *thread);
void sys_trace_k_thread_switched_in(void);
void sys_trace_k_thread_switched_out(void);
void sys_trace_thread_info(struct k_thread *thread);
void sys_trace_thread_sched_priority_set(struct k_thread *thread, int prio);
void sys_trace_thread_sched_ready(struct k_thread *thread);
void sys_trace_thread_pend(struct k_thread *thread);
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);
void sys_trace_idle(void);
void sys_trace_idle_exit(void);
void sys_trace_sys_init_enter(const struct init_entry *entry, int level);
void sys_trace_sys_init_exit(const struct init_entry *entry, int level, int result);
void sys_trace_k_thread_sleep_enter(k_timeout_t timeout);
void sys_trace_k_thread_sleep_exit(k_timeout_t timeout, int32_t ret);
void sys_trace_k_thread_msleep_enter(int32_t ms);
void sys_trace_k_thread_msleep_exit(int32_t ms, int32_t ret);
void sys_trace_k_thread_usleep_enter(int32_t us);
void sys_trace_k_thread_usleep_exit(int32_t us, int32_t ret);
void sys_trace_timer_init(struct k_timer *timer);
void sys_trace_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period);
void sys_trace_timer_stop(struct k_timer *timer);
void sys_trace_timer_status_sync_enter(struct k_timer *timer);
void sys_trace_timer_status_sync_blocking(struct k_timer *timer, k_timeout_t timeout);
void sys_trace_timer_status_sync_exit(struct k_timer *timer, uint32_t result);
void sys_trace_timer_expiry_enter(struct k_timer *timer);
void sys_trace_timer_expiry_exit(struct k_timer *timer);
void sys_trace_timer_stop_fn_expiry_enter(struct k_timer *timer);
void sys_trace_timer_stop_fn_expiry_exit(struct k_timer *timer);

struct rtio;
struct rtio_sqe;
struct rtio_cqe;
struct rtio_iodev_sqe;
struct gpio_callback;
typedef uint8_t gpio_pin_t;
typedef uint32_t gpio_flags_t;
typedef uint32_t gpio_port_pins_t;
typedef uint32_t gpio_port_value_t;
typedef void (*gpio_callback_handler_t)(const struct device *port, struct gpio_callback *cb,
					gpio_port_pins_t pins);
void sys_trace_gpio_pin_interrupt_configure_enter_user(const struct device *port, gpio_pin_t pin,
						       gpio_flags_t flags);
void sys_trace_gpio_pin_interrupt_configure_exit_user(const struct device *port, gpio_pin_t pin,
						      int ret);
void sys_trace_gpio_pin_configure_enter_user(const struct device *port, gpio_pin_t pin,
					     gpio_flags_t flags);
void sys_trace_gpio_pin_configure_exit_user(const struct device *port, gpio_pin_t pin, int ret);
void sys_trace_gpio_port_get_direction_enter_user(const struct device *port, gpio_port_pins_t map,
						  gpio_port_pins_t inputs,
						  gpio_port_pins_t outputs);
void sys_trace_gpio_port_get_direction_exit_user(const struct device *port, int ret);
void sys_trace_gpio_pin_get_config_enter_user(const struct device *port, gpio_pin_t pin, int ret);
void sys_trace_gpio_pin_get_config_exit_user(const struct device *port, gpio_pin_t pin, int ret);
void sys_trace_gpio_port_get_raw_enter_user(const struct device *port, gpio_port_value_t *value);
void sys_trace_gpio_port_get_raw_exit_user(const struct device *port, int ret);
void sys_trace_gpio_port_set_masked_raw_enter_user(const struct device *port, gpio_port_pins_t mask,
						   gpio_port_value_t value);
void sys_trace_gpio_port_set_masked_raw_exit_user(const struct device *port, int ret);
void sys_trace_gpio_port_set_bits_raw_enter_user(const struct device *port, gpio_port_pins_t pins);
void sys_trace_gpio_port_set_bits_raw_exit_user(const struct device *port, int ret);
void sys_trace_gpio_port_clear_bits_raw_enter_user(const struct device *port,
						   gpio_port_pins_t pins);
void sys_trace_gpio_port_clear_bits_raw_exit_user(const struct device *port, int ret);
void sys_trace_gpio_port_toggle_bits_enter_user(const struct device *port, gpio_port_pins_t pins);
void sys_trace_gpio_port_toggle_bits_exit_user(const struct device *port, int ret);
void sys_trace_gpio_init_callback_enter_user(struct gpio_callback *callback,
					     gpio_callback_handler_t handler,
					     gpio_port_pins_t pin_mask);
void sys_trace_gpio_init_callback_exit_user(struct gpio_callback *callback);
void sys_trace_gpio_add_callback_enter_user(const struct device *port,
					    struct gpio_callback *callback);
void sys_trace_gpio_add_callback_exit_user(const struct device *port, int ret);
void sys_trace_gpio_remove_callback_enter_user(const struct device *port,
					       struct gpio_callback *callback);
void sys_trace_gpio_remove_callback_exit_user(const struct device *port, int ret);
void sys_trace_gpio_get_pending_int_enter_user(const struct device *dev);
void sys_trace_gpio_get_pending_int_exit_user(const struct device *dev, int ret);
void sys_trace_gpio_fire_callbacks_enter_user(sys_slist_t *list, const struct device *port,
					      gpio_pin_t pins);
void sys_trace_gpio_fire_callback_user(const struct device *port, struct gpio_callback *callback);

void sys_trace_rtio_submit_enter(const struct rtio *r, uint32_t wait_count);
void sys_trace_rtio_submit_exit(const struct rtio *r);
void sys_trace_rtio_sqe_acquire_enter(const struct rtio *r);
void sys_trace_rtio_sqe_acquire_exit(const struct rtio *r, const struct rtio_sqe *sqe);
void sys_trace_rtio_sqe_cancel(const struct rtio_sqe *sqe);
void sys_trace_rtio_cqe_submit_enter(const struct rtio *r, int result, uint32_t flags);
void sys_trace_rtio_cqe_submit_exit(const struct rtio *r);
void sys_trace_rtio_cqe_acquire_enter(const struct rtio *r);
void sys_trace_rtio_cqe_acquire_exit(const struct rtio *r, const struct rtio_cqe *cqe);
void sys_trace_rtio_cqe_release(const struct rtio *r, const struct rtio_cqe *cqe);
void sys_trace_rtio_cqe_consume_enter(const struct rtio *r);
void sys_trace_rtio_cqe_consume_exit(const struct rtio *r, const struct rtio_cqe *cqe);
void sys_trace_rtio_txn_next_enter(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe);
void sys_trace_rtio_txn_next_exit(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe);
void sys_trace_rtio_chain_next_enter(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe);
void sys_trace_rtio_chain_next_exit(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe);

#define sys_port_trace_k_thread_create(new_thread) sys_trace_thread_create(new_thread)
#define sys_port_trace_k_thread_sleep_enter(timeout) sys_trace_k_thread_sleep_enter(timeout)
#define sys_port_trace_k_thread_sleep_exit(timeout, ret) sys_trace_k_thread_sleep_exit(timeout, ret)
#define sys_port_trace_k_thread_msleep_enter(ms) sys_trace_k_thread_msleep_enter(ms)
#define sys_port_trace_k_thread_msleep_exit(ms, ret) sys_trace_k_thread_msleep_exit(ms, ret)
#define sys_port_trace_k_thread_usleep_enter(us) sys_trace_k_thread_usleep_enter(us)
#define sys_port_trace_k_thread_usleep_exit(us, ret) sys_trace_k_thread_usleep_exit(us, ret)
#define sys_port_trace_k_thread_abort(thread) sys_trace_thread_abort(thread)
#define sys_port_trace_k_thread_suspend_enter(thread) sys_trace_thread_suspend(thread)
#define sys_port_trace_k_thread_resume_enter(thread) sys_trace_thread_resume(thread)
#define sys_port_trace_k_thread_name_set(thread, ret) sys_trace_thread_name_set(thread)
#define sys_port_trace_k_thread_switched_out() sys_trace_k_thread_switched_out()
#define sys_port_trace_k_thread_switched_in() sys_trace_k_thread_switched_in()
#define sys_port_trace_k_thread_info(thread) sys_trace_thread_info(thread)

#define sys_port_trace_k_thread_sched_priority_set(thread, prio) \
	sys_trace_thread_sched_priority_set(thread, prio)
#define sys_port_trace_k_thread_sched_ready(thread) sys_trace_thread_sched_ready(thread)
#define sys_port_trace_k_thread_sched_pend(thread) sys_trace_thread_pend(thread)

#define sys_port_trace_k_timer_init(timer)					\
					sys_trace_timer_init(timer)
#define sys_port_trace_k_timer_start(timer, duration, period)			\
					sys_trace_timer_start(timer, duration, period)
#define sys_port_trace_k_timer_stop(timer)					\
					sys_trace_timer_stop(timer)
#define sys_port_trace_k_timer_status_sync_enter(timer)				\
					sys_trace_timer_status_sync_enter(timer)
#define sys_port_trace_k_timer_status_sync_blocking(timer, timeout)		\
					sys_trace_timer_status_sync_blocking(timer, timeout)
#define sys_port_trace_k_timer_status_sync_exit(timer, result)			\
					sys_trace_timer_status_sync_exit(timer, result)
#define sys_port_trace_k_timer_expiry_enter(timer)				\
					sys_trace_timer_expiry_enter(timer)
#define sys_port_trace_k_timer_expiry_exit(timer)				\
					sys_trace_timer_expiry_exit(timer)
#define sys_port_trace_k_timer_stop_fn_expiry_enter(timer)			\
					sys_trace_timer_stop_fn_expiry_enter(timer)
#define sys_port_trace_k_timer_stop_fn_expiry_exit(timer)			\
					sys_trace_timer_stop_fn_expiry_exit(timer)

#define sys_trace_named_event(name, arg0, arg1)

#define sys_port_trace_gpio_pin_interrupt_configure_enter(port, pin, flags) \
	sys_trace_gpio_pin_interrupt_configure_enter_user(port, pin, flags)
#define sys_port_trace_gpio_pin_interrupt_configure_exit(port, pin, ret) \
	sys_trace_gpio_pin_interrupt_configure_exit_user(port, pin, ret)
#define sys_port_trace_gpio_pin_configure_enter(port, pin, flags) \
	sys_trace_gpio_pin_configure_enter_user(port, pin, flags)
#define sys_port_trace_gpio_pin_configure_exit(port, pin, ret) \
	sys_trace_gpio_pin_configure_exit_user(port, pin, ret)
#define sys_port_trace_gpio_port_get_direction_enter(port, map, inputs, outputs) \
	sys_trace_gpio_port_get_direction_enter_user(port, map, inputs, outputs)
#define sys_port_trace_gpio_port_get_direction_exit(port, ret) \
	sys_trace_gpio_port_get_direction_exit_user(port, ret)
#define sys_port_trace_gpio_pin_get_config_enter(port, pin, ret) \
	sys_trace_gpio_pin_get_config_enter_user(port, pin, ret)
#define sys_port_trace_gpio_pin_get_config_exit(port, pin, ret) \
	sys_trace_gpio_pin_get_config_exit_user(port, pin, ret)
#define sys_port_trace_gpio_port_get_raw_enter(port, value) \
	sys_trace_gpio_port_get_raw_enter_user(port, value)
#define sys_port_trace_gpio_port_get_raw_exit(port, ret) \
	sys_trace_gpio_port_get_raw_exit_user(port, ret)
#define sys_port_trace_gpio_port_set_masked_raw_enter(port, mask, value) \
	sys_trace_gpio_port_set_masked_raw_enter_user(port, mask, value)
#define sys_port_trace_gpio_port_set_masked_raw_exit(port, ret) \
	sys_trace_gpio_port_set_masked_raw_exit_user(port, ret)
#define sys_port_trace_gpio_port_set_bits_raw_enter(port, pins) \
	sys_trace_gpio_port_set_bits_raw_enter_user(port, pins)
#define sys_port_trace_gpio_port_set_bits_raw_exit(port, ret) \
	sys_trace_gpio_port_set_bits_raw_exit_user(port, ret)
#define sys_port_trace_gpio_port_clear_bits_raw_enter(port, pins) \
	sys_trace_gpio_port_clear_bits_raw_enter_user(port, pins)
#define sys_port_trace_gpio_port_clear_bits_raw_exit(port, ret) \
	sys_trace_gpio_port_clear_bits_raw_exit_user(port, ret)
#define sys_port_trace_gpio_port_toggle_bits_enter(port, pins) \
	sys_trace_gpio_port_toggle_bits_enter_user(port, pins)
#define sys_port_trace_gpio_port_toggle_bits_exit(port, ret) \
	sys_trace_gpio_port_toggle_bits_exit_user(port, ret)
#define sys_port_trace_gpio_init_callback_enter(callback, handler, pin_mask) \
	sys_trace_gpio_init_callback_enter_user(callback, handler, pin_mask)
#define sys_port_trace_gpio_init_callback_exit(callback) \
	sys_trace_gpio_init_callback_exit_user(callback)
#define sys_port_trace_gpio_add_callback_enter(port, callback) \
	sys_trace_gpio_add_callback_enter_user(port, callback)
#define sys_port_trace_gpio_add_callback_exit(port, ret) \
	sys_trace_gpio_add_callback_exit_user(port, ret)
#define sys_port_trace_gpio_remove_callback_enter(port, callback) \
	sys_trace_gpio_remove_callback_enter_user(port, callback)
#define sys_port_trace_gpio_remove_callback_exit(port, ret) \
	sys_trace_gpio_remove_callback_exit_user(port, ret)
#define sys_port_trace_gpio_get_pending_int_enter(dev) \
	sys_trace_gpio_get_pending_int_enter_user(dev)
#define sys_port_trace_gpio_get_pending_int_exit(dev, ret) \
	sys_trace_gpio_get_pending_int_exit_user(dev, ret)
#define sys_port_trace_gpio_fire_callbacks_enter(list, port, pins) \
	sys_trace_gpio_fire_callbacks_enter_user(list, port, pins)
#define sys_port_trace_gpio_fire_callback(port, callback) \
	sys_trace_gpio_fire_callback_user(port, callback)

#define sys_port_trace_rtio_submit_enter(rtio, wait_count)                                         \
	sys_trace_rtio_submit_enter(rtio, wait_count)

#define sys_port_trace_rtio_submit_exit(rtio) sys_trace_rtio_submit_exit(rtio)

#define sys_port_trace_rtio_sqe_acquire_enter(rtio) sys_trace_rtio_sqe_acquire_enter(rtio)

#define sys_port_trace_rtio_sqe_acquire_exit(rtio, sqe) sys_trace_rtio_sqe_acquire_exit(rtio, sqe)

#define sys_port_trace_rtio_sqe_cancel(sqe) sys_trace_rtio_sqe_cancel(sqe)

#define sys_port_trace_rtio_cqe_submit_enter(rtio, result, flags)                                  \
	sys_trace_rtio_cqe_submit_enter(rtio, result, flags)

#define sys_port_trace_rtio_cqe_submit_exit(rtio) sys_trace_rtio_cqe_submit_exit(rtio)

#define sys_port_trace_rtio_cqe_acquire_enter(rtio) sys_trace_rtio_cqe_acquire_enter(rtio)

#define sys_port_trace_rtio_cqe_acquire_exit(rtio, cqe) sys_trace_rtio_cqe_acquire_exit(rtio, cqe)

#define sys_port_trace_rtio_cqe_release(rtio, cqe) sys_trace_rtio_cqe_release(rtio, cqe)

#define sys_port_trace_rtio_cqe_consume_enter(rtio) sys_trace_rtio_cqe_consume_enter(rtio)

#define sys_port_trace_rtio_cqe_consume_exit(rtio, cqe) sys_trace_rtio_cqe_consume_exit(rtio, cqe)

#define sys_port_trace_rtio_txn_next_enter(rtio, iodev_sqe)                                        \
	sys_trace_rtio_txn_next_enter(rtio, iodev_sqe)

#define sys_port_trace_rtio_txn_next_exit(rtio, iodev_sqe)                                         \
	sys_trace_rtio_txn_next_exit(rtio, iodev_sqe)

#define sys_port_trace_rtio_chain_next_enter(rtio, iodev_sqe)                                      \
	sys_trace_rtio_chain_next_enter(rtio, iodev_sqe)

#define sys_port_trace_rtio_chain_next_exit(rtio, iodev_sqe)                                       \
	sys_trace_rtio_chain_next_exit(rtio, iodev_sqe)

/*
 * Kernel-object hooks. Each sys_port_trace_k_*() macro routes to a typed
 * sys_trace_k_*() trampoline (tracing_user.c) that forwards to a __weak
 * sys_trace_k_*_user() callback a downstream integrator can override. Object
 * types not covered here (queue/fifo/lifo/stack/pipe/heap) and any undefined
 * hook fall back to a no-op via tracing_hooks.h below.
 */

/* k_condvar */
void sys_trace_k_condvar_broadcast_enter(struct k_condvar *condvar);
void sys_trace_k_condvar_broadcast_enter_user(struct k_condvar *condvar);
void sys_trace_k_condvar_broadcast_exit(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_broadcast_exit_user(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_init(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_init_user(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_signal_blocking(struct k_condvar *condvar, k_timeout_t timeout);
void sys_trace_k_condvar_signal_blocking_user(struct k_condvar *condvar, k_timeout_t timeout);
void sys_trace_k_condvar_signal_enter(struct k_condvar *condvar);
void sys_trace_k_condvar_signal_enter_user(struct k_condvar *condvar);
void sys_trace_k_condvar_signal_exit(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_signal_exit_user(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_wait_enter(struct k_condvar *condvar, k_timeout_t timeout);
void sys_trace_k_condvar_wait_enter_user(struct k_condvar *condvar, k_timeout_t timeout);
void sys_trace_k_condvar_wait_exit(struct k_condvar *condvar, k_timeout_t timeout, int ret);
void sys_trace_k_condvar_wait_exit_user(struct k_condvar *condvar, k_timeout_t timeout, int ret);
#define sys_port_trace_k_condvar_broadcast_enter(condvar)                                          \
	sys_trace_k_condvar_broadcast_enter(condvar)
#define sys_port_trace_k_condvar_broadcast_exit(condvar, ret)                                      \
	sys_trace_k_condvar_broadcast_exit(condvar, ret)
#define sys_port_trace_k_condvar_init(condvar, ret) sys_trace_k_condvar_init(condvar, ret)
#define sys_port_trace_k_condvar_signal_blocking(condvar, timeout)                                 \
	sys_trace_k_condvar_signal_blocking(condvar, timeout)
#define sys_port_trace_k_condvar_signal_enter(condvar) sys_trace_k_condvar_signal_enter(condvar)
#define sys_port_trace_k_condvar_signal_exit(condvar, ret)                                         \
	sys_trace_k_condvar_signal_exit(condvar, ret)
#define sys_port_trace_k_condvar_wait_enter(condvar, timeout)                                      \
	sys_trace_k_condvar_wait_enter(condvar, timeout)
#define sys_port_trace_k_condvar_wait_exit(condvar, timeout, ret)                                  \
	sys_trace_k_condvar_wait_exit(condvar, timeout, ret)

/* k_event */
void sys_trace_k_event_init(struct k_event *event);
void sys_trace_k_event_init_user(struct k_event *event);
void sys_trace_k_event_post_enter(struct k_event *event, uint32_t events, uint32_t events_mask);
void sys_trace_k_event_post_enter_user(struct k_event *event, uint32_t events,
				       uint32_t events_mask);
void sys_trace_k_event_post_exit(struct k_event *event, uint32_t events, uint32_t events_mask);
void sys_trace_k_event_post_exit_user(struct k_event *event, uint32_t events, uint32_t events_mask);
void sys_trace_k_event_wait_blocking(struct k_event *event, uint32_t events, uint32_t options,
				     k_timeout_t timeout);
void sys_trace_k_event_wait_blocking_user(struct k_event *event, uint32_t events, uint32_t options,
					  k_timeout_t timeout);
void sys_trace_k_event_wait_enter(struct k_event *event, uint32_t events, uint32_t options,
				  k_timeout_t timeout);
void sys_trace_k_event_wait_enter_user(struct k_event *event, uint32_t events, uint32_t options,
				       k_timeout_t timeout);
void sys_trace_k_event_wait_exit(struct k_event *event, uint32_t events, int ret);
void sys_trace_k_event_wait_exit_user(struct k_event *event, uint32_t events, int ret);
#define sys_port_trace_k_event_init(event) sys_trace_k_event_init(event)
#define sys_port_trace_k_event_post_enter(event, events, events_mask)                              \
	sys_trace_k_event_post_enter(event, events, events_mask)
#define sys_port_trace_k_event_post_exit(event, events, events_mask)                               \
	sys_trace_k_event_post_exit(event, events, events_mask)
#define sys_port_trace_k_event_wait_blocking(event, events, options, timeout)                      \
	sys_trace_k_event_wait_blocking(event, events, options, timeout)
#define sys_port_trace_k_event_wait_enter(event, events, options, timeout)                         \
	sys_trace_k_event_wait_enter(event, events, options, timeout)
#define sys_port_trace_k_event_wait_exit(event, events, ret)                                       \
	sys_trace_k_event_wait_exit(event, events, ret)

/* k_mbox */
void sys_trace_k_mbox_async_put_enter(struct k_mbox *mbox, struct k_sem *sem);
void sys_trace_k_mbox_async_put_enter_user(struct k_mbox *mbox, struct k_sem *sem);
void sys_trace_k_mbox_async_put_exit(struct k_mbox *mbox, struct k_sem *sem);
void sys_trace_k_mbox_async_put_exit_user(struct k_mbox *mbox, struct k_sem *sem);
void sys_trace_k_mbox_data_get(struct k_mbox_msg *rx_msg);
void sys_trace_k_mbox_data_get_user(struct k_mbox_msg *rx_msg);
void sys_trace_k_mbox_get_blocking(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_get_blocking_user(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_get_enter(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_get_enter_user(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_get_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret);
void sys_trace_k_mbox_get_exit_user(struct k_mbox *mbox, k_timeout_t timeout, int ret);
void sys_trace_k_mbox_init(struct k_mbox *mbox);
void sys_trace_k_mbox_init_user(struct k_mbox *mbox);
void sys_trace_k_mbox_message_put_blocking(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_message_put_blocking_user(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_message_put_enter(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_message_put_enter_user(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_message_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret);
void sys_trace_k_mbox_message_put_exit_user(struct k_mbox *mbox, k_timeout_t timeout, int ret);
void sys_trace_k_mbox_put_enter(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_put_enter_user(struct k_mbox *mbox, k_timeout_t timeout);
void sys_trace_k_mbox_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret);
void sys_trace_k_mbox_put_exit_user(struct k_mbox *mbox, k_timeout_t timeout, int ret);
#define sys_port_trace_k_mbox_async_put_enter(mbox, sem) sys_trace_k_mbox_async_put_enter(mbox, sem)
#define sys_port_trace_k_mbox_async_put_exit(mbox, sem) sys_trace_k_mbox_async_put_exit(mbox, sem)
#define sys_port_trace_k_mbox_data_get(rx_msg) sys_trace_k_mbox_data_get(rx_msg)
#define sys_port_trace_k_mbox_get_blocking(mbox, timeout)                                          \
	sys_trace_k_mbox_get_blocking(mbox, timeout)
#define sys_port_trace_k_mbox_get_enter(mbox, timeout) sys_trace_k_mbox_get_enter(mbox, timeout)
#define sys_port_trace_k_mbox_get_exit(mbox, timeout, ret)                                         \
	sys_trace_k_mbox_get_exit(mbox, timeout, ret)
#define sys_port_trace_k_mbox_init(mbox) sys_trace_k_mbox_init(mbox)
#define sys_port_trace_k_mbox_message_put_blocking(mbox, timeout)                                  \
	sys_trace_k_mbox_message_put_blocking(mbox, timeout)
#define sys_port_trace_k_mbox_message_put_enter(mbox, timeout)                                     \
	sys_trace_k_mbox_message_put_enter(mbox, timeout)
#define sys_port_trace_k_mbox_message_put_exit(mbox, timeout, ret)                                 \
	sys_trace_k_mbox_message_put_exit(mbox, timeout, ret)
#define sys_port_trace_k_mbox_put_enter(mbox, timeout) sys_trace_k_mbox_put_enter(mbox, timeout)
#define sys_port_trace_k_mbox_put_exit(mbox, timeout, ret)                                         \
	sys_trace_k_mbox_put_exit(mbox, timeout, ret)

/* k_mem */
void sys_trace_k_mem_slab_alloc_blocking(struct k_mem_slab *slab, k_timeout_t timeout);
void sys_trace_k_mem_slab_alloc_blocking_user(struct k_mem_slab *slab, k_timeout_t timeout);
void sys_trace_k_mem_slab_alloc_enter(struct k_mem_slab *slab, k_timeout_t timeout);
void sys_trace_k_mem_slab_alloc_enter_user(struct k_mem_slab *slab, k_timeout_t timeout);
void sys_trace_k_mem_slab_alloc_exit(struct k_mem_slab *slab, k_timeout_t timeout, int ret);
void sys_trace_k_mem_slab_alloc_exit_user(struct k_mem_slab *slab, k_timeout_t timeout, int ret);
void sys_trace_k_mem_slab_free_enter(struct k_mem_slab *slab);
void sys_trace_k_mem_slab_free_enter_user(struct k_mem_slab *slab);
void sys_trace_k_mem_slab_free_exit(struct k_mem_slab *slab);
void sys_trace_k_mem_slab_free_exit_user(struct k_mem_slab *slab);
void sys_trace_k_mem_slab_init(struct k_mem_slab *slab, int ret);
void sys_trace_k_mem_slab_init_user(struct k_mem_slab *slab, int ret);
#define sys_port_trace_k_mem_slab_alloc_blocking(slab, timeout)                                    \
	sys_trace_k_mem_slab_alloc_blocking(slab, timeout)
#define sys_port_trace_k_mem_slab_alloc_enter(slab, timeout)                                       \
	sys_trace_k_mem_slab_alloc_enter(slab, timeout)
#define sys_port_trace_k_mem_slab_alloc_exit(slab, timeout, ret)                                   \
	sys_trace_k_mem_slab_alloc_exit(slab, timeout, ret)
#define sys_port_trace_k_mem_slab_free_enter(slab) sys_trace_k_mem_slab_free_enter(slab)
#define sys_port_trace_k_mem_slab_free_exit(slab) sys_trace_k_mem_slab_free_exit(slab)
#define sys_port_trace_k_mem_slab_init(slab, rc) sys_trace_k_mem_slab_init(slab, rc)

/* k_msgq */
void sys_trace_k_msgq_alloc_init_enter(struct k_msgq *msgq);
void sys_trace_k_msgq_alloc_init_enter_user(struct k_msgq *msgq);
void sys_trace_k_msgq_alloc_init_exit(struct k_msgq *msgq, int ret);
void sys_trace_k_msgq_alloc_init_exit_user(struct k_msgq *msgq, int ret);
void sys_trace_k_msgq_cleanup_enter(struct k_msgq *msgq);
void sys_trace_k_msgq_cleanup_enter_user(struct k_msgq *msgq);
void sys_trace_k_msgq_cleanup_exit(struct k_msgq *msgq, int ret);
void sys_trace_k_msgq_cleanup_exit_user(struct k_msgq *msgq, int ret);
void sys_trace_k_msgq_get_blocking(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_get_blocking_user(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_get_enter(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_get_enter_user(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_get_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret);
void sys_trace_k_msgq_get_exit_user(struct k_msgq *msgq, k_timeout_t timeout, int ret);
void sys_trace_k_msgq_init(struct k_msgq *msgq);
void sys_trace_k_msgq_init_user(struct k_msgq *msgq);
void sys_trace_k_msgq_peek(struct k_msgq *msgq, int ret);
void sys_trace_k_msgq_peek_user(struct k_msgq *msgq, int ret);
void sys_trace_k_msgq_purge(struct k_msgq *msgq);
void sys_trace_k_msgq_purge_user(struct k_msgq *msgq);
void sys_trace_k_msgq_put_blocking(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_blocking_user(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_enter(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_enter_user(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret);
void sys_trace_k_msgq_put_exit_user(struct k_msgq *msgq, k_timeout_t timeout, int ret);
void sys_trace_k_msgq_put_front_blocking(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_front_blocking_user(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_front_enter(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_front_enter_user(struct k_msgq *msgq, k_timeout_t timeout);
void sys_trace_k_msgq_put_front_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret);
void sys_trace_k_msgq_put_front_exit_user(struct k_msgq *msgq, k_timeout_t timeout, int ret);
#define sys_port_trace_k_msgq_alloc_init_enter(msgq) sys_trace_k_msgq_alloc_init_enter(msgq)
#define sys_port_trace_k_msgq_alloc_init_exit(msgq, ret) sys_trace_k_msgq_alloc_init_exit(msgq, ret)
#define sys_port_trace_k_msgq_cleanup_enter(msgq) sys_trace_k_msgq_cleanup_enter(msgq)
#define sys_port_trace_k_msgq_cleanup_exit(msgq, ret) sys_trace_k_msgq_cleanup_exit(msgq, ret)
#define sys_port_trace_k_msgq_get_blocking(msgq, timeout)                                          \
	sys_trace_k_msgq_get_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_get_enter(msgq, timeout) sys_trace_k_msgq_get_enter(msgq, timeout)
#define sys_port_trace_k_msgq_get_exit(msgq, timeout, ret)                                         \
	sys_trace_k_msgq_get_exit(msgq, timeout, ret)
#define sys_port_trace_k_msgq_init(msgq) sys_trace_k_msgq_init(msgq)
#define sys_port_trace_k_msgq_peek(msgq, ret) sys_trace_k_msgq_peek(msgq, ret)
#define sys_port_trace_k_msgq_purge(msgq) sys_trace_k_msgq_purge(msgq)
#define sys_port_trace_k_msgq_put_blocking(msgq, timeout)                                          \
	sys_trace_k_msgq_put_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_put_enter(msgq, timeout) sys_trace_k_msgq_put_enter(msgq, timeout)
#define sys_port_trace_k_msgq_put_exit(msgq, timeout, ret)                                         \
	sys_trace_k_msgq_put_exit(msgq, timeout, ret)
#define sys_port_trace_k_msgq_put_front_blocking(msgq, timeout)                                    \
	sys_trace_k_msgq_put_front_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_put_front_enter(msgq, timeout)                                       \
	sys_trace_k_msgq_put_front_enter(msgq, timeout)
#define sys_port_trace_k_msgq_put_front_exit(msgq, timeout, ret)                                   \
	sys_trace_k_msgq_put_front_exit(msgq, timeout, ret)

/* k_mutex */
void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret);
void sys_trace_k_mutex_init_user(struct k_mutex *mutex, int ret);
void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_blocking_user(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_enter_user(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret);
void sys_trace_k_mutex_lock_exit_user(struct k_mutex *mutex, k_timeout_t timeout, int ret);
void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex);
void sys_trace_k_mutex_unlock_enter_user(struct k_mutex *mutex);
void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret);
void sys_trace_k_mutex_unlock_exit_user(struct k_mutex *mutex, int ret);
#define sys_port_trace_k_mutex_init(mutex, ret) sys_trace_k_mutex_init(mutex, ret)
#define sys_port_trace_k_mutex_lock_blocking(mutex, timeout)                                       \
	sys_trace_k_mutex_lock_blocking(mutex, timeout)
#define sys_port_trace_k_mutex_lock_enter(mutex, timeout)                                          \
	sys_trace_k_mutex_lock_enter(mutex, timeout)
#define sys_port_trace_k_mutex_lock_exit(mutex, timeout, ret)                                      \
	sys_trace_k_mutex_lock_exit(mutex, timeout, ret)
#define sys_port_trace_k_mutex_unlock_enter(mutex) sys_trace_k_mutex_unlock_enter(mutex)
#define sys_port_trace_k_mutex_unlock_exit(mutex, ret) sys_trace_k_mutex_unlock_exit(mutex, ret)

/* k_poll */
void sys_trace_k_poll_api_event_init(struct k_poll_event *event);
void sys_trace_k_poll_api_event_init_user(struct k_poll_event *event);
void sys_trace_k_poll_api_poll_enter(struct k_poll_event *events);
void sys_trace_k_poll_api_poll_enter_user(struct k_poll_event *events);
void sys_trace_k_poll_api_poll_exit(struct k_poll_event *events, int ret);
void sys_trace_k_poll_api_poll_exit_user(struct k_poll_event *events, int ret);
void sys_trace_k_poll_api_signal_check(struct k_poll_signal *sig);
void sys_trace_k_poll_api_signal_check_user(struct k_poll_signal *sig);
void sys_trace_k_poll_api_signal_init(struct k_poll_signal *sig);
void sys_trace_k_poll_api_signal_init_user(struct k_poll_signal *sig);
void sys_trace_k_poll_api_signal_raise(struct k_poll_signal *sig, int ret);
void sys_trace_k_poll_api_signal_raise_user(struct k_poll_signal *sig, int ret);
void sys_trace_k_poll_api_signal_reset(struct k_poll_signal *sig);
void sys_trace_k_poll_api_signal_reset_user(struct k_poll_signal *sig);
#define sys_port_trace_k_poll_api_event_init(event) sys_trace_k_poll_api_event_init(event)
#define sys_port_trace_k_poll_api_poll_enter(events) sys_trace_k_poll_api_poll_enter(events)
#define sys_port_trace_k_poll_api_poll_exit(events, ret) sys_trace_k_poll_api_poll_exit(events, ret)
#define sys_port_trace_k_poll_api_signal_check(signal) sys_trace_k_poll_api_signal_check(signal)
#define sys_port_trace_k_poll_api_signal_init(signal) sys_trace_k_poll_api_signal_init(signal)
#define sys_port_trace_k_poll_api_signal_raise(signal, ret)                                        \
	sys_trace_k_poll_api_signal_raise(signal, ret)
#define sys_port_trace_k_poll_api_signal_reset(signal) sys_trace_k_poll_api_signal_reset(signal)

/* k_sem */
void sys_trace_k_sem_give_enter(struct k_sem *sem);
void sys_trace_k_sem_give_enter_user(struct k_sem *sem);
void sys_trace_k_sem_give_exit(struct k_sem *sem);
void sys_trace_k_sem_give_exit_user(struct k_sem *sem);
void sys_trace_k_sem_init(struct k_sem *sem, int ret);
void sys_trace_k_sem_init_user(struct k_sem *sem, int ret);
void sys_trace_k_sem_reset(struct k_sem *sem);
void sys_trace_k_sem_reset_user(struct k_sem *sem);
void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_blocking_user(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_enter_user(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret);
void sys_trace_k_sem_take_exit_user(struct k_sem *sem, k_timeout_t timeout, int ret);
#define sys_port_trace_k_sem_give_enter(sem) sys_trace_k_sem_give_enter(sem)
#define sys_port_trace_k_sem_give_exit(sem) sys_trace_k_sem_give_exit(sem)
#define sys_port_trace_k_sem_init(sem, ret) sys_trace_k_sem_init(sem, ret)
#define sys_port_trace_k_sem_reset(sem) sys_trace_k_sem_reset(sem)
#define sys_port_trace_k_sem_take_blocking(sem, timeout) sys_trace_k_sem_take_blocking(sem, timeout)
#define sys_port_trace_k_sem_take_enter(sem, timeout) sys_trace_k_sem_take_enter(sem, timeout)
#define sys_port_trace_k_sem_take_exit(sem, timeout, ret)                                          \
	sys_trace_k_sem_take_exit(sem, timeout, ret)

/* k_thread */
void sys_trace_k_thread_busy_wait_enter(uint32_t usec_to_wait);
void sys_trace_k_thread_busy_wait_enter_user(uint32_t usec_to_wait);
void sys_trace_k_thread_busy_wait_exit(uint32_t usec_to_wait);
void sys_trace_k_thread_busy_wait_exit_user(uint32_t usec_to_wait);
void sys_trace_k_thread_foreach_enter(void);
void sys_trace_k_thread_foreach_enter_user(void);
void sys_trace_k_thread_foreach_exit(void);
void sys_trace_k_thread_foreach_exit_user(void);
void sys_trace_k_thread_foreach_unlocked_enter(void);
void sys_trace_k_thread_foreach_unlocked_enter_user(void);
void sys_trace_k_thread_foreach_unlocked_exit(void);
void sys_trace_k_thread_foreach_unlocked_exit_user(void);
void sys_trace_k_thread_heap_assign(struct k_thread *thread, struct k_heap *heap);
void sys_trace_k_thread_heap_assign_user(struct k_thread *thread, struct k_heap *heap);
void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout);
void sys_trace_k_thread_join_blocking_user(struct k_thread *thread, k_timeout_t timeout);
void sys_trace_k_thread_join_enter(struct k_thread *thread, k_timeout_t timeout);
void sys_trace_k_thread_join_enter_user(struct k_thread *thread, k_timeout_t timeout);
void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret);
void sys_trace_k_thread_join_exit_user(struct k_thread *thread, k_timeout_t timeout, int ret);
void sys_trace_k_thread_sched_abort(struct k_thread *thread);
void sys_trace_k_thread_sched_abort_user(struct k_thread *thread);
void sys_trace_k_thread_sched_lock(void);
void sys_trace_k_thread_sched_lock_user(void);
void sys_trace_k_thread_sched_resume(struct k_thread *thread);
void sys_trace_k_thread_sched_resume_user(struct k_thread *thread);
void sys_trace_k_thread_sched_suspend(struct k_thread *thread);
void sys_trace_k_thread_sched_suspend_user(struct k_thread *thread);
void sys_trace_k_thread_sched_unlock(void);
void sys_trace_k_thread_sched_unlock_user(void);
void sys_trace_k_thread_sched_wakeup(struct k_thread *thread);
void sys_trace_k_thread_sched_wakeup_user(struct k_thread *thread);
void sys_trace_k_thread_start(struct k_thread *thread);
void sys_trace_k_thread_start_user(struct k_thread *thread);
void sys_trace_k_thread_suspend_exit(struct k_thread *thread);
void sys_trace_k_thread_suspend_exit_user(struct k_thread *thread);
void sys_trace_k_thread_user_mode_enter(void);
void sys_trace_k_thread_user_mode_enter_user(void);
void sys_trace_k_thread_wakeup(struct k_thread *thread);
void sys_trace_k_thread_wakeup_user(struct k_thread *thread);
void sys_trace_k_thread_yield(void);
void sys_trace_k_thread_yield_user(void);
#define sys_port_trace_k_thread_busy_wait_enter(usec_to_wait)                                      \
	sys_trace_k_thread_busy_wait_enter(usec_to_wait)
#define sys_port_trace_k_thread_busy_wait_exit(usec_to_wait)                                       \
	sys_trace_k_thread_busy_wait_exit(usec_to_wait)
#define sys_port_trace_k_thread_foreach_enter() sys_trace_k_thread_foreach_enter()
#define sys_port_trace_k_thread_foreach_exit() sys_trace_k_thread_foreach_exit()
#define sys_port_trace_k_thread_foreach_unlocked_enter() sys_trace_k_thread_foreach_unlocked_enter()
#define sys_port_trace_k_thread_foreach_unlocked_exit() sys_trace_k_thread_foreach_unlocked_exit()
#define sys_port_trace_k_thread_heap_assign(thread, heap)                                          \
	sys_trace_k_thread_heap_assign(thread, heap)
#define sys_port_trace_k_thread_join_blocking(thread, timeout)                                     \
	sys_trace_k_thread_join_blocking(thread, timeout)
#define sys_port_trace_k_thread_join_enter(thread, timeout)                                        \
	sys_trace_k_thread_join_enter(thread, timeout)
#define sys_port_trace_k_thread_join_exit(thread, timeout, ret)                                    \
	sys_trace_k_thread_join_exit(thread, timeout, ret)
#define sys_port_trace_k_thread_sched_abort(thread) sys_trace_k_thread_sched_abort(thread)
#define sys_port_trace_k_thread_sched_lock() sys_trace_k_thread_sched_lock()
#define sys_port_trace_k_thread_sched_resume(thread) sys_trace_k_thread_sched_resume(thread)
#define sys_port_trace_k_thread_sched_suspend(thread) sys_trace_k_thread_sched_suspend(thread)
#define sys_port_trace_k_thread_sched_unlock() sys_trace_k_thread_sched_unlock()
#define sys_port_trace_k_thread_sched_wakeup(thread) sys_trace_k_thread_sched_wakeup(thread)
#define sys_port_trace_k_thread_start(thread) sys_trace_k_thread_start(thread)
#define sys_port_trace_k_thread_suspend_exit(thread) sys_trace_k_thread_suspend_exit(thread)
#define sys_port_trace_k_thread_user_mode_enter() sys_trace_k_thread_user_mode_enter()
#define sys_port_trace_k_thread_wakeup(thread) sys_trace_k_thread_wakeup(thread)
#define sys_port_trace_k_thread_yield() sys_trace_k_thread_yield()

/* k_work */
void sys_trace_k_work_cancel_delayable_enter(struct k_work_delayable *dwork);
void sys_trace_k_work_cancel_delayable_enter_user(struct k_work_delayable *dwork);
void sys_trace_k_work_cancel_delayable_exit(struct k_work_delayable *dwork, int ret);
void sys_trace_k_work_cancel_delayable_exit_user(struct k_work_delayable *dwork, int ret);
void sys_trace_k_work_cancel_delayable_sync_enter(struct k_work_delayable *dwork,
						  struct k_work_sync *sync);
void sys_trace_k_work_cancel_delayable_sync_enter_user(struct k_work_delayable *dwork,
						       struct k_work_sync *sync);
void sys_trace_k_work_cancel_delayable_sync_exit(struct k_work_delayable *dwork,
						 struct k_work_sync *sync, int ret);
void sys_trace_k_work_cancel_delayable_sync_exit_user(struct k_work_delayable *dwork,
						      struct k_work_sync *sync, int ret);
void sys_trace_k_work_cancel_enter(struct k_work *work);
void sys_trace_k_work_cancel_enter_user(struct k_work *work);
void sys_trace_k_work_cancel_exit(struct k_work *work, int ret);
void sys_trace_k_work_cancel_exit_user(struct k_work *work, int ret);
void sys_trace_k_work_cancel_sync_blocking(struct k_work *work, struct k_work_sync *sync);
void sys_trace_k_work_cancel_sync_blocking_user(struct k_work *work, struct k_work_sync *sync);
void sys_trace_k_work_cancel_sync_enter(struct k_work *work, struct k_work_sync *sync);
void sys_trace_k_work_cancel_sync_enter_user(struct k_work *work, struct k_work_sync *sync);
void sys_trace_k_work_cancel_sync_exit(struct k_work *work, struct k_work_sync *sync, int ret);
void sys_trace_k_work_cancel_sync_exit_user(struct k_work *work, struct k_work_sync *sync, int ret);
void sys_trace_k_work_delayable_init(struct k_work_delayable *dwork);
void sys_trace_k_work_delayable_init_user(struct k_work_delayable *dwork);
void sys_trace_k_work_flush_blocking(struct k_work *work, k_timeout_t timeout);
void sys_trace_k_work_flush_blocking_user(struct k_work *work, k_timeout_t timeout);
void sys_trace_k_work_flush_delayable_enter(struct k_work_delayable *dwork,
					    struct k_work_sync *sync);
void sys_trace_k_work_flush_delayable_enter_user(struct k_work_delayable *dwork,
						 struct k_work_sync *sync);
void sys_trace_k_work_flush_delayable_exit(struct k_work_delayable *dwork, struct k_work_sync *sync,
					   int ret);
void sys_trace_k_work_flush_delayable_exit_user(struct k_work_delayable *dwork,
						struct k_work_sync *sync, int ret);
void sys_trace_k_work_flush_enter(struct k_work *work);
void sys_trace_k_work_flush_enter_user(struct k_work *work);
void sys_trace_k_work_flush_exit(struct k_work *work, int ret);
void sys_trace_k_work_flush_exit_user(struct k_work *work, int ret);
void sys_trace_k_work_init(struct k_work *work);
void sys_trace_k_work_init_user(struct k_work *work);
void sys_trace_k_work_poll_cancel_enter(struct k_work_poll *work);
void sys_trace_k_work_poll_cancel_enter_user(struct k_work_poll *work);
void sys_trace_k_work_poll_cancel_exit(struct k_work_poll *work, int ret);
void sys_trace_k_work_poll_cancel_exit_user(struct k_work_poll *work, int ret);
void sys_trace_k_work_poll_init_enter(struct k_work_poll *work);
void sys_trace_k_work_poll_init_enter_user(struct k_work_poll *work);
void sys_trace_k_work_poll_init_exit(struct k_work_poll *work);
void sys_trace_k_work_poll_init_exit_user(struct k_work_poll *work);
void sys_trace_k_work_poll_submit_enter(struct k_work_poll *work, k_timeout_t timeout);
void sys_trace_k_work_poll_submit_enter_user(struct k_work_poll *work, k_timeout_t timeout);
void sys_trace_k_work_poll_submit_exit(struct k_work_poll *work, k_timeout_t timeout, int ret);
void sys_trace_k_work_poll_submit_exit_user(struct k_work_poll *work, k_timeout_t timeout, int ret);
void sys_trace_k_work_poll_submit_to_queue_blocking(struct k_work_q *work_q,
						    struct k_work_poll *work, k_timeout_t timeout);
void sys_trace_k_work_poll_submit_to_queue_blocking_user(struct k_work_q *work_q,
							 struct k_work_poll *work,
							 k_timeout_t timeout);
void sys_trace_k_work_poll_submit_to_queue_enter(struct k_work_q *work_q, struct k_work_poll *work,
						 k_timeout_t timeout);
void sys_trace_k_work_poll_submit_to_queue_enter_user(struct k_work_q *work_q,
						      struct k_work_poll *work,
						      k_timeout_t timeout);
void sys_trace_k_work_poll_submit_to_queue_exit(struct k_work_q *work_q, struct k_work_poll *work,
						k_timeout_t timeout, int ret);
void sys_trace_k_work_poll_submit_to_queue_exit_user(struct k_work_q *work_q,
						     struct k_work_poll *work, k_timeout_t timeout,
						     int ret);
void sys_trace_k_work_queue_drain_enter(struct k_work_q *queue);
void sys_trace_k_work_queue_drain_enter_user(struct k_work_q *queue);
void sys_trace_k_work_queue_drain_exit(struct k_work_q *queue, int ret);
void sys_trace_k_work_queue_drain_exit_user(struct k_work_q *queue, int ret);
void sys_trace_k_work_queue_init(struct k_work_q *queue);
void sys_trace_k_work_queue_init_user(struct k_work_q *queue);
void sys_trace_k_work_queue_start_enter(struct k_work_q *queue);
void sys_trace_k_work_queue_start_enter_user(struct k_work_q *queue);
void sys_trace_k_work_queue_start_exit(struct k_work_q *queue);
void sys_trace_k_work_queue_start_exit_user(struct k_work_q *queue);
void sys_trace_k_work_queue_stop_blocking(struct k_work_q *queue, k_timeout_t timeout);
void sys_trace_k_work_queue_stop_blocking_user(struct k_work_q *queue, k_timeout_t timeout);
void sys_trace_k_work_queue_stop_enter(struct k_work_q *queue, k_timeout_t timeout);
void sys_trace_k_work_queue_stop_enter_user(struct k_work_q *queue, k_timeout_t timeout);
void sys_trace_k_work_queue_stop_exit(struct k_work_q *queue, k_timeout_t timeout, int ret);
void sys_trace_k_work_queue_stop_exit_user(struct k_work_q *queue, k_timeout_t timeout, int ret);
void sys_trace_k_work_queue_unplug_enter(struct k_work_q *queue);
void sys_trace_k_work_queue_unplug_enter_user(struct k_work_q *queue);
void sys_trace_k_work_queue_unplug_exit(struct k_work_q *queue, int ret);
void sys_trace_k_work_queue_unplug_exit_user(struct k_work_q *queue, int ret);
void sys_trace_k_work_reschedule_enter(struct k_work_delayable *dwork, k_timeout_t delay);
void sys_trace_k_work_reschedule_enter_user(struct k_work_delayable *dwork, k_timeout_t delay);
void sys_trace_k_work_reschedule_exit(struct k_work_delayable *dwork, k_timeout_t delay, int ret);
void sys_trace_k_work_reschedule_exit_user(struct k_work_delayable *dwork, k_timeout_t delay,
					   int ret);
void sys_trace_k_work_reschedule_for_queue_enter(struct k_work_q *queue,
						 struct k_work_delayable *dwork, k_timeout_t delay);
void sys_trace_k_work_reschedule_for_queue_enter_user(struct k_work_q *queue,
						      struct k_work_delayable *dwork,
						      k_timeout_t delay);
void sys_trace_k_work_reschedule_for_queue_exit(struct k_work_q *queue,
						struct k_work_delayable *dwork, k_timeout_t delay,
						int ret);
void sys_trace_k_work_reschedule_for_queue_exit_user(struct k_work_q *queue,
						     struct k_work_delayable *dwork,
						     k_timeout_t delay, int ret);
void sys_trace_k_work_schedule_enter(struct k_work_delayable *dwork, k_timeout_t delay);
void sys_trace_k_work_schedule_enter_user(struct k_work_delayable *dwork, k_timeout_t delay);
void sys_trace_k_work_schedule_exit(struct k_work_delayable *dwork, k_timeout_t delay, int ret);
void sys_trace_k_work_schedule_exit_user(struct k_work_delayable *dwork, k_timeout_t delay,
					 int ret);
void sys_trace_k_work_schedule_for_queue_enter(struct k_work_q *queue,
					       struct k_work_delayable *dwork, k_timeout_t delay);
void sys_trace_k_work_schedule_for_queue_enter_user(struct k_work_q *queue,
						    struct k_work_delayable *dwork,
						    k_timeout_t delay);
void sys_trace_k_work_schedule_for_queue_exit(struct k_work_q *queue,
					      struct k_work_delayable *dwork, k_timeout_t delay,
					      int ret);
void sys_trace_k_work_schedule_for_queue_exit_user(struct k_work_q *queue,
						   struct k_work_delayable *dwork,
						   k_timeout_t delay, int ret);
void sys_trace_k_work_submit_enter(struct k_work *work);
void sys_trace_k_work_submit_enter_user(struct k_work *work);
void sys_trace_k_work_submit_exit(struct k_work *work, int ret);
void sys_trace_k_work_submit_exit_user(struct k_work *work, int ret);
void sys_trace_k_work_submit_to_queue_enter(struct k_work_q *queue, struct k_work *work);
void sys_trace_k_work_submit_to_queue_enter_user(struct k_work_q *queue, struct k_work *work);
void sys_trace_k_work_submit_to_queue_exit(struct k_work_q *queue, struct k_work *work, int ret);
void sys_trace_k_work_submit_to_queue_exit_user(struct k_work_q *queue, struct k_work *work,
						int ret);
#define sys_port_trace_k_work_cancel_delayable_enter(dwork)                                        \
	sys_trace_k_work_cancel_delayable_enter(dwork)
#define sys_port_trace_k_work_cancel_delayable_exit(dwork, ret)                                    \
	sys_trace_k_work_cancel_delayable_exit(dwork, ret)
#define sys_port_trace_k_work_cancel_delayable_sync_enter(dwork, sync)                             \
	sys_trace_k_work_cancel_delayable_sync_enter(dwork, sync)
#define sys_port_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)                         \
	sys_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)
#define sys_port_trace_k_work_cancel_enter(work) sys_trace_k_work_cancel_enter(work)
#define sys_port_trace_k_work_cancel_exit(work, ret) sys_trace_k_work_cancel_exit(work, ret)
#define sys_port_trace_k_work_cancel_sync_blocking(work, sync)                                     \
	sys_trace_k_work_cancel_sync_blocking(work, sync)
#define sys_port_trace_k_work_cancel_sync_enter(work, sync)                                        \
	sys_trace_k_work_cancel_sync_enter(work, sync)
#define sys_port_trace_k_work_cancel_sync_exit(work, sync, ret)                                    \
	sys_trace_k_work_cancel_sync_exit(work, sync, ret)
#define sys_port_trace_k_work_delayable_init(dwork) sys_trace_k_work_delayable_init(dwork)
#define sys_port_trace_k_work_flush_blocking(work, timeout)                                        \
	sys_trace_k_work_flush_blocking(work, timeout)
#define sys_port_trace_k_work_flush_delayable_enter(dwork, sync)                                   \
	sys_trace_k_work_flush_delayable_enter(dwork, sync)
#define sys_port_trace_k_work_flush_delayable_exit(dwork, sync, ret)                               \
	sys_trace_k_work_flush_delayable_exit(dwork, sync, ret)
#define sys_port_trace_k_work_flush_enter(work) sys_trace_k_work_flush_enter(work)
#define sys_port_trace_k_work_flush_exit(work, ret) sys_trace_k_work_flush_exit(work, ret)
#define sys_port_trace_k_work_init(work) sys_trace_k_work_init(work)
#define sys_port_trace_k_work_poll_cancel_enter(work) sys_trace_k_work_poll_cancel_enter(work)
#define sys_port_trace_k_work_poll_cancel_exit(work, ret)                                          \
	sys_trace_k_work_poll_cancel_exit(work, ret)
#define sys_port_trace_k_work_poll_init_enter(work) sys_trace_k_work_poll_init_enter(work)
#define sys_port_trace_k_work_poll_init_exit(work) sys_trace_k_work_poll_init_exit(work)
#define sys_port_trace_k_work_poll_submit_enter(work, timeout)                                     \
	sys_trace_k_work_poll_submit_enter(work, timeout)
#define sys_port_trace_k_work_poll_submit_exit(work, timeout, ret)                                 \
	sys_trace_k_work_poll_submit_exit(work, timeout, ret)
#define sys_port_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)                 \
	sys_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)
#define sys_port_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)                    \
	sys_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)
#define sys_port_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)                \
	sys_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)
#define sys_port_trace_k_work_queue_drain_enter(queue) sys_trace_k_work_queue_drain_enter(queue)
#define sys_port_trace_k_work_queue_drain_exit(queue, ret)                                         \
	sys_trace_k_work_queue_drain_exit(queue, ret)
#define sys_port_trace_k_work_queue_init(queue) sys_trace_k_work_queue_init(queue)
#define sys_port_trace_k_work_queue_start_enter(queue) sys_trace_k_work_queue_start_enter(queue)
#define sys_port_trace_k_work_queue_start_exit(queue) sys_trace_k_work_queue_start_exit(queue)
#define sys_port_trace_k_work_queue_stop_blocking(queue, timeout)                                  \
	sys_trace_k_work_queue_stop_blocking(queue, timeout)
#define sys_port_trace_k_work_queue_stop_enter(queue, timeout)                                     \
	sys_trace_k_work_queue_stop_enter(queue, timeout)
#define sys_port_trace_k_work_queue_stop_exit(queue, timeout, ret)                                 \
	sys_trace_k_work_queue_stop_exit(queue, timeout, ret)
#define sys_port_trace_k_work_queue_unplug_enter(queue) sys_trace_k_work_queue_unplug_enter(queue)
#define sys_port_trace_k_work_queue_unplug_exit(queue, ret)                                        \
	sys_trace_k_work_queue_unplug_exit(queue, ret)
#define sys_port_trace_k_work_reschedule_enter(dwork, delay)                                       \
	sys_trace_k_work_reschedule_enter(dwork, delay)
#define sys_port_trace_k_work_reschedule_exit(dwork, delay, ret)                                   \
	sys_trace_k_work_reschedule_exit(dwork, delay, ret)
#define sys_port_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)                      \
	sys_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)
#define sys_port_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)                  \
	sys_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)
#define sys_port_trace_k_work_schedule_enter(dwork, delay)                                         \
	sys_trace_k_work_schedule_enter(dwork, delay)
#define sys_port_trace_k_work_schedule_exit(dwork, delay, ret)                                     \
	sys_trace_k_work_schedule_exit(dwork, delay, ret)
#define sys_port_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)                        \
	sys_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)
#define sys_port_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)                    \
	sys_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)
#define sys_port_trace_k_work_submit_enter(work) sys_trace_k_work_submit_enter(work)
#define sys_port_trace_k_work_submit_exit(work, ret) sys_trace_k_work_submit_exit(work, ret)
#define sys_port_trace_k_work_submit_to_queue_enter(queue, work)                                   \
	sys_trace_k_work_submit_to_queue_enter(queue, work)
#define sys_port_trace_k_work_submit_to_queue_exit(queue, work, ret)                               \
	sys_trace_k_work_submit_to_queue_exit(queue, work, ret)

#ifdef __cplusplus
}
#endif

/*
 * Fill any sys_port_trace_* hook not defined above with a canonical no-op. The
 * per-macro #ifndef guards keep the real definitions above; only gaps are filled,
 * so this header never has to drift from the canonical hook list.
 */
#include <zephyr/tracing/tracing_hooks.h>

#endif /* _TRACE_USER_H */
