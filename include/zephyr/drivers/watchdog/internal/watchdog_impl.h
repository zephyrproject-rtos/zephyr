/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_
#error "Should only be included by zephyr/drivers/watchdog.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_INTERNAL_WATCHDOG_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_INTERNAL_WATCHDOG_IMPL_H_

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_wdt_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_driver_api *api =
		(const struct wdt_driver_api *)dev->api;

	return api->setup(dev, options);
}

static inline int z_impl_wdt_disable(const struct device *dev)
{
	const struct wdt_driver_api *api =
		(const struct wdt_driver_api *)dev->api;

	return api->disable(dev);
}

static inline int z_impl_wdt_feed(const struct device *dev, int channel_id)
{
	const struct wdt_driver_api *api =
		(const struct wdt_driver_api *)dev->api;

	return api->feed(dev, channel_id);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_INTERNAL_WATCHDOG_IMPL_H_ */
