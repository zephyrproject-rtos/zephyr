/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/watchdog.h>
#include <syscall_handler.h>

static inline int z_vrfy_wdt_setup(const struct device *dev, uint8_t options)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, setup));
	return z_impl_wdt_setup(dev, options);
}

#include <syscalls/wdt_setup_mrsh.c>

static inline int z_vrfy_wdt_disable(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, disable));
	return z_impl_wdt_disable(dev);
}

#include <syscalls/wdt_disable_mrsh.c>

static inline int z_vrfy_wdt_feed(const struct device *dev, int channel_id)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, feed));
	return z_impl_wdt_feed(dev, channel_id);
}

#include <syscalls/wdt_feed_mrsh.c>
