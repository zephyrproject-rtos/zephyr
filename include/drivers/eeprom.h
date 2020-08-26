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
 * @brief Public API for EEPROM drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_H_
#define ZEPHYR_INCLUDE_DRIVERS_EEPROM_H_

/**
 * @brief EEPROM Interface
 * @defgroup eeprom_interface EEPROM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*eeprom_api_read)(struct device *dev, off_t offset, void *data,
			       size_t len);
typedef int (*eeprom_api_write)(struct device *dev, off_t offset,
				const void *data, size_t len);
typedef size_t (*eeprom_api_size)(struct device *dev);

__subsystem struct eeprom_driver_api {
	eeprom_api_read read;
	eeprom_api_write write;
	eeprom_api_size size;
};

/**
 *  @brief Read data from EEPROM
 *
 *  @param dev EEPROM device
 *  @param offset Address offset to read from.
 *  @param data Buffer to store read data.
 *  @param len Number of bytes to read.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int eeprom_read(struct device *dev, off_t offset, void *data,
			  size_t len);

static inline int z_impl_eeprom_read(struct device *dev, off_t offset,
				     void *data, size_t len)
{
	const struct eeprom_driver_api *api =
		(const struct eeprom_driver_api *)dev->api;

	return api->read(dev, offset, data, len);
}

/**
 *  @brief Write data to EEPROM
 *
 *  @param dev EEPROM device
 *  @param offset Address offset to write data to.
 *  @param data Buffer with data to write.
 *  @param len Number of bytes to write.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int eeprom_write(struct device *dev, off_t offset, const void *data,
			   size_t len);

static inline int z_impl_eeprom_write(struct device *dev, off_t offset,
				      const void *data, size_t len)
{
	const struct eeprom_driver_api *api =
		(const struct eeprom_driver_api *)dev->api;

	return api->write(dev, offset, data, len);
}

/**
 *  @brief Get the size of the EEPROM in bytes
 *
 *  @param dev EEPROM device.
 *
 *  @return EEPROM size in bytes.
 */
__syscall size_t eeprom_get_size(struct device *dev);

static inline size_t z_impl_eeprom_get_size(struct device *dev)
{
	const struct eeprom_driver_api *api =
		(const struct eeprom_driver_api *)dev->api;

	return api->size(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/eeprom.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_H_ */
