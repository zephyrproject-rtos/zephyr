/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for kscan APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_KB_SCAN_H_
#error "Should only be included by zephyr/drivers/kscan.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_KSCAN_INTERNAL_KSCAN_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_KSCAN_INTERNAL_KSCAN_IMPL_H_

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_kscan_config(const struct device *dev,
					kscan_callback_t callback)
{
	const struct kscan_driver_api *api =
				(struct kscan_driver_api *)dev->api;

	return api->config(dev, callback);
}

static inline int z_impl_kscan_enable_callback(const struct device *dev)
{
	const struct kscan_driver_api *api =
			(const struct kscan_driver_api *)dev->api;

	if (api->enable_callback == NULL) {
		return -ENOSYS;
	}

	return api->enable_callback(dev);
}

static inline int z_impl_kscan_disable_callback(const struct device *dev)
{
	const struct kscan_driver_api *api =
			(const struct kscan_driver_api *)dev->api;

	if (api->disable_callback == NULL) {
		return -ENOSYS;
	}

	return api->disable_callback(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_KSCAN_INTERNAL_KSCAN_IMPL_H_ */
