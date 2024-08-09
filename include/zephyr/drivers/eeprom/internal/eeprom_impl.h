/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Heavily based on drivers/flash.h which is:
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for EEPROM driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_H_
#error "Should only be included by zephyr/drivers/eeprom.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_INTERNAL_EEPROM_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_EEPROM_INTERNAL_EEPROM_IMPL_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_eeprom_read(const struct device *dev, off_t offset,
				     void *data, size_t len)
{
	const struct eeprom_driver_api *api =
		(const struct eeprom_driver_api *)dev->api;

	return api->read(dev, offset, data, len);
}

static inline int z_impl_eeprom_write(const struct device *dev, off_t offset,
				      const void *data, size_t len)
{
	const struct eeprom_driver_api *api =
		(const struct eeprom_driver_api *)dev->api;

	return api->write(dev, offset, data, len);
}

static inline size_t z_impl_eeprom_get_size(const struct device *dev)
{
	const struct eeprom_driver_api *api =
		(const struct eeprom_driver_api *)dev->api;

	return api->size(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_INTERNAL_EEPROM_IMPL_H_ */
