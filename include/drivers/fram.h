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
 * @brief Public API for FRAM drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FRAM_H_
#define ZEPHYR_INCLUDE_DRIVERS_FRAM_H_

/**
 * @brief FRAM Interface
 * @defgroup fram_interface FRAM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*fram_api_read)(const struct device *dev, uint16_t addr, uint8_t *data, size_t len);
typedef int (*fram_api_write)(const struct device *dev, uint16_t addr, const uint8_t *data,
			      size_t len);

__subsystem struct fram_driver_api {
	fram_api_read read;
	fram_api_write write;
};

/**
 *  @brief Read data from FRAM
 *
 *  @param dev FRAM device
 *  @param addr Address addr to read from.
 *  @param data Buffer to store read data.
 *  @param len Number of bytes to read.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int fram_read(const struct device *dev, uint16_t addr, uint8_t *data, size_t len);

static inline int z_impl_fram_read(const struct device *dev, uint16_t addr, uint8_t *data,
				   size_t len)
{
	const struct fram_driver_api *api = (const struct fram_driver_api *)dev->api;

	return api->read(dev, addr, data, len);
}

/**
 *  @brief Write data to FRAM
 *
 *  @param dev FRAM device
 *  @param addr Address addr to write data to.
 *  @param data Buffer with data to write.
 *  @param len Number of bytes to write.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int fram_write(const struct device *dev, uint16_t addr, const uint8_t *data, size_t len);

static inline int z_impl_fram_write(const struct device *dev, uint16_t addr, const uint8_t *data,
				    size_t len)
{
	const struct fram_driver_api *api = (const struct fram_driver_api *)dev->api;

	return api->write(dev, addr, data, len);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/fram.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_FRAM_H_ */
