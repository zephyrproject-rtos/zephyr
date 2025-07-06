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
