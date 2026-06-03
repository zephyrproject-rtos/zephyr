/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup memc_interface
 * @brief Main header file for Memory Controller (MEMC) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MEMC_H_
#define ZEPHYR_INCLUDE_DRIVERS_MEMC_H_

/**
 * @brief Interfaces for Memory Controllers (MEMC).
 * @defgroup memc_interface MEMC
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def_driverbackendgroup{MEMC,memc_interface}
 * @ingroup memc_interface
 * @{
 */

/**
 * @brief Callback API for reading from external memory.
 * See @a memc_read() for argument description.
 */
typedef int (*memc_api_read)(const struct device *dev, uint32_t addr, uint8_t *data, size_t len);

/**
 * @brief Callback API for writing to external memory.
 * See @a memc_write() for argument description.
 */
typedef int (*memc_api_write)(const struct device *dev, uint32_t addr, const uint8_t *data,
			      size_t len);

/**
 * @driver_ops{MEMC}
 */
__subsystem struct memc_driver_api {
	/** @driver_ops_mandatory @copybrief memc_read */
	memc_api_read read;
	/** @driver_ops_mandatory @copybrief memc_write */
	memc_api_write write;
};

/** @} */

/**
 * @brief Read data from external memory.
 *
 * @param dev   MEMC device.
 * @param addr  Byte address to read from.
 * @param data  Buffer to store read data.
 * @param len   Number of bytes to read.
 *
 * @return 0 on success, negative errno code on failure.
 */
__syscall int memc_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len);

static inline int z_impl_memc_read(const struct device *dev, uint32_t addr, uint8_t *data,
				   size_t len)
{
	const struct memc_driver_api *api = DEVICE_API_GET(memc, dev);

	if (api->read == NULL) {
		return -ENOTSUP;
	}

	return api->read(dev, addr, data, len);
}

/**
 * @brief Write data to external memory.
 *
 * @param dev   MEMC device.
 * @param addr  Byte address to write to.
 * @param data  Buffer containing data to write.
 * @param len   Number of bytes to write.
 *
 * @return 0 on success, negative errno code on failure.
 */
__syscall int memc_write(const struct device *dev, uint32_t addr, const uint8_t *data, size_t len);

static inline int z_impl_memc_write(const struct device *dev, uint32_t addr, const uint8_t *data,
				    size_t len)
{
	const struct memc_driver_api *api = DEVICE_API_GET(memc, dev);

	if (api->write == NULL) {
		return -ENOTSUP;
	}

	return api->write(dev, addr, data, len);
}

/** @} */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/memc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MEMC_H_ */
