/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <syscall_handler.h>

_SYSCALL_HANDLER(gpio_config, port, access_op, pin, flags)
{
	_SYSCALL_OBJ(port, K_OBJ_DRIVER_GPIO);
	return _impl_gpio_config((struct device *)port, access_op, pin, flags);
}

_SYSCALL_HANDLER(gpio_write, port, access_op, pin, value)
{
	_SYSCALL_OBJ(port, K_OBJ_DRIVER_GPIO);
	return _impl_gpio_write((struct device *)port, access_op, pin, value);
}

_SYSCALL_HANDLER(gpio_read, port, access_op, pin, value)
{
	_SYSCALL_OBJ(port, K_OBJ_DRIVER_GPIO);
	_SYSCALL_MEMORY_WRITE(value, sizeof(u32_t));
	return _impl_gpio_read((struct device *)port, access_op, pin,
			       (u32_t *)value);
}

_SYSCALL_HANDLER(gpio_enable_callback, port, access_op, pin)
{
	_SYSCALL_OBJ(port, K_OBJ_DRIVER_GPIO);
	return _impl_gpio_enable_callback((struct device *)port, access_op,
					  pin);
}

_SYSCALL_HANDLER(gpio_disable_callback, port, access_op, pin)
{
	_SYSCALL_OBJ(port, K_OBJ_DRIVER_GPIO);
	return _impl_gpio_disable_callback((struct device *)port, access_op,
					   pin);
}

_SYSCALL_HANDLER1_SIMPLE(gpio_get_pending_int, K_OBJ_DRIVER_GPIO,
			 struct device *);
