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
