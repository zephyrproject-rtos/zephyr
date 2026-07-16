/*
 * Copyright (c) 2020 Lexmark International, Inc.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/init.h>

void __weak sys_trace_thread_create_user(struct k_thread *thread) {}
void __weak sys_trace_thread_abort_user(struct k_thread *thread) {}
void __weak sys_trace_thread_suspend_user(struct k_thread *thread) {}
void __weak sys_trace_thread_resume_user(struct k_thread *thread) {}
void __weak sys_trace_thread_name_set_user(struct k_thread *thread) {}
void __weak sys_trace_thread_switched_in_user(void) {}
void __weak sys_trace_thread_switched_out_user(void) {}
void __weak sys_trace_thread_info_user(struct k_thread *thread) {}
void __weak sys_trace_thread_sched_ready_user(struct k_thread *thread) {}
void __weak sys_trace_thread_pend_user(struct k_thread *thread) {}
void __weak sys_trace_thread_priority_set_user(struct k_thread *thread, int prio) {}
void __weak sys_trace_isr_enter_user(void) {}
void __weak sys_trace_isr_exit_user(void) {}
void __weak sys_trace_idle_user(void) {}
void __weak sys_trace_sys_init_enter_user(const struct init_entry *entry, int level) {}
void __weak sys_trace_sys_init_exit_user(const struct init_entry *entry, int level, int result) {}
void __weak sys_trace_k_thread_sleep_enter_user(k_timeout_t timeout) {}
void __weak sys_trace_k_thread_sleep_exit_user(k_timeout_t timeout, int32_t ret) {}
void __weak sys_trace_k_thread_msleep_enter_user(int32_t ms) {}
void __weak sys_trace_k_thread_msleep_exit_user(int32_t ms, int32_t ret) {}
void __weak sys_trace_k_thread_usleep_enter_user(int32_t us) {}
void __weak sys_trace_k_thread_usleep_exit_user(int32_t us, int32_t ret) {}
void __weak sys_trace_gpio_pin_interrupt_configure_enter_user(const struct device *port,
							      gpio_pin_t pin, gpio_flags_t flags) {}
void __weak sys_trace_gpio_pin_interrupt_configure_exit_user(const struct device *port,
							     gpio_pin_t pin, int ret) {}
void __weak sys_trace_gpio_pin_configure_enter_user(const struct device *port, gpio_pin_t pin,
						    gpio_flags_t flags) {}
void __weak sys_trace_gpio_pin_configure_exit_user(const struct device *port, gpio_pin_t pin,
						   int ret) {}
void __weak sys_trace_gpio_port_get_direction_enter_user(const struct device *port,
							 gpio_port_pins_t map,
							 gpio_port_pins_t inputs,
							 gpio_port_pins_t outputs) {}
void __weak sys_trace_gpio_port_get_direction_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_pin_get_config_enter_user(const struct device *port, gpio_pin_t pin,
						     int ret) {}
void __weak sys_trace_gpio_pin_get_config_exit_user(const struct device *port, gpio_pin_t pin,
						    int ret) {}
void __weak sys_trace_gpio_port_get_raw_enter_user(const struct device *port,
						   gpio_port_value_t *value) {}
void __weak sys_trace_gpio_port_get_raw_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_port_set_masked_raw_enter_user(const struct device *port,
							  gpio_port_pins_t mask,
							  gpio_port_value_t value) {}
void __weak sys_trace_gpio_port_set_masked_raw_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_port_set_bits_raw_enter_user(const struct device *port,
							gpio_port_pins_t pins) {}
void __weak sys_trace_gpio_port_set_bits_raw_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_port_clear_bits_raw_enter_user(const struct device *port,
							  gpio_port_pins_t pins) {}
void __weak sys_trace_gpio_port_clear_bits_raw_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_port_toggle_bits_enter_user(const struct device *port,
						       gpio_port_pins_t pins) {}
void __weak sys_trace_gpio_port_toggle_bits_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_init_callback_enter_user(struct gpio_callback *callback,
						    gpio_callback_handler_t handler,
						    gpio_port_pins_t pin_mask) {}
void __weak sys_trace_gpio_init_callback_exit_user(struct gpio_callback *callback) {}
void __weak sys_trace_gpio_add_callback_enter_user(const struct device *port,
						   struct gpio_callback *callback) {}
void __weak sys_trace_gpio_add_callback_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_remove_callback_enter_user(const struct device *port,
						      struct gpio_callback *callback) {}
void __weak sys_trace_gpio_remove_callback_exit_user(const struct device *port, int ret) {}
void __weak sys_trace_gpio_get_pending_int_enter_user(const struct device *dev) {}
void __weak sys_trace_gpio_get_pending_int_exit_user(const struct device *dev, int ret) {}
void __weak sys_trace_gpio_fire_callbacks_enter_user(sys_slist_t *list, const struct device *port,
						     gpio_pin_t pins) {}
void __weak sys_trace_gpio_fire_callback_user(const struct device *port,
					      struct gpio_callback *callback) {}
void __weak sys_trace_timer_init_user(struct k_timer *timer) {}
void __weak sys_trace_timer_start_user(struct k_timer *timer, k_timeout_t duration,
							    k_timeout_t period)
{}
void __weak sys_trace_timer_stop_user(struct k_timer *timer) {}
void __weak sys_trace_timer_status_sync_enter_user(struct k_timer *timer) {}
void __weak sys_trace_timer_status_sync_blocking_user(struct k_timer *timer, k_timeout_t timeout)
{}
void __weak sys_trace_timer_status_sync_exit_user(struct k_timer *timer, uint32_t result) {}
void __weak sys_trace_timer_expiry_enter_user(struct k_timer *timer) {}
void __weak sys_trace_timer_expiry_exit_user(struct k_timer *timer) {}
void __weak sys_trace_timer_stop_fn_expiry_enter_user(struct k_timer *timer) {}
void __weak sys_trace_timer_stop_fn_expiry_exit_user(struct k_timer *timer) {}

void __weak sys_trace_rtio_submit_enter_user(const struct rtio *r, uint32_t wait_count)
{
	printk("rtio_submit_enter_user: %p, wait_count: %d\n", r, wait_count);
}
void __weak sys_trace_rtio_submit_exit_user(const struct rtio *r)
{
	printk("rtio_submit_exit: rtio: %p\n", r);
}
void __weak sys_trace_rtio_sqe_acquire_enter_user(const struct rtio *r)
{
	printk("sqe_acquire_enter: rtio: %p\n", r);
}
void __weak sys_trace_rtio_sqe_acquire_exit_user(const struct rtio *r, const struct rtio_sqe *sqe)
{
	printk("sqe_acquire_exit: rtio: %p\t sqe: %p\n", r, sqe);
}
void __weak sys_trace_rtio_sqe_cancel_user(const struct rtio_sqe *sqe)
{
	printk("sqe_cancel_user: sqe: %p", sqe);
}
void __weak sys_trace_rtio_cqe_submit_enter_user(const struct rtio *r, int result, uint32_t flags)
{
	printk("cqe_submit_enter_user: rtio: %p\t result: %d\t flags: %d\n", r, result, flags);
}
void __weak sys_trace_rtio_cqe_submit_exit_user(const struct rtio *r)
{
	printk("cqe_submit_exit: rtio: %p\n", r);
}
void __weak sys_trace_rtio_cqe_acquire_enter_user(const struct rtio *r)
{
	printk("cqe_acquire_enter_user: rtio: %p\n", r);
}
void __weak sys_trace_rtio_cqe_acquire_exit_user(const struct rtio *r, const struct rtio_cqe *cqe)
{
	printk("cqe_acquire_exit_user: rtio: %p\t cqe: %p\n", r, cqe);
}
void __weak sys_trace_rtio_cqe_release_user(const struct rtio *r, const struct rtio_cqe *cqe)
{
	printk("cqe_release: rtio: %p\t cqe: %p\n", r, cqe);
}
void __weak sys_trace_rtio_cqe_consume_enter_user(const struct rtio *r)
{
	printk("cqe_consume_enter: rtio: %p\n", r);
}
void __weak sys_trace_rtio_cqe_consume_exit_user(const struct rtio *r, const struct rtio_cqe *cqe)
{
	printk("cqe_consume_exit: rtio: %p\t cqe: %p\n", r, cqe);
}
void __weak sys_trace_rtio_txn_next_enter_user(const struct rtio *r,
					       const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("txn_next_enter: rtio: %p\t iodev_sqe: %p\n", r, iodev_sqe);
}
void __weak sys_trace_rtio_txn_next_exit_user(const struct rtio *r,
					      const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("txn_next_exit: rtio: %p\t iodev_sqe: %p\n", r, iodev_sqe);
}
void __weak sys_trace_rtio_chain_next_enter_user(const struct rtio *r,
						 const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("chain_next_enter: rtio: %p\t iodev_sqe: %p\n", r, iodev_sqe);
}
void __weak sys_trace_rtio_chain_next_exit_user(const struct rtio *r,
						const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("chain_next_exit: rtio: %p\t iodev_sqe: %p\n", r, iodev_sqe);
}

void sys_trace_thread_create(struct k_thread *thread)
{
	sys_trace_thread_create_user(thread);
}

void sys_trace_thread_abort(struct k_thread *thread)
{
	sys_trace_thread_abort_user(thread);
}

void sys_trace_thread_suspend(struct k_thread *thread)
{
	sys_trace_thread_suspend_user(thread);
}

void sys_trace_thread_resume(struct k_thread *thread)
{
	sys_trace_thread_resume_user(thread);
}

void sys_trace_thread_name_set(struct k_thread *thread)
{
	sys_trace_thread_name_set_user(thread);
}

void sys_trace_k_thread_switched_in(void)
{
	sys_trace_thread_switched_in_user();
}

void sys_trace_k_thread_switched_out(void)
{
	sys_trace_thread_switched_out_user();
}

void sys_trace_thread_info(struct k_thread *thread)
{
	sys_trace_thread_info_user(thread);
}

void sys_trace_thread_sched_priority_set(struct k_thread *thread, int prio)
{
	sys_trace_thread_priority_set_user(thread, prio);
}

void sys_trace_thread_sched_ready(struct k_thread *thread)
{
	sys_trace_thread_sched_ready_user(thread);
}

void sys_trace_thread_pend(struct k_thread *thread)
{
	sys_trace_thread_pend_user(thread);
}

void sys_trace_isr_enter(void)
{
	sys_trace_isr_enter_user();
}

void sys_trace_isr_exit(void)
{
	sys_trace_isr_exit_user();
}

void sys_trace_idle(void)
{
	sys_trace_idle_user();

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_enter_idle();
	}
}

void sys_trace_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_exit_idle();
	}
}

void sys_trace_sys_init_enter(const struct init_entry *entry, int level)
{
	sys_trace_sys_init_enter_user(entry, level);
}

void sys_trace_sys_init_exit(const struct init_entry *entry, int level, int result)
{
	sys_trace_sys_init_exit_user(entry, level, result);
}

void sys_trace_k_thread_sleep_enter(k_timeout_t timeout)
{
	sys_trace_k_thread_sleep_enter_user(timeout);
}

void sys_trace_k_thread_sleep_exit(k_timeout_t timeout, int32_t ret)
{
	sys_trace_k_thread_sleep_exit_user(timeout, ret);
}

void sys_trace_k_thread_msleep_enter(int32_t ms)
{
	sys_trace_k_thread_msleep_enter_user(ms);
}

void sys_trace_k_thread_msleep_exit(int32_t ms, int32_t ret)
{
	sys_trace_k_thread_msleep_exit_user(ms, ret);
}

void sys_trace_k_thread_usleep_enter(int32_t us)
{
	sys_trace_k_thread_usleep_enter_user(us);
}

void sys_trace_k_thread_usleep_exit(int32_t us, int32_t ret)
{
	sys_trace_k_thread_usleep_exit_user(us, ret);
}

void sys_trace_gpio_pin_interrupt_configure_enter(const struct device *port, gpio_pin_t pin,
						  gpio_flags_t flags)
{
	sys_trace_gpio_pin_interrupt_configure_enter_user(port, pin, flags);
}

void sys_trace_gpio_pin_interrupt_configure_exit(const struct device *port, gpio_pin_t pin,
						  int ret)
{
	sys_trace_gpio_pin_interrupt_configure_exit_user(port, pin, ret);
}

void sys_trace_gpio_pin_configure_enter(const struct device *port, gpio_pin_t pin,
					gpio_flags_t flags)
{
	sys_trace_gpio_pin_configure_enter_user(port, pin, flags);
}

void sys_trace_gpio_pin_configure_exit(const struct device *port, gpio_pin_t pin, int ret)
{
	sys_trace_gpio_pin_configure_exit_user(port, pin, ret);
}

void sys_trace_gpio_port_get_direction_enter(const struct device *port, gpio_port_pins_t map,
					     gpio_port_pins_t inputs, gpio_port_pins_t outputs)
{
	sys_trace_gpio_port_get_direction_enter_user(port, map, inputs, outputs);
}

void sys_trace_gpio_port_get_direction_exit(const struct device *port, int ret)
{
	sys_trace_gpio_port_get_direction_exit_user(port, ret);
}

void sys_trace_gpio_pin_get_config_enter(const struct device *port, gpio_pin_t pin, int ret)
{
	sys_trace_gpio_pin_get_config_enter_user(port, pin, ret);
}

void sys_trace_gpio_pin_get_config_exit(const struct device *port, gpio_pin_t pin, int ret)
{
	sys_trace_gpio_pin_get_config_exit_user(port, pin, ret);
}

void sys_trace_gpio_port_get_raw_enter(const struct device *port, gpio_port_value_t *value)
{
	sys_trace_gpio_port_get_raw_enter_user(port, value);
}

void sys_trace_gpio_port_get_raw_exit(const struct device *port, int ret)
{
	sys_trace_gpio_port_get_raw_exit_user(port, ret);
}

void sys_trace_gpio_port_set_masked_raw_enter(const struct device *port, gpio_port_pins_t mask,
					      gpio_port_value_t value)
{
	sys_trace_gpio_port_set_masked_raw_enter_user(port, mask, value);
}

void sys_trace_gpio_port_set_masked_raw_exit(const struct device *port, int ret)
{
	sys_trace_gpio_port_set_masked_raw_exit_user(port, ret);
}

void sys_trace_gpio_port_set_bits_raw_enter(const struct device *port, gpio_port_pins_t pins)
{
	sys_trace_gpio_port_set_bits_raw_enter_user(port, pins);
}

void sys_trace_gpio_port_set_bits_raw_exit(const struct device *port, int ret)
{
	sys_trace_gpio_port_set_bits_raw_exit_user(port, ret);
}

void sys_trace_gpio_port_clear_bits_raw_enter(const struct device *port, gpio_port_pins_t pins)
{
	sys_trace_gpio_port_clear_bits_raw_enter_user(port, pins);
}

void sys_trace_gpio_port_clear_bits_raw_exit(const struct device *port, int ret)
{
	sys_trace_gpio_port_clear_bits_raw_exit_user(port, ret);
}

void sys_trace_gpio_port_toggle_bits_enter(const struct device *port, gpio_port_pins_t pins)
{
	sys_trace_gpio_port_toggle_bits_enter_user(port, pins);
}

void sys_trace_gpio_port_toggle_bits_exit(const struct device *port, int ret)
{
	sys_trace_gpio_port_toggle_bits_exit_user(port, ret);
}

void sys_trace_gpio_init_callback_enter(struct gpio_callback *callback,
					gpio_callback_handler_t handler, gpio_port_pins_t pin_mask)
{
	sys_trace_gpio_init_callback_enter_user(callback, handler, pin_mask);
}

void sys_trace_gpio_init_callback_exit(struct gpio_callback *callback)
{
	sys_trace_gpio_init_callback_exit_user(callback);
}

void sys_trace_gpio_add_callback_enter(const struct device *port, struct gpio_callback *callback)
{
	sys_trace_gpio_add_callback_enter_user(port, callback);
}

void sys_trace_gpio_add_callback_exit(const struct device *port, int ret)
{
	sys_trace_gpio_add_callback_exit_user(port, ret);
}

void sys_trace_gpio_remove_callback_enter(const struct device *port,
						struct gpio_callback *callback)
{
	sys_trace_gpio_remove_callback_enter_user(port, callback);
}

void sys_trace_gpio_remove_callback_exit(const struct device *port, int ret)
{
	sys_trace_gpio_remove_callback_exit_user(port, ret);
}

void sys_trace_gpio_get_pending_int_enter(const struct device *dev)
{
	sys_trace_gpio_get_pending_int_enter_user(dev);
}

void sys_trace_gpio_get_pending_int_exit(const struct device *dev, int ret)
{
	sys_trace_gpio_get_pending_int_exit_user(dev, ret);
}

void sys_trace_gpio_fire_callbacks_enter(sys_slist_t *list, const struct device *port,
					 gpio_pin_t pins)
{
	sys_trace_gpio_fire_callbacks_enter_user(list, port, pins);
}

void sys_trace_gpio_fire_callback(const struct device *port, struct gpio_callback *callback)
{
	sys_trace_gpio_fire_callback_user(port, callback);
}

void sys_trace_rtio_submit_enter(const struct rtio *r, uint32_t wait_count)
{
	sys_trace_rtio_submit_enter_user(r, wait_count);
}

void sys_trace_rtio_submit_exit(const struct rtio *r)
{
	sys_trace_rtio_submit_exit_user(r);
}

void sys_trace_rtio_sqe_acquire_enter(const struct rtio *r)
{
	sys_trace_rtio_sqe_acquire_enter_user(r);
}

void sys_trace_rtio_sqe_acquire_exit(const struct rtio *r, const struct rtio_sqe *sqe)
{
	sys_trace_rtio_sqe_acquire_exit_user(r, sqe);
}

void sys_trace_rtio_sqe_cancel(const struct rtio_sqe *sqe)
{
	sys_trace_rtio_sqe_cancel_user(sqe);
}

void sys_trace_rtio_cqe_submit_enter(const struct rtio *r, int result, uint32_t flags)
{
	sys_trace_rtio_cqe_submit_enter_user(r, result, flags);
}

void sys_trace_rtio_cqe_submit_exit(const struct rtio *r)
{
	sys_trace_rtio_cqe_submit_exit_user(r);
}

void sys_trace_rtio_cqe_acquire_enter(const struct rtio *r)
{
	sys_trace_rtio_cqe_acquire_enter_user(r);
}

void sys_trace_rtio_cqe_acquire_exit(const struct rtio *r, const struct rtio_cqe *cqe)
{
	sys_trace_rtio_cqe_acquire_exit_user(r, cqe);
}

void sys_trace_rtio_cqe_release(const struct rtio *r, const struct rtio_cqe *cqe)
{
	sys_trace_rtio_cqe_release_user(r, cqe);
}

void sys_trace_rtio_cqe_consume_enter(const struct rtio *r)
{
	sys_trace_rtio_cqe_consume_enter_user(r);
}

void sys_trace_rtio_cqe_consume_exit(const struct rtio *r, const struct rtio_cqe *cqe)
{
	sys_trace_rtio_cqe_consume_exit_user(r, cqe);
}

void sys_trace_rtio_txn_next_enter(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	sys_trace_rtio_txn_next_enter_user(r, iodev_sqe);
}

void sys_trace_rtio_txn_next_exit(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	sys_trace_rtio_txn_next_exit_user(r, iodev_sqe);
}

void sys_trace_rtio_chain_next_enter(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	sys_trace_rtio_chain_next_enter_user(r, iodev_sqe);
}

void sys_trace_rtio_chain_next_exit(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	sys_trace_rtio_chain_next_exit_user(r, iodev_sqe);
}

void sys_trace_timer_init(struct k_timer *timer)
{
	sys_trace_timer_init_user(timer);
}

void sys_trace_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period)
{
	sys_trace_timer_start_user(timer, duration, period);
}

void sys_trace_timer_stop(struct k_timer *timer)
{
	sys_trace_timer_stop_user(timer);
}

void sys_trace_timer_status_sync_enter(struct k_timer *timer)
{
	sys_trace_timer_status_sync_enter_user(timer);
}

void sys_trace_timer_status_sync_blocking(struct k_timer *timer, k_timeout_t timeout)
{
	sys_trace_timer_status_sync_blocking_user(timer, timeout);
}

void sys_trace_timer_status_sync_exit(struct k_timer *timer, uint32_t result)
{
	sys_trace_timer_status_sync_exit_user(timer, result);
}

void sys_trace_timer_expiry_enter(struct k_timer *timer)
{
	sys_trace_timer_expiry_enter_user(timer);
}

void sys_trace_timer_expiry_exit(struct k_timer *timer)
{
	sys_trace_timer_expiry_exit_user(timer);
}

void sys_trace_timer_stop_fn_expiry_enter(struct k_timer *timer)
{
	sys_trace_timer_stop_fn_expiry_enter_user(timer);
}

void sys_trace_timer_stop_fn_expiry_exit(struct k_timer *timer)
{
	sys_trace_timer_stop_fn_expiry_exit_user(timer);
}

/*
 * Kernel-object hook trampolines and their weak user callbacks (see the
 * matching declarations in tracing_user.h).
 */

/* k_condvar */
void __weak sys_trace_k_condvar_broadcast_enter_user(struct k_condvar *condvar) {}
void sys_trace_k_condvar_broadcast_enter(struct k_condvar *condvar)
{
	sys_trace_k_condvar_broadcast_enter_user(condvar);
}
void __weak sys_trace_k_condvar_broadcast_exit_user(struct k_condvar *condvar, int ret) {}
void sys_trace_k_condvar_broadcast_exit(struct k_condvar *condvar, int ret)
{
	sys_trace_k_condvar_broadcast_exit_user(condvar, ret);
}
void __weak sys_trace_k_condvar_init_user(struct k_condvar *condvar, int ret) {}
void sys_trace_k_condvar_init(struct k_condvar *condvar, int ret)
{
	sys_trace_k_condvar_init_user(condvar, ret);
}
void __weak sys_trace_k_condvar_signal_blocking_user(struct k_condvar *condvar, k_timeout_t timeout)
{
}
void sys_trace_k_condvar_signal_blocking(struct k_condvar *condvar, k_timeout_t timeout)
{
	sys_trace_k_condvar_signal_blocking_user(condvar, timeout);
}
void __weak sys_trace_k_condvar_signal_enter_user(struct k_condvar *condvar) {}
void sys_trace_k_condvar_signal_enter(struct k_condvar *condvar)
{
	sys_trace_k_condvar_signal_enter_user(condvar);
}
void __weak sys_trace_k_condvar_signal_exit_user(struct k_condvar *condvar, int ret) {}
void sys_trace_k_condvar_signal_exit(struct k_condvar *condvar, int ret)
{
	sys_trace_k_condvar_signal_exit_user(condvar, ret);
}
void __weak sys_trace_k_condvar_wait_enter_user(struct k_condvar *condvar, k_timeout_t timeout) {}
void sys_trace_k_condvar_wait_enter(struct k_condvar *condvar, k_timeout_t timeout)
{
	sys_trace_k_condvar_wait_enter_user(condvar, timeout);
}
void __weak sys_trace_k_condvar_wait_exit_user(struct k_condvar *condvar, k_timeout_t timeout,
					       int ret)
{
}
void sys_trace_k_condvar_wait_exit(struct k_condvar *condvar, k_timeout_t timeout, int ret)
{
	sys_trace_k_condvar_wait_exit_user(condvar, timeout, ret);
}

/* k_event */
void __weak sys_trace_k_event_init_user(struct k_event *event) {}
void sys_trace_k_event_init(struct k_event *event)
{
	sys_trace_k_event_init_user(event);
}
void __weak sys_trace_k_event_post_enter_user(struct k_event *event, uint32_t events,
					      uint32_t events_mask)
{
}
void sys_trace_k_event_post_enter(struct k_event *event, uint32_t events, uint32_t events_mask)
{
	sys_trace_k_event_post_enter_user(event, events, events_mask);
}
void __weak sys_trace_k_event_post_exit_user(struct k_event *event, uint32_t events,
					     uint32_t events_mask)
{
}
void sys_trace_k_event_post_exit(struct k_event *event, uint32_t events, uint32_t events_mask)
{
	sys_trace_k_event_post_exit_user(event, events, events_mask);
}
void __weak sys_trace_k_event_wait_blocking_user(struct k_event *event, uint32_t events,
						 uint32_t options, k_timeout_t timeout)
{
}
void sys_trace_k_event_wait_blocking(struct k_event *event, uint32_t events, uint32_t options,
				     k_timeout_t timeout)
{
	sys_trace_k_event_wait_blocking_user(event, events, options, timeout);
}
void __weak sys_trace_k_event_wait_enter_user(struct k_event *event, uint32_t events,
					      uint32_t options, k_timeout_t timeout)
{
}
void sys_trace_k_event_wait_enter(struct k_event *event, uint32_t events, uint32_t options,
				  k_timeout_t timeout)
{
	sys_trace_k_event_wait_enter_user(event, events, options, timeout);
}
void __weak sys_trace_k_event_wait_exit_user(struct k_event *event, uint32_t events, int ret) {}
void sys_trace_k_event_wait_exit(struct k_event *event, uint32_t events, int ret)
{
	sys_trace_k_event_wait_exit_user(event, events, ret);
}

/* k_mbox */
void __weak sys_trace_k_mbox_async_put_enter_user(struct k_mbox *mbox, struct k_sem *sem) {}
void sys_trace_k_mbox_async_put_enter(struct k_mbox *mbox, struct k_sem *sem)
{
	sys_trace_k_mbox_async_put_enter_user(mbox, sem);
}
void __weak sys_trace_k_mbox_async_put_exit_user(struct k_mbox *mbox, struct k_sem *sem) {}
void sys_trace_k_mbox_async_put_exit(struct k_mbox *mbox, struct k_sem *sem)
{
	sys_trace_k_mbox_async_put_exit_user(mbox, sem);
}
void __weak sys_trace_k_mbox_data_get_user(struct k_mbox_msg *rx_msg) {}
void sys_trace_k_mbox_data_get(struct k_mbox_msg *rx_msg)
{
	sys_trace_k_mbox_data_get_user(rx_msg);
}
void __weak sys_trace_k_mbox_get_blocking_user(struct k_mbox *mbox, k_timeout_t timeout) {}
void sys_trace_k_mbox_get_blocking(struct k_mbox *mbox, k_timeout_t timeout)
{
	sys_trace_k_mbox_get_blocking_user(mbox, timeout);
}
void __weak sys_trace_k_mbox_get_enter_user(struct k_mbox *mbox, k_timeout_t timeout) {}
void sys_trace_k_mbox_get_enter(struct k_mbox *mbox, k_timeout_t timeout)
{
	sys_trace_k_mbox_get_enter_user(mbox, timeout);
}
void __weak sys_trace_k_mbox_get_exit_user(struct k_mbox *mbox, k_timeout_t timeout, int ret) {}
void sys_trace_k_mbox_get_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret)
{
	sys_trace_k_mbox_get_exit_user(mbox, timeout, ret);
}
void __weak sys_trace_k_mbox_init_user(struct k_mbox *mbox) {}
void sys_trace_k_mbox_init(struct k_mbox *mbox)
{
	sys_trace_k_mbox_init_user(mbox);
}
void __weak sys_trace_k_mbox_message_put_blocking_user(struct k_mbox *mbox, k_timeout_t timeout) {}
void sys_trace_k_mbox_message_put_blocking(struct k_mbox *mbox, k_timeout_t timeout)
{
	sys_trace_k_mbox_message_put_blocking_user(mbox, timeout);
}
void __weak sys_trace_k_mbox_message_put_enter_user(struct k_mbox *mbox, k_timeout_t timeout) {}
void sys_trace_k_mbox_message_put_enter(struct k_mbox *mbox, k_timeout_t timeout)
{
	sys_trace_k_mbox_message_put_enter_user(mbox, timeout);
}
void __weak sys_trace_k_mbox_message_put_exit_user(struct k_mbox *mbox, k_timeout_t timeout,
						   int ret)
{
}
void sys_trace_k_mbox_message_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret)
{
	sys_trace_k_mbox_message_put_exit_user(mbox, timeout, ret);
}
void __weak sys_trace_k_mbox_put_enter_user(struct k_mbox *mbox, k_timeout_t timeout) {}
void sys_trace_k_mbox_put_enter(struct k_mbox *mbox, k_timeout_t timeout)
{
	sys_trace_k_mbox_put_enter_user(mbox, timeout);
}
void __weak sys_trace_k_mbox_put_exit_user(struct k_mbox *mbox, k_timeout_t timeout, int ret) {}
void sys_trace_k_mbox_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret)
{
	sys_trace_k_mbox_put_exit_user(mbox, timeout, ret);
}

/* k_mem */
void __weak sys_trace_k_mem_slab_alloc_blocking_user(struct k_mem_slab *slab, k_timeout_t timeout)
{
}
void sys_trace_k_mem_slab_alloc_blocking(struct k_mem_slab *slab, k_timeout_t timeout)
{
	sys_trace_k_mem_slab_alloc_blocking_user(slab, timeout);
}
void __weak sys_trace_k_mem_slab_alloc_enter_user(struct k_mem_slab *slab, k_timeout_t timeout) {}
void sys_trace_k_mem_slab_alloc_enter(struct k_mem_slab *slab, k_timeout_t timeout)
{
	sys_trace_k_mem_slab_alloc_enter_user(slab, timeout);
}
void __weak sys_trace_k_mem_slab_alloc_exit_user(struct k_mem_slab *slab, k_timeout_t timeout,
						 int ret)
{
}
void sys_trace_k_mem_slab_alloc_exit(struct k_mem_slab *slab, k_timeout_t timeout, int ret)
{
	sys_trace_k_mem_slab_alloc_exit_user(slab, timeout, ret);
}
void __weak sys_trace_k_mem_slab_free_enter_user(struct k_mem_slab *slab) {}
void sys_trace_k_mem_slab_free_enter(struct k_mem_slab *slab)
{
	sys_trace_k_mem_slab_free_enter_user(slab);
}
void __weak sys_trace_k_mem_slab_free_exit_user(struct k_mem_slab *slab) {}
void sys_trace_k_mem_slab_free_exit(struct k_mem_slab *slab)
{
	sys_trace_k_mem_slab_free_exit_user(slab);
}
void __weak sys_trace_k_mem_slab_init_user(struct k_mem_slab *slab, int ret) {}
void sys_trace_k_mem_slab_init(struct k_mem_slab *slab, int ret)
{
	sys_trace_k_mem_slab_init_user(slab, ret);
}

/* k_msgq */
void __weak sys_trace_k_msgq_alloc_init_enter_user(struct k_msgq *msgq) {}
void sys_trace_k_msgq_alloc_init_enter(struct k_msgq *msgq)
{
	sys_trace_k_msgq_alloc_init_enter_user(msgq);
}
void __weak sys_trace_k_msgq_alloc_init_exit_user(struct k_msgq *msgq, int ret) {}
void sys_trace_k_msgq_alloc_init_exit(struct k_msgq *msgq, int ret)
{
	sys_trace_k_msgq_alloc_init_exit_user(msgq, ret);
}
void __weak sys_trace_k_msgq_cleanup_enter_user(struct k_msgq *msgq) {}
void sys_trace_k_msgq_cleanup_enter(struct k_msgq *msgq)
{
	sys_trace_k_msgq_cleanup_enter_user(msgq);
}
void __weak sys_trace_k_msgq_cleanup_exit_user(struct k_msgq *msgq, int ret) {}
void sys_trace_k_msgq_cleanup_exit(struct k_msgq *msgq, int ret)
{
	sys_trace_k_msgq_cleanup_exit_user(msgq, ret);
}
void __weak sys_trace_k_msgq_get_blocking_user(struct k_msgq *msgq, k_timeout_t timeout) {}
void sys_trace_k_msgq_get_blocking(struct k_msgq *msgq, k_timeout_t timeout)
{
	sys_trace_k_msgq_get_blocking_user(msgq, timeout);
}
void __weak sys_trace_k_msgq_get_enter_user(struct k_msgq *msgq, k_timeout_t timeout) {}
void sys_trace_k_msgq_get_enter(struct k_msgq *msgq, k_timeout_t timeout)
{
	sys_trace_k_msgq_get_enter_user(msgq, timeout);
}
void __weak sys_trace_k_msgq_get_exit_user(struct k_msgq *msgq, k_timeout_t timeout, int ret) {}
void sys_trace_k_msgq_get_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
	sys_trace_k_msgq_get_exit_user(msgq, timeout, ret);
}
void __weak sys_trace_k_msgq_init_user(struct k_msgq *msgq) {}
void sys_trace_k_msgq_init(struct k_msgq *msgq)
{
	sys_trace_k_msgq_init_user(msgq);
}
void __weak sys_trace_k_msgq_peek_user(struct k_msgq *msgq, int ret) {}
void sys_trace_k_msgq_peek(struct k_msgq *msgq, int ret)
{
	sys_trace_k_msgq_peek_user(msgq, ret);
}
void __weak sys_trace_k_msgq_purge_user(struct k_msgq *msgq) {}
void sys_trace_k_msgq_purge(struct k_msgq *msgq)
{
	sys_trace_k_msgq_purge_user(msgq);
}
void __weak sys_trace_k_msgq_put_blocking_user(struct k_msgq *msgq, k_timeout_t timeout) {}
void sys_trace_k_msgq_put_blocking(struct k_msgq *msgq, k_timeout_t timeout)
{
	sys_trace_k_msgq_put_blocking_user(msgq, timeout);
}
void __weak sys_trace_k_msgq_put_enter_user(struct k_msgq *msgq, k_timeout_t timeout) {}
void sys_trace_k_msgq_put_enter(struct k_msgq *msgq, k_timeout_t timeout)
{
	sys_trace_k_msgq_put_enter_user(msgq, timeout);
}
void __weak sys_trace_k_msgq_put_exit_user(struct k_msgq *msgq, k_timeout_t timeout, int ret) {}
void sys_trace_k_msgq_put_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
	sys_trace_k_msgq_put_exit_user(msgq, timeout, ret);
}
void __weak sys_trace_k_msgq_put_front_blocking_user(struct k_msgq *msgq, k_timeout_t timeout) {}
void sys_trace_k_msgq_put_front_blocking(struct k_msgq *msgq, k_timeout_t timeout)
{
	sys_trace_k_msgq_put_front_blocking_user(msgq, timeout);
}
void __weak sys_trace_k_msgq_put_front_enter_user(struct k_msgq *msgq, k_timeout_t timeout) {}
void sys_trace_k_msgq_put_front_enter(struct k_msgq *msgq, k_timeout_t timeout)
{
	sys_trace_k_msgq_put_front_enter_user(msgq, timeout);
}
void __weak sys_trace_k_msgq_put_front_exit_user(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
}
void sys_trace_k_msgq_put_front_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
	sys_trace_k_msgq_put_front_exit_user(msgq, timeout, ret);
}

/* k_mutex */
void __weak sys_trace_k_mutex_init_user(struct k_mutex *mutex, int ret) {}
void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret)
{
	sys_trace_k_mutex_init_user(mutex, ret);
}
void __weak sys_trace_k_mutex_lock_blocking_user(struct k_mutex *mutex, k_timeout_t timeout) {}
void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout)
{
	sys_trace_k_mutex_lock_blocking_user(mutex, timeout);
}
void __weak sys_trace_k_mutex_lock_enter_user(struct k_mutex *mutex, k_timeout_t timeout) {}
void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout)
{
	sys_trace_k_mutex_lock_enter_user(mutex, timeout);
}
void __weak sys_trace_k_mutex_lock_exit_user(struct k_mutex *mutex, k_timeout_t timeout, int ret) {}
void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret)
{
	sys_trace_k_mutex_lock_exit_user(mutex, timeout, ret);
}
void __weak sys_trace_k_mutex_unlock_enter_user(struct k_mutex *mutex) {}
void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex)
{
	sys_trace_k_mutex_unlock_enter_user(mutex);
}
void __weak sys_trace_k_mutex_unlock_exit_user(struct k_mutex *mutex, int ret) {}
void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret)
{
	sys_trace_k_mutex_unlock_exit_user(mutex, ret);
}

/* k_poll */
void __weak sys_trace_k_poll_api_event_init_user(struct k_poll_event *event) {}
void sys_trace_k_poll_api_event_init(struct k_poll_event *event)
{
	sys_trace_k_poll_api_event_init_user(event);
}
void __weak sys_trace_k_poll_api_poll_enter_user(struct k_poll_event *events) {}
void sys_trace_k_poll_api_poll_enter(struct k_poll_event *events)
{
	sys_trace_k_poll_api_poll_enter_user(events);
}
void __weak sys_trace_k_poll_api_poll_exit_user(struct k_poll_event *events, int ret) {}
void sys_trace_k_poll_api_poll_exit(struct k_poll_event *events, int ret)
{
	sys_trace_k_poll_api_poll_exit_user(events, ret);
}
void __weak sys_trace_k_poll_api_signal_check_user(struct k_poll_signal *sig) {}
void sys_trace_k_poll_api_signal_check(struct k_poll_signal *sig)
{
	sys_trace_k_poll_api_signal_check_user(sig);
}
void __weak sys_trace_k_poll_api_signal_init_user(struct k_poll_signal *sig) {}
void sys_trace_k_poll_api_signal_init(struct k_poll_signal *sig)
{
	sys_trace_k_poll_api_signal_init_user(sig);
}
void __weak sys_trace_k_poll_api_signal_raise_user(struct k_poll_signal *sig, int ret) {}
void sys_trace_k_poll_api_signal_raise(struct k_poll_signal *sig, int ret)
{
	sys_trace_k_poll_api_signal_raise_user(sig, ret);
}
void __weak sys_trace_k_poll_api_signal_reset_user(struct k_poll_signal *sig) {}
void sys_trace_k_poll_api_signal_reset(struct k_poll_signal *sig)
{
	sys_trace_k_poll_api_signal_reset_user(sig);
}

/* k_sem */
void __weak sys_trace_k_sem_give_enter_user(struct k_sem *sem) {}
void sys_trace_k_sem_give_enter(struct k_sem *sem)
{
	sys_trace_k_sem_give_enter_user(sem);
}
void __weak sys_trace_k_sem_give_exit_user(struct k_sem *sem) {}
void sys_trace_k_sem_give_exit(struct k_sem *sem)
{
	sys_trace_k_sem_give_exit_user(sem);
}
void __weak sys_trace_k_sem_init_user(struct k_sem *sem, int ret) {}
void sys_trace_k_sem_init(struct k_sem *sem, int ret)
{
	sys_trace_k_sem_init_user(sem, ret);
}
void __weak sys_trace_k_sem_reset_user(struct k_sem *sem) {}
void sys_trace_k_sem_reset(struct k_sem *sem)
{
	sys_trace_k_sem_reset_user(sem);
}
void __weak sys_trace_k_sem_take_blocking_user(struct k_sem *sem, k_timeout_t timeout) {}
void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout)
{
	sys_trace_k_sem_take_blocking_user(sem, timeout);
}
void __weak sys_trace_k_sem_take_enter_user(struct k_sem *sem, k_timeout_t timeout) {}
void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout)
{
	sys_trace_k_sem_take_enter_user(sem, timeout);
}
void __weak sys_trace_k_sem_take_exit_user(struct k_sem *sem, k_timeout_t timeout, int ret) {}
void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret)
{
	sys_trace_k_sem_take_exit_user(sem, timeout, ret);
}

/* k_thread */
void __weak sys_trace_k_thread_busy_wait_enter_user(uint32_t usec_to_wait) {}
void sys_trace_k_thread_busy_wait_enter(uint32_t usec_to_wait)
{
	sys_trace_k_thread_busy_wait_enter_user(usec_to_wait);
}
void __weak sys_trace_k_thread_busy_wait_exit_user(uint32_t usec_to_wait) {}
void sys_trace_k_thread_busy_wait_exit(uint32_t usec_to_wait)
{
	sys_trace_k_thread_busy_wait_exit_user(usec_to_wait);
}
void __weak sys_trace_k_thread_foreach_enter_user(void) {}
void sys_trace_k_thread_foreach_enter(void)
{
	sys_trace_k_thread_foreach_enter_user();
}
void __weak sys_trace_k_thread_foreach_exit_user(void) {}
void sys_trace_k_thread_foreach_exit(void)
{
	sys_trace_k_thread_foreach_exit_user();
}
void __weak sys_trace_k_thread_foreach_unlocked_enter_user(void) {}
void sys_trace_k_thread_foreach_unlocked_enter(void)
{
	sys_trace_k_thread_foreach_unlocked_enter_user();
}
void __weak sys_trace_k_thread_foreach_unlocked_exit_user(void) {}
void sys_trace_k_thread_foreach_unlocked_exit(void)
{
	sys_trace_k_thread_foreach_unlocked_exit_user();
}
void __weak sys_trace_k_thread_heap_assign_user(struct k_thread *thread, struct k_heap *heap) {}
void sys_trace_k_thread_heap_assign(struct k_thread *thread, struct k_heap *heap)
{
	sys_trace_k_thread_heap_assign_user(thread, heap);
}
void __weak sys_trace_k_thread_join_blocking_user(struct k_thread *thread, k_timeout_t timeout) {}
void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout)
{
	sys_trace_k_thread_join_blocking_user(thread, timeout);
}
void __weak sys_trace_k_thread_join_enter_user(struct k_thread *thread, k_timeout_t timeout) {}
void sys_trace_k_thread_join_enter(struct k_thread *thread, k_timeout_t timeout)
{
	sys_trace_k_thread_join_enter_user(thread, timeout);
}
void __weak sys_trace_k_thread_join_exit_user(struct k_thread *thread, k_timeout_t timeout, int ret)
{
}
void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret)
{
	sys_trace_k_thread_join_exit_user(thread, timeout, ret);
}
void __weak sys_trace_k_thread_sched_abort_user(struct k_thread *thread) {}
void sys_trace_k_thread_sched_abort(struct k_thread *thread)
{
	sys_trace_k_thread_sched_abort_user(thread);
}
void __weak sys_trace_k_thread_sched_lock_user(void) {}
void sys_trace_k_thread_sched_lock(void)
{
	sys_trace_k_thread_sched_lock_user();
}
void __weak sys_trace_k_thread_sched_resume_user(struct k_thread *thread) {}
void sys_trace_k_thread_sched_resume(struct k_thread *thread)
{
	sys_trace_k_thread_sched_resume_user(thread);
}
void __weak sys_trace_k_thread_sched_suspend_user(struct k_thread *thread) {}
void sys_trace_k_thread_sched_suspend(struct k_thread *thread)
{
	sys_trace_k_thread_sched_suspend_user(thread);
}
void __weak sys_trace_k_thread_sched_unlock_user(void) {}
void sys_trace_k_thread_sched_unlock(void)
{
	sys_trace_k_thread_sched_unlock_user();
}
void __weak sys_trace_k_thread_sched_wakeup_user(struct k_thread *thread) {}
void sys_trace_k_thread_sched_wakeup(struct k_thread *thread)
{
	sys_trace_k_thread_sched_wakeup_user(thread);
}
void __weak sys_trace_k_thread_start_user(struct k_thread *thread) {}
void sys_trace_k_thread_start(struct k_thread *thread)
{
	sys_trace_k_thread_start_user(thread);
}
void __weak sys_trace_k_thread_suspend_exit_user(struct k_thread *thread) {}
void sys_trace_k_thread_suspend_exit(struct k_thread *thread)
{
	sys_trace_k_thread_suspend_exit_user(thread);
}
void __weak sys_trace_k_thread_user_mode_enter_user(void) {}
void sys_trace_k_thread_user_mode_enter(void)
{
	sys_trace_k_thread_user_mode_enter_user();
}
void __weak sys_trace_k_thread_wakeup_user(struct k_thread *thread) {}
void sys_trace_k_thread_wakeup(struct k_thread *thread)
{
	sys_trace_k_thread_wakeup_user(thread);
}
void __weak sys_trace_k_thread_yield_user(void) {}
void sys_trace_k_thread_yield(void)
{
	sys_trace_k_thread_yield_user();
}

/* k_work */
void __weak sys_trace_k_work_cancel_delayable_enter_user(struct k_work_delayable *dwork) {}
void sys_trace_k_work_cancel_delayable_enter(struct k_work_delayable *dwork)
{
	sys_trace_k_work_cancel_delayable_enter_user(dwork);
}
void __weak sys_trace_k_work_cancel_delayable_exit_user(struct k_work_delayable *dwork, int ret) {}
void sys_trace_k_work_cancel_delayable_exit(struct k_work_delayable *dwork, int ret)
{
	sys_trace_k_work_cancel_delayable_exit_user(dwork, ret);
}
void __weak sys_trace_k_work_cancel_delayable_sync_enter_user(struct k_work_delayable *dwork,
							      struct k_work_sync *sync)
{
}
void sys_trace_k_work_cancel_delayable_sync_enter(struct k_work_delayable *dwork,
						  struct k_work_sync *sync)
{
	sys_trace_k_work_cancel_delayable_sync_enter_user(dwork, sync);
}
void __weak sys_trace_k_work_cancel_delayable_sync_exit_user(struct k_work_delayable *dwork,
							     struct k_work_sync *sync, int ret)
{
}
void sys_trace_k_work_cancel_delayable_sync_exit(struct k_work_delayable *dwork,
						 struct k_work_sync *sync, int ret)
{
	sys_trace_k_work_cancel_delayable_sync_exit_user(dwork, sync, ret);
}
void __weak sys_trace_k_work_cancel_enter_user(struct k_work *work) {}
void sys_trace_k_work_cancel_enter(struct k_work *work)
{
	sys_trace_k_work_cancel_enter_user(work);
}
void __weak sys_trace_k_work_cancel_exit_user(struct k_work *work, int ret) {}
void sys_trace_k_work_cancel_exit(struct k_work *work, int ret)
{
	sys_trace_k_work_cancel_exit_user(work, ret);
}
void __weak sys_trace_k_work_cancel_sync_blocking_user(struct k_work *work,
						       struct k_work_sync *sync)
{
}
void sys_trace_k_work_cancel_sync_blocking(struct k_work *work, struct k_work_sync *sync)
{
	sys_trace_k_work_cancel_sync_blocking_user(work, sync);
}
void __weak sys_trace_k_work_cancel_sync_enter_user(struct k_work *work, struct k_work_sync *sync)
{
}
void sys_trace_k_work_cancel_sync_enter(struct k_work *work, struct k_work_sync *sync)
{
	sys_trace_k_work_cancel_sync_enter_user(work, sync);
}
void __weak sys_trace_k_work_cancel_sync_exit_user(struct k_work *work, struct k_work_sync *sync,
						   int ret)
{
}
void sys_trace_k_work_cancel_sync_exit(struct k_work *work, struct k_work_sync *sync, int ret)
{
	sys_trace_k_work_cancel_sync_exit_user(work, sync, ret);
}
void __weak sys_trace_k_work_delayable_init_user(struct k_work_delayable *dwork) {}
void sys_trace_k_work_delayable_init(struct k_work_delayable *dwork)
{
	sys_trace_k_work_delayable_init_user(dwork);
}
void __weak sys_trace_k_work_flush_blocking_user(struct k_work *work, k_timeout_t timeout) {}
void sys_trace_k_work_flush_blocking(struct k_work *work, k_timeout_t timeout)
{
	sys_trace_k_work_flush_blocking_user(work, timeout);
}
void __weak sys_trace_k_work_flush_delayable_enter_user(struct k_work_delayable *dwork,
							struct k_work_sync *sync)
{
}
void sys_trace_k_work_flush_delayable_enter(struct k_work_delayable *dwork,
					    struct k_work_sync *sync)
{
	sys_trace_k_work_flush_delayable_enter_user(dwork, sync);
}
void __weak sys_trace_k_work_flush_delayable_exit_user(struct k_work_delayable *dwork,
						       struct k_work_sync *sync, int ret)
{
}
void sys_trace_k_work_flush_delayable_exit(struct k_work_delayable *dwork, struct k_work_sync *sync,
					   int ret)
{
	sys_trace_k_work_flush_delayable_exit_user(dwork, sync, ret);
}
void __weak sys_trace_k_work_flush_enter_user(struct k_work *work) {}
void sys_trace_k_work_flush_enter(struct k_work *work)
{
	sys_trace_k_work_flush_enter_user(work);
}
void __weak sys_trace_k_work_flush_exit_user(struct k_work *work, int ret) {}
void sys_trace_k_work_flush_exit(struct k_work *work, int ret)
{
	sys_trace_k_work_flush_exit_user(work, ret);
}
void __weak sys_trace_k_work_init_user(struct k_work *work) {}
void sys_trace_k_work_init(struct k_work *work)
{
	sys_trace_k_work_init_user(work);
}
void __weak sys_trace_k_work_poll_cancel_enter_user(struct k_work_poll *work) {}
void sys_trace_k_work_poll_cancel_enter(struct k_work_poll *work)
{
	sys_trace_k_work_poll_cancel_enter_user(work);
}
void __weak sys_trace_k_work_poll_cancel_exit_user(struct k_work_poll *work, int ret) {}
void sys_trace_k_work_poll_cancel_exit(struct k_work_poll *work, int ret)
{
	sys_trace_k_work_poll_cancel_exit_user(work, ret);
}
void __weak sys_trace_k_work_poll_init_enter_user(struct k_work_poll *work) {}
void sys_trace_k_work_poll_init_enter(struct k_work_poll *work)
{
	sys_trace_k_work_poll_init_enter_user(work);
}
void __weak sys_trace_k_work_poll_init_exit_user(struct k_work_poll *work) {}
void sys_trace_k_work_poll_init_exit(struct k_work_poll *work)
{
	sys_trace_k_work_poll_init_exit_user(work);
}
void __weak sys_trace_k_work_poll_submit_enter_user(struct k_work_poll *work, k_timeout_t timeout)
{
}
void sys_trace_k_work_poll_submit_enter(struct k_work_poll *work, k_timeout_t timeout)
{
	sys_trace_k_work_poll_submit_enter_user(work, timeout);
}
void __weak sys_trace_k_work_poll_submit_exit_user(struct k_work_poll *work, k_timeout_t timeout,
						   int ret)
{
}
void sys_trace_k_work_poll_submit_exit(struct k_work_poll *work, k_timeout_t timeout, int ret)
{
	sys_trace_k_work_poll_submit_exit_user(work, timeout, ret);
}
void __weak sys_trace_k_work_poll_submit_to_queue_blocking_user(struct k_work_q *work_q,
								struct k_work_poll *work,
								k_timeout_t timeout)
{
}
void sys_trace_k_work_poll_submit_to_queue_blocking(struct k_work_q *work_q,
						    struct k_work_poll *work, k_timeout_t timeout)
{
	sys_trace_k_work_poll_submit_to_queue_blocking_user(work_q, work, timeout);
}
void __weak sys_trace_k_work_poll_submit_to_queue_enter_user(struct k_work_q *work_q,
							     struct k_work_poll *work,
							     k_timeout_t timeout)
{
}
void sys_trace_k_work_poll_submit_to_queue_enter(struct k_work_q *work_q, struct k_work_poll *work,
						 k_timeout_t timeout)
{
	sys_trace_k_work_poll_submit_to_queue_enter_user(work_q, work, timeout);
}
void __weak sys_trace_k_work_poll_submit_to_queue_exit_user(struct k_work_q *work_q,
							    struct k_work_poll *work,
							    k_timeout_t timeout, int ret)
{
}
void sys_trace_k_work_poll_submit_to_queue_exit(struct k_work_q *work_q, struct k_work_poll *work,
						k_timeout_t timeout, int ret)
{
	sys_trace_k_work_poll_submit_to_queue_exit_user(work_q, work, timeout, ret);
}
void __weak sys_trace_k_work_queue_drain_enter_user(struct k_work_q *queue) {}
void sys_trace_k_work_queue_drain_enter(struct k_work_q *queue)
{
	sys_trace_k_work_queue_drain_enter_user(queue);
}
void __weak sys_trace_k_work_queue_drain_exit_user(struct k_work_q *queue, int ret) {}
void sys_trace_k_work_queue_drain_exit(struct k_work_q *queue, int ret)
{
	sys_trace_k_work_queue_drain_exit_user(queue, ret);
}
void __weak sys_trace_k_work_queue_init_user(struct k_work_q *queue) {}
void sys_trace_k_work_queue_init(struct k_work_q *queue)
{
	sys_trace_k_work_queue_init_user(queue);
}
void __weak sys_trace_k_work_queue_start_enter_user(struct k_work_q *queue) {}
void sys_trace_k_work_queue_start_enter(struct k_work_q *queue)
{
	sys_trace_k_work_queue_start_enter_user(queue);
}
void __weak sys_trace_k_work_queue_start_exit_user(struct k_work_q *queue) {}
void sys_trace_k_work_queue_start_exit(struct k_work_q *queue)
{
	sys_trace_k_work_queue_start_exit_user(queue);
}
void __weak sys_trace_k_work_queue_stop_blocking_user(struct k_work_q *queue, k_timeout_t timeout)
{
}
void sys_trace_k_work_queue_stop_blocking(struct k_work_q *queue, k_timeout_t timeout)
{
	sys_trace_k_work_queue_stop_blocking_user(queue, timeout);
}
void __weak sys_trace_k_work_queue_stop_enter_user(struct k_work_q *queue, k_timeout_t timeout) {}
void sys_trace_k_work_queue_stop_enter(struct k_work_q *queue, k_timeout_t timeout)
{
	sys_trace_k_work_queue_stop_enter_user(queue, timeout);
}
void __weak sys_trace_k_work_queue_stop_exit_user(struct k_work_q *queue, k_timeout_t timeout,
						  int ret)
{
}
void sys_trace_k_work_queue_stop_exit(struct k_work_q *queue, k_timeout_t timeout, int ret)
{
	sys_trace_k_work_queue_stop_exit_user(queue, timeout, ret);
}
void __weak sys_trace_k_work_queue_unplug_enter_user(struct k_work_q *queue) {}
void sys_trace_k_work_queue_unplug_enter(struct k_work_q *queue)
{
	sys_trace_k_work_queue_unplug_enter_user(queue);
}
void __weak sys_trace_k_work_queue_unplug_exit_user(struct k_work_q *queue, int ret) {}
void sys_trace_k_work_queue_unplug_exit(struct k_work_q *queue, int ret)
{
	sys_trace_k_work_queue_unplug_exit_user(queue, ret);
}
void __weak sys_trace_k_work_reschedule_enter_user(struct k_work_delayable *dwork,
						   k_timeout_t delay)
{
}
void sys_trace_k_work_reschedule_enter(struct k_work_delayable *dwork, k_timeout_t delay)
{
	sys_trace_k_work_reschedule_enter_user(dwork, delay);
}
void __weak sys_trace_k_work_reschedule_exit_user(struct k_work_delayable *dwork, k_timeout_t delay,
						  int ret)
{
}
void sys_trace_k_work_reschedule_exit(struct k_work_delayable *dwork, k_timeout_t delay, int ret)
{
	sys_trace_k_work_reschedule_exit_user(dwork, delay, ret);
}
void __weak sys_trace_k_work_reschedule_for_queue_enter_user(struct k_work_q *queue,
							     struct k_work_delayable *dwork,
							     k_timeout_t delay)
{
}
void sys_trace_k_work_reschedule_for_queue_enter(struct k_work_q *queue,
						 struct k_work_delayable *dwork, k_timeout_t delay)
{
	sys_trace_k_work_reschedule_for_queue_enter_user(queue, dwork, delay);
}
void __weak sys_trace_k_work_reschedule_for_queue_exit_user(struct k_work_q *queue,
							    struct k_work_delayable *dwork,
							    k_timeout_t delay, int ret)
{
}
void sys_trace_k_work_reschedule_for_queue_exit(struct k_work_q *queue,
						struct k_work_delayable *dwork, k_timeout_t delay,
						int ret)
{
	sys_trace_k_work_reschedule_for_queue_exit_user(queue, dwork, delay, ret);
}
void __weak sys_trace_k_work_schedule_enter_user(struct k_work_delayable *dwork, k_timeout_t delay)
{
}
void sys_trace_k_work_schedule_enter(struct k_work_delayable *dwork, k_timeout_t delay)
{
	sys_trace_k_work_schedule_enter_user(dwork, delay);
}
void __weak sys_trace_k_work_schedule_exit_user(struct k_work_delayable *dwork, k_timeout_t delay,
						int ret)
{
}
void sys_trace_k_work_schedule_exit(struct k_work_delayable *dwork, k_timeout_t delay, int ret)
{
	sys_trace_k_work_schedule_exit_user(dwork, delay, ret);
}
void __weak sys_trace_k_work_schedule_for_queue_enter_user(struct k_work_q *queue,
							   struct k_work_delayable *dwork,
							   k_timeout_t delay)
{
}
void sys_trace_k_work_schedule_for_queue_enter(struct k_work_q *queue,
					       struct k_work_delayable *dwork, k_timeout_t delay)
{
	sys_trace_k_work_schedule_for_queue_enter_user(queue, dwork, delay);
}
void __weak sys_trace_k_work_schedule_for_queue_exit_user(struct k_work_q *queue,
							  struct k_work_delayable *dwork,
							  k_timeout_t delay, int ret)
{
}
void sys_trace_k_work_schedule_for_queue_exit(struct k_work_q *queue,
					      struct k_work_delayable *dwork, k_timeout_t delay,
					      int ret)
{
	sys_trace_k_work_schedule_for_queue_exit_user(queue, dwork, delay, ret);
}
void __weak sys_trace_k_work_submit_enter_user(struct k_work *work) {}
void sys_trace_k_work_submit_enter(struct k_work *work)
{
	sys_trace_k_work_submit_enter_user(work);
}
void __weak sys_trace_k_work_submit_exit_user(struct k_work *work, int ret) {}
void sys_trace_k_work_submit_exit(struct k_work *work, int ret)
{
	sys_trace_k_work_submit_exit_user(work, ret);
}
void __weak sys_trace_k_work_submit_to_queue_enter_user(struct k_work_q *queue, struct k_work *work)
{
}
void sys_trace_k_work_submit_to_queue_enter(struct k_work_q *queue, struct k_work *work)
{
	sys_trace_k_work_submit_to_queue_enter_user(queue, work);
}
void __weak sys_trace_k_work_submit_to_queue_exit_user(struct k_work_q *queue, struct k_work *work,
						       int ret)
{
}
void sys_trace_k_work_submit_to_queue_exit(struct k_work_q *queue, struct k_work *work, int ret)
{
	sys_trace_k_work_submit_to_queue_exit_user(queue, work, ret);
}
