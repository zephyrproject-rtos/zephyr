/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pinmux.h>
#include <syscall_handler.h>
#include <toolchain.h>

_SYSCALL_HANDLER(pinmux_pin_set, dev, pin, func)
{
	_SYSCALL_DRIVER_PINMUX(dev, set);
	return _impl_pinmux_pin_set((struct device *)dev, pin, func);
}

_SYSCALL_HANDLER(pinmux_pin_get, dev, pin, func)
{
	_SYSCALL_DRIVER_PINMUX(dev, get);
	_SYSCALL_MEMORY_WRITE(func, sizeof(u32_t));
	return _impl_pinmux_pin_get((struct device *)dev, pin,
				    (u32_t *)func);
}

_SYSCALL_HANDLER(pinmux_pin_pullup, dev, pin, func)
{
	_SYSCALL_DRIVER_PINMUX(dev, pullup);
	return _impl_pinmux_pin_pullup((struct device *)dev, pin, func);
}

_SYSCALL_HANDLER(pinmux_pin_input_enable, dev, pin, func)
{
	_SYSCALL_DRIVER_PINMUX(dev, input);
	return _impl_pinmux_pin_input_enable((struct device *)dev, pin, func);
}
