/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <watchdog.h>
#include <syscall_handler.h>

Z_SYSCALL_HANDLER(wdt_setup, dev, options)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, setup));
	return z_impl_wdt_setup((struct device *)dev, options);
}

Z_SYSCALL_HANDLER(wdt_disable, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, disable));
	return z_impl_wdt_disable((struct device *)dev);
}

Z_SYSCALL_HANDLER(wdt_feed, dev, channel_id)
{
	Z_OOPS(Z_SYSCALL_DRIVER_WDT(dev, feed));
	return z_impl_wdt_feed((struct device *)dev, channel_id);
}
