/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for retained memory driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_
#error "Should only be included by zephyr/drivers/retained_mem.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_INTERNAL_RETAINED_MEM_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_INTERNAL_RETAINED_MEM_IMPL_H_

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline ssize_t z_impl_retained_mem_size(const struct device *dev)
{
	struct retained_mem_driver_api *api = (struct retained_mem_driver_api *)dev->api;

	return api->size(dev);
}

static inline int z_impl_retained_mem_read(const struct device *dev, off_t offset,
					   uint8_t *buffer, size_t size)
{
	struct retained_mem_driver_api *api = (struct retained_mem_driver_api *)dev->api;
	size_t area_size;

	/* Validate user-supplied parameters */
	if (size == 0) {
		return 0;
	}

	area_size = api->size(dev);

	if (offset < 0 || size > area_size || (area_size - size) < (size_t)offset) {
		return -EINVAL;
	}

	return api->read(dev, offset, buffer, size);
}

static inline int z_impl_retained_mem_write(const struct device *dev, off_t offset,
					    const uint8_t *buffer, size_t size)
{
	struct retained_mem_driver_api *api = (struct retained_mem_driver_api *)dev->api;
	size_t area_size;

	/* Validate user-supplied parameters */
	if (size == 0) {
		return 0;
	}

	area_size = api->size(dev);

	if (offset < 0 || size > area_size || (area_size - size) < (size_t)offset) {
		return -EINVAL;
	}

	return api->write(dev, offset, buffer, size);
}

static inline int z_impl_retained_mem_clear(const struct device *dev)
{
	struct retained_mem_driver_api *api = (struct retained_mem_driver_api *)dev->api;

	return api->clear(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_INTERNAL_RETAINED_MEM_IMPL_H_ */
