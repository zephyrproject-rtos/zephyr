/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/key.h>

static inline int z_vrfy_key_setup(const struct device *dev,
				      key_callback_t callback_isr)
{
	Z_OOPS(Z_SYSCALL_DRIVER_KEY(dev, setup));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(callback_isr == 0,
				    "callback cannot be set from user mode"));
	return z_impl_key_setup((const struct device *)dev, callback_isr);
}
#include <syscalls/key_setup_mrsh.c>

static inline int z_vrfy_key_remove(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_KEY(dev, remove));

	return z_impl_key_remove((const struct device *)dev);
}
#include <syscalls/key_remove_mrsh.c>
