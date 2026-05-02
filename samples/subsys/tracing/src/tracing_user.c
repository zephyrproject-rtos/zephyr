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

void sys_trace_gpio_pin_configure_enter_user(const struct device *port, gpio_pin_t pin,
					     gpio_flags_t flags)
{
	printk("port: %s, pin: %d flags: %d\n", port->name, pin, flags);
}

void sys_trace_gpio_pin_configure_exit_user(const struct device *port, gpio_pin_t pin, int ret)
{
	printk("port: %s, pin: %d ret: %d\n", port->name, pin, ret);
}

void sys_trace_gpio_port_set_bits_raw_enter_user(const struct device *port, gpio_port_pins_t pins)
{
	printk("port: %s, pins: %d\n", port->name, pins);
}

void sys_trace_gpio_port_set_bits_raw_exit_user(const struct device *port, int ret)
{
	printk("port: %s, ret: %d\n", port->name, ret);
}

void sys_trace_gpio_port_clear_bits_raw_enter_user(const struct device *port, gpio_port_pins_t pins)
{
	printk("port: %s, pins: %d\n", port->name, pins);
}

void sys_trace_gpio_port_clear_bits_raw_exit_user(const struct device *port, int ret)
{
	printk("port: %s, pins: %d\n", port->name, ret);
}

void sys_trace_gpio_pin_interrupt_configure_enter_user(const struct device *port, gpio_pin_t pin,
						       gpio_flags_t flags)
{
	printk("port: %s, pin: %d flags: %d\n", port->name, pin, flags);
}

void sys_trace_gpio_pin_interrupt_configure_exit_user(const struct device *port, gpio_pin_t pin,
						      int ret)
{
	printk("port: %s, pin: %d ret: %d\n", port->name, pin, ret);
}
