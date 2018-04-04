/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <syscall_handler.h>

_SYSCALL_HANDLER(gpio_config, port, access_op, pin, flags)
{
	_SYSCALL_DRIVER_GPIO(port, config);
	return _impl_gpio_config((struct device *)port, access_op, pin, flags);
}

_SYSCALL_HANDLER(gpio_write, port, access_op, pin, value)
{
	_SYSCALL_DRIVER_GPIO(port, write);
	return _impl_gpio_write((struct device *)port, access_op, pin, value);
}

_SYSCALL_HANDLER(gpio_read, port, access_op, pin, value)
{
	_SYSCALL_DRIVER_GPIO(port, read);
	_SYSCALL_MEMORY_WRITE(value, sizeof(u32_t));
	return _impl_gpio_read((struct device *)port, access_op, pin,
			       (u32_t *)value);
}

_SYSCALL_HANDLER(gpio_enable_callback, port, access_op, pin)
{
	_SYSCALL_DRIVER_GPIO(port, enable_callback);
	return _impl_gpio_enable_callback((struct device *)port, access_op,
					  pin);
}

_SYSCALL_HANDLER(gpio_disable_callback, port, access_op, pin)
{
	_SYSCALL_DRIVER_GPIO(port, disable_callback);
	return _impl_gpio_disable_callback((struct device *)port, access_op,
					   pin);
}

_SYSCALL_HANDLER(gpio_get_pending_int, port)
{
	_SYSCALL_DRIVER_GPIO(port, get_pending_int);
	return _impl_gpio_get_pending_int((struct device *)port);
}
