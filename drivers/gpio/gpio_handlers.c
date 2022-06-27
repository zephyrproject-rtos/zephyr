/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_gpio_pin_configure(const struct device *port,
					    gpio_pin_t pin,
					    gpio_flags_t flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, pin_configure));
	return z_impl_gpio_pin_configure((const struct device *)port,
					  pin,
					  flags);
}
#include <syscalls/gpio_pin_configure_mrsh.c>

static inline int z_vrfy_gpio_port_get_raw(const struct device *port,
					   gpio_port_value_t *value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_get_raw));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(value, sizeof(gpio_port_value_t)));
	return z_impl_gpio_port_get_raw((const struct device *)port,
					(gpio_port_value_t *)value);
}
#include <syscalls/gpio_port_get_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_masked_raw(const struct device *port,
						  gpio_port_pins_t mask,
						  gpio_port_value_t value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_masked_raw((const struct device *)port,
						mask,
						value);
}
#include <syscalls/gpio_port_set_masked_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_bits_raw(const struct device *port,
						gpio_port_pins_t pins)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_set_bits_raw));
	return z_impl_gpio_port_set_bits_raw((const struct device *)port,
					     pins);
}
#include <syscalls/gpio_port_set_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_clear_bits_raw(const struct device *port,
						  gpio_port_pins_t pins)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_clear_bits_raw));
	return z_impl_gpio_port_clear_bits_raw((const struct device *)port,
					       pins);
}
#include <syscalls/gpio_port_clear_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_toggle_bits(const struct device *port,
					       gpio_port_pins_t pins)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, port_toggle_bits));
	return z_impl_gpio_port_toggle_bits((const struct device *)port, pins);
}
#include <syscalls/gpio_port_toggle_bits_mrsh.c>

static inline int z_vrfy_gpio_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      gpio_flags_t flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(port, pin_interrupt_configure));
	return z_impl_gpio_pin_interrupt_configure((const struct device *)port,
						   pin,
						   flags);
}
#include <syscalls/gpio_pin_interrupt_configure_mrsh.c>

static inline int z_vrfy_gpio_get_pending_int(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_GPIO(dev, get_pending_int));

	return z_impl_gpio_get_pending_int((const struct device *)dev);
}
#include <syscalls/gpio_get_pending_int_mrsh.c>
