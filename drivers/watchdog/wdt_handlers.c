/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/watchdog.h>
#include <syscall_handler.h>

static inline int z_vrfy_wdt_setup(struct device *dev, u8_t options)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, wdt_setup));
	return z_impl_wdt_setup(dev, options);
}

#include <syscalls/wdt_setup.c>

static inline int z_vrfy_wdt_disable(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, wdt_disable));
	return z_impl_wdt_disable(dev);
}

#include <syscalls/wdt_disable.c>

static inline int z_vrfy_wdt_feed(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, wdt_feed));
	return z_impl_wdt_feed(dev);
}

#include <syscalls/wdt_feed.c>
