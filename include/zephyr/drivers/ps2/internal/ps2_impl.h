/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for PS2 APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PS2_H_
#error "Should only be included by zephyr/drivers/ps2.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_PS2_INTERNAL_PS2_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PS2_INTERNAL_PS2_IMPL_H_

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_ps2_config(const struct device *dev,
				    ps2_callback_t callback_isr)
{
	const struct ps2_driver_api *api =
				(struct ps2_driver_api *)dev->api;

	return api->config(dev, callback_isr);
}

static inline int z_impl_ps2_write(const struct device *dev, uint8_t value)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	return api->write(dev, value);
}

static inline int z_impl_ps2_read(const struct device *dev, uint8_t *value)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	return api->read(dev, value);
}

static inline int z_impl_ps2_enable_callback(const struct device *dev)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	if (api->enable_callback == NULL) {
		return -ENOSYS;
	}

	return api->enable_callback(dev);
}

static inline int z_impl_ps2_disable_callback(const struct device *dev)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	if (api->disable_callback == NULL) {
		return -ENOSYS;
	}

	return api->disable_callback(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PS2_INTERNAL_PS2_IMPL_H_ */
