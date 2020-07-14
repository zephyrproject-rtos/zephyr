/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/ps2.h>
#include <syscall_handler.h>

Z_SYSCALL_HANDLER(ps2_config, dev, callback_isr)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PS2(dev, config));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(callback_isr == 0,
				    "callback not be set from user mode"));
	return z_impl_ps2_config((const struct device *)dev, callback_isr);
}

Z_SYSCALL_HANDLER(ps2_write, dev, value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PS2(dev, write));
	return z_impl_ps2_write((const struct device *)dev, value);
}

Z_SYSCALL_HANDLER(ps2_read, dev, value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PS2(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(value, sizeof(uint8_t)));
	return z_impl_ps2_read((const struct device *)dev, (uint32_t *)value);
}

Z_SYSCALL_HANDLER(ps2_enable_callback)
{
	return z_impl_ps2_enable_callback((const struct device *)dev);
}

Z_SYSCALL_HANDLER(ps2_disable_callback, dev)
{
	return z_impl_ps2_disable_callback((const struct device *)dev);
}
