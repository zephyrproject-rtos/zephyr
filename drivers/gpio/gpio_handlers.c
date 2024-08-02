/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_gpio_pin_configure(const struct device *port,
					    gpio_pin_t pin,
					    gpio_flags_t flags)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, pin_configure));
	return z_impl_gpio_pin_configure((const struct device *)port,
					  pin,
					  flags);
}
#include <zephyr/syscalls/gpio_pin_configure_mrsh.c>

#ifdef CONFIG_GPIO_GET_CONFIG
static inline int z_vrfy_gpio_pin_get_config(const struct device *port,
					     gpio_pin_t pin,
					     gpio_flags_t *flags)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, pin_get_config));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(flags, sizeof(gpio_flags_t)));

	return z_impl_gpio_pin_get_config(port, pin, flags);
}
#include <zephyr/syscalls/gpio_pin_get_config_mrsh.c>
#endif

static inline int z_vrfy_gpio_port_get_raw(const struct device *port,
					   gpio_port_value_t *value)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_get_raw));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(value, sizeof(gpio_port_value_t)));
	return z_impl_gpio_port_get_raw((const struct device *)port,
					(gpio_port_value_t *)value);
}
#include <zephyr/syscalls/gpio_port_get_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_masked_raw(const struct device *port,
						  gpio_port_pins_t mask,
						  gpio_port_value_t value)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_masked_raw((const struct device *)port,
						mask,
						value);
}
#include <zephyr/syscalls/gpio_port_set_masked_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_bits_raw(const struct device *port,
						gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_bits_raw));
	return z_impl_gpio_port_set_bits_raw((const struct device *)port,
					     pins);
}
#include <zephyr/syscalls/gpio_port_set_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_clear_bits_raw(const struct device *port,
						  gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_clear_bits_raw));
	return z_impl_gpio_port_clear_bits_raw((const struct device *)port,
					       pins);
}
#include <zephyr/syscalls/gpio_port_clear_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_toggle_bits(const struct device *port,
					       gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_toggle_bits));
	return z_impl_gpio_port_toggle_bits((const struct device *)port, pins);
}
#include <zephyr/syscalls/gpio_port_toggle_bits_mrsh.c>

static inline int z_vrfy_gpio_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      gpio_flags_t flags)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, pin_interrupt_configure));
	return z_impl_gpio_pin_interrupt_configure((const struct device *)port,
						   pin,
						   flags);
}
#include <zephyr/syscalls/gpio_pin_interrupt_configure_mrsh.c>

static inline int z_vrfy_gpio_get_pending_int(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(dev, get_pending_int));

	return z_impl_gpio_get_pending_int((const struct device *)dev);
}
#include <zephyr/syscalls/gpio_get_pending_int_mrsh.c>

#ifdef CONFIG_GPIO_GET_DIRECTION
static inline int z_vrfy_gpio_port_get_direction(const struct device *dev, gpio_port_pins_t map,
						 gpio_port_pins_t *inputs,
						 gpio_port_pins_t *outputs)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(dev, port_get_direction));

	if (inputs != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(inputs, sizeof(gpio_port_pins_t)));
	}

	if (outputs != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(outputs, sizeof(gpio_port_pins_t)));
	}

	return z_impl_gpio_port_get_direction(dev, map, inputs, outputs);
}
#include <zephyr/syscalls/gpio_port_get_direction_mrsh.c>
#endif /* CONFIG_GPIO_GET_DIRECTION */
