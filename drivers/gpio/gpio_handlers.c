/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <syscall_handler.h>

static inline int z_vrfy_gpio_config(struct device *port, int access_op,
				    u32_t pin, int flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, config));
	return z_impl_gpio_config((struct device *)port, access_op, pin, flags);
}
#include <syscalls/gpio_config_mrsh.c>

static inline int z_vrfy_gpio_write(struct device *port, int access_op,
				   u32_t pin, u32_t value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, write));
	return z_impl_gpio_write((struct device *)port, access_op, pin, value);
}
#include <syscalls/gpio_write_mrsh.c>

static inline int z_vrfy_gpio_read(struct device *port, int access_op,
				  u32_t pin, u32_t *value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(value, sizeof(u32_t)));
	return z_impl_gpio_read((struct device *)port, access_op, pin,
			       (u32_t *)value);
}
#include <syscalls/gpio_read_mrsh.c>

static inline int z_vrfy_gpio_port_get_raw(struct device *port,
					   gpio_port_value_t *value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_get_raw));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(value, sizeof(gpio_port_value_t)));
	return z_impl_gpio_port_get_raw((struct device *)port,
					(gpio_port_value_t *)value);
}
#include <syscalls/gpio_port_get_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_masked_raw(struct device *port,
		gpio_port_pins_t mask, gpio_port_value_t value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_masked_raw((struct device *)port, mask,
						value);
}
#include <syscalls/gpio_port_set_masked_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_bits_raw(struct device *port,
						gpio_port_pins_t pins)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_set_bits_raw));
	return z_impl_gpio_port_set_bits_raw((struct device *)port, pins);
}
#include <syscalls/gpio_port_set_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_clear_bits_raw(struct device *port,
						  gpio_port_pins_t pins)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_clear_bits_raw));
	return z_impl_gpio_port_clear_bits_raw((struct device *)port, pins);
}
#include <syscalls/gpio_port_clear_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_toggle_bits(struct device *port,
					       gpio_port_pins_t pins)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_toggle_bits));
	return z_impl_gpio_port_toggle_bits((struct device *)port, pins);
}
#include <syscalls/gpio_port_toggle_bits_mrsh.c>

static inline int z_vrfy_gpio_pin_interrupt_configure(struct device *port,
		unsigned int pin, unsigned int flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, pin_interrupt_configure));
	return z_impl_gpio_pin_interrupt_configure((struct device *)port, pin,
						   flags);
}
#include <syscalls/gpio_pin_interrupt_configure_mrsh.c>

static inline int z_vrfy_gpio_enable_callback(struct device *port,
					     int access_op, u32_t pin)
{
	return z_impl_gpio_enable_callback((struct device *)port, access_op,
					  pin);
}
#include <syscalls/gpio_enable_callback_mrsh.c>

static inline int z_vrfy_gpio_disable_callback(struct device *port,
					      int access_op, u32_t pin)
{
	return z_impl_gpio_disable_callback((struct device *)port, access_op,
					   pin);
}
#include <syscalls/gpio_disable_callback_mrsh.c>

static inline int z_vrfy_gpio_get_pending_int(struct device *dev)
{
	return z_impl_gpio_get_pending_int((struct device *)dev);
}
#include <syscalls/gpio_get_pending_int_mrsh.c>
