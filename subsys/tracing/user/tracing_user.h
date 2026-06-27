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
