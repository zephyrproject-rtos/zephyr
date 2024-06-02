/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_wdt_setup(const struct device *dev, uint8_t options)
{
	K_OOPS(K_SYSCALL_DRIVER_WDT(dev, setup));
	return z_impl_wdt_setup(dev, options);
}

#include <zephyr/syscalls/wdt_setup_mrsh.c>

static inline int z_vrfy_wdt_disable(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_WDT(dev, disable));
	return z_impl_wdt_disable(dev);
}

#include <zephyr/syscalls/wdt_disable_mrsh.c>

static inline int z_vrfy_wdt_feed(const struct device *dev, int channel_id)
{
	K_OOPS(K_SYSCALL_DRIVER_WDT(dev, feed));
	return z_impl_wdt_feed(dev, channel_id);
}

#include <zephyr/syscalls/wdt_feed_mrsh.c>
