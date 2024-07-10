/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BBRAM_H
#error "Should only be included by zephyr/drivers/bbram.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_BBRAM_INTERNAL_BBRAM_IMPL_H
#define ZEPHYR_INCLUDE_DRIVERS_BBRAM_INTERNAL_BBRAM_IMPL_H

#include <errno.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_bbram_check_invalid(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_invalid) {
		return -ENOTSUP;
	}

	return api->check_invalid(dev);
}

static inline int z_impl_bbram_check_standby_power(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_standby_power) {
		return -ENOTSUP;
	}

	return api->check_standby_power(dev);
}

static inline int z_impl_bbram_check_power(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_power) {
		return -ENOTSUP;
	}

	return api->check_power(dev);
}

static inline int z_impl_bbram_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->get_size) {
		return -ENOTSUP;
	}

	return api->get_size(dev, size);
}

static inline int z_impl_bbram_read(const struct device *dev, size_t offset,
				    size_t size, uint8_t *data)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->read) {
		return -ENOTSUP;
	}

	return api->read(dev, offset, size, data);
}

static inline int z_impl_bbram_write(const struct device *dev, size_t offset,
				     size_t size, const uint8_t *data)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->write) {
		return -ENOTSUP;
	}

	return api->write(dev, offset, size, data);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_BBRAM_INTERNAL_BBRAM_IMPL_H */
