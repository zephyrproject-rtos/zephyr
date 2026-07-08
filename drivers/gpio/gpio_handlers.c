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
	return z_impl_gpio_pin_configure(port, pin, flags);
}
#include <zephyr/syscalls/gpio_pin_configure_mrsh.c>

#ifdef CONFIG_GPIO_GET_CONFIG
static inline int z_vrfy_gpio_pin_get_config(const struct device *port,
					     gpio_pin_t pin,
					     gpio_flags_t *flags)
{
	K_OOPS(K_SYSCALL_OBJ(port, K_OBJ_DRIVER_GPIO));
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
	return z_impl_gpio_port_get_raw(port, value);
}
#include <zephyr/syscalls/gpio_port_get_raw_mrsh.c>

static inline int z_vrfy_gpio_port_get(const struct device *port,
				       gpio_port_value_t *value)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_get_raw));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(value, sizeof(gpio_port_value_t)));
	return z_impl_gpio_port_get(port, value);
}
#include <zephyr/syscalls/gpio_port_get_mrsh.c>

static inline int z_vrfy_gpio_port_set_masked_raw(const struct device *port,
						  gpio_port_pins_t mask,
						  gpio_port_value_t value)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_masked_raw(port, mask, value);
}
#include <zephyr/syscalls/gpio_port_set_masked_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_masked(const struct device *port,
					      gpio_port_pins_t mask,
					      gpio_port_value_t value)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_masked(port, mask, value);
}
#include <zephyr/syscalls/gpio_port_set_masked_mrsh.c>

static inline int z_vrfy_gpio_port_set_bits_raw(const struct device *port,
						gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_bits_raw));
	return z_impl_gpio_port_set_bits_raw(port, pins);
}
#include <zephyr/syscalls/gpio_port_set_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_bits(const struct device *port,
					    gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_bits(port, pins);
}
#include <zephyr/syscalls/gpio_port_set_bits_mrsh.c>

static inline int z_vrfy_gpio_port_clear_bits_raw(const struct device *port,
						  gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_clear_bits_raw));
	return z_impl_gpio_port_clear_bits_raw(port, pins);
}
#include <zephyr/syscalls/gpio_port_clear_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_clear_bits(const struct device *port,
					      gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_clear_bits(port, pins);
}
#include <zephyr/syscalls/gpio_port_clear_bits_mrsh.c>

static inline int z_vrfy_gpio_port_toggle_bits(const struct device *port,
					       gpio_port_pins_t pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_toggle_bits));
	return z_impl_gpio_port_toggle_bits(port, pins);
}
#include <zephyr/syscalls/gpio_port_toggle_bits_mrsh.c>

static inline int z_vrfy_gpio_port_set_clr_bits_raw(const struct device *port,
						    gpio_port_pins_t set_pins,
						    gpio_port_pins_t clear_pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_clr_bits_raw(port, set_pins, clear_pins);
}
#include <zephyr/syscalls/gpio_port_set_clr_bits_raw_mrsh.c>

static inline int z_vrfy_gpio_port_set_clr_bits(const struct device *port,
						gpio_port_pins_t set_pins,
						gpio_port_pins_t clear_pins)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_masked_raw));
	return z_impl_gpio_port_set_clr_bits(port, set_pins, clear_pins);
}
#include <zephyr/syscalls/gpio_port_set_clr_bits_mrsh.c>

static inline int z_vrfy_gpio_pin_get_raw(const struct device *port, gpio_pin_t pin)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_get_raw));
	return z_impl_gpio_pin_get_raw(port, pin);
}
#include <zephyr/syscalls/gpio_pin_get_raw_mrsh.c>

static inline int z_vrfy_gpio_pin_get(const struct device *port, gpio_pin_t pin)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_get_raw));
	return z_impl_gpio_pin_get(port, pin);
}
#include <zephyr/syscalls/gpio_pin_get_mrsh.c>

static inline int z_vrfy_gpio_pin_set_raw(const struct device *port, gpio_pin_t pin, int value)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_bits_raw));
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_clear_bits_raw));
	return z_impl_gpio_pin_set_raw(port, pin, value);
}
#include <zephyr/syscalls/gpio_pin_set_raw_mrsh.c>

static inline int z_vrfy_gpio_pin_set(const struct device *port, gpio_pin_t pin, int value)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_set_bits_raw));
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_clear_bits_raw));
	return z_impl_gpio_pin_set(port, pin, value);
}
#include <zephyr/syscalls/gpio_pin_set_mrsh.c>

static inline int z_vrfy_gpio_pin_toggle(const struct device *port, gpio_pin_t pin)
{
	K_OOPS(K_SYSCALL_DRIVER_GPIO(port, port_toggle_bits));
	return z_impl_gpio_pin_toggle(port, pin);
}
#include <zephyr/syscalls/gpio_pin_toggle_mrsh.c>

static inline int z_vrfy_gpio_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      gpio_flags_t flags)
{
	K_OOPS(K_SYSCALL_OBJ(port, K_OBJ_DRIVER_GPIO));
	return z_impl_gpio_pin_interrupt_configure(port, pin, flags);
}
#include <zephyr/syscalls/gpio_pin_interrupt_configure_mrsh.c>

static inline int z_vrfy_gpio_get_pending_int(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_GPIO));
	return z_impl_gpio_get_pending_int(dev);
}
#include <zephyr/syscalls/gpio_get_pending_int_mrsh.c>

#ifdef CONFIG_GPIO_GET_DIRECTION
static inline int z_vrfy_gpio_port_get_direction(const struct device *dev, gpio_port_pins_t map,
						 gpio_port_pins_t *inputs,
						 gpio_port_pins_t *outputs)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_GPIO));

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
