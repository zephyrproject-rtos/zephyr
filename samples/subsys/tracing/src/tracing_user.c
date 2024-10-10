/*
 * Copyright (c) 2020 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

static int nested_interrupts[CONFIG_MP_MAX_NUM_CPUS];

void sys_trace_thread_switched_in_user(void)
{
	unsigned int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[_current_cpu->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	printk("%s: %p\n", __func__, k_sched_current_thread_query());

	irq_unlock(key);
}

void sys_trace_thread_switched_out_user(void)
{
	unsigned int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts[_current_cpu->id] == 0);
	/* Can't use k_current_get as thread base and z_tls_current might be incorrect */
	printk("%s: %p\n", __func__, k_sched_current_thread_query());

	irq_unlock(key);
}

void sys_trace_isr_enter_user(void)
{
	unsigned int key = irq_lock();
	_cpu_t *curr_cpu = _current_cpu;

	printk("%s: %d\n", __func__, nested_interrupts[curr_cpu->id]);
	nested_interrupts[curr_cpu->id]++;

	irq_unlock(key);
}

void sys_trace_isr_exit_user(void)
{
	unsigned int key = irq_lock();
	_cpu_t *curr_cpu = _current_cpu;

	nested_interrupts[curr_cpu->id]--;
	printk("%s: %d\n", __func__, nested_interrupts[curr_cpu->id]);

	irq_unlock(key);
}

void sys_trace_idle_user(void)
{
	printk("%s\n", __func__);
}

void sys_trace_gpio_pin_active_user(const struct device *port, gpio_pin_t pin)
{
	printk("port: %s, pin: %d status: active\n", port->name, pin);
}

void sys_trace_gpio_pin_inactive_user(const struct device *port, gpio_pin_t pin)
{
	printk("port: %s, pin: %d status: inactive\n", port->name, pin);
}

void sys_trace_gpio_pin_configured_output_user(const struct device *port, gpio_pin_t pin,
					       gpio_flags_t flags)
{
	printk("port: %s, pin: %d status: configured output\n", port->name, pin);
}

void sys_trace_gpio_pin_configured_input_user(const struct device *port, gpio_pin_t pin,
					      gpio_flags_t flags)
{
	printk("port: %s, pin: %d status: configured input\n", port->name, pin);
}

void sys_trace_gpio_pin_event_attached_user(const struct device *port,
					    struct gpio_callback *callback)
{
	printk("port: %s status: event attached\n", port->name);
}

void sys_trace_gpio_pin_event_removed_user(const struct device *port,
					   struct gpio_callback *callback)
{
	printk("port: %s status: event removed\n", port->name);
}

void sys_trace_gpio_pin_event_executed_user(const struct device *port,
					    struct gpio_callback *callback)
{
	printk("port: %s status: event executed\n", port->name);
}
