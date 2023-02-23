/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for retention API
 */

#ifndef ZEPHYR_INCLUDE_RETENTION_
#define ZEPHYR_INCLUDE_RETENTION_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Retention API
 * @defgroup retention_api Retention API
 * @ingroup retention
 * @{
 */

typedef ssize_t (*retention_size_api)(const struct device *dev);
typedef int (*retention_is_valid_api)(const struct device *dev);
typedef int (*retention_read_api)(const struct device *dev, off_t offset, uint8_t *buffer,
				  size_t size);
typedef int (*retention_write_api)(const struct device *dev, off_t offset,
				   const uint8_t *buffer, size_t size);
typedef int (*retention_clear_api)(const struct device *dev);

struct retention_api {
	retention_size_api size;
	retention_is_valid_api is_valid;
	retention_read_api read;
	retention_write_api write;
	retention_clear_api clear;
};

/**
 * @brief		Returns the size of the retention area.
 *
 * @param dev		Retention device to use.
 *
 * @retval		Positive value indicating size in bytes on success, else negative errno
 *			code.
 */
ssize_t retention_size(const struct device *dev);

/**
 * @brief		Checks if the underlying data in the retention area is valid or not.
 *
 * @param dev		Retention device to use.
 *
 * @retval 1		If successful and data is valid.
 * @retval 0		If data is not valid.
 * @retval -ENOTSUP	If there is no header/checksum configured for the retention area.
 * @retval -errno	Error code code.
 */
int retention_is_valid(const struct device *dev);

/**
 * @brief		Reads data from the retention area.
 *
 * @param dev		Retention device to use.
 * @param offset	Offset to read data from.
 * @param buffer	Buffer to store read data in.
 * @param size		Size of data to read.
 *
 * @retval 0		If successful.
 * @retval -errno	Error code code.
 */
int retention_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size);

/**
 * @brief		Writes data to the retention area (underlying data does not need to be
 *			cleared prior to writing), once function returns with a success code, the
 *			data will be classed as valid if queried using retention_is_valid().
 *
 * @param dev		Retention device to use.
 * @param offset	Offset to write data to.
 * @param buffer	Data to write.
 * @param size		Size of data to be written.
 *
 * @retval		0 on success else negative errno code.
 */
int retention_write(const struct device *dev, off_t offset, const uint8_t *buffer, size_t size);

/**
 * @brief		Clears all data in the retention area (sets it to 0)
 *
 * @param dev		Retention device to use.
 *
 * @retval		0 on success else negative errno code.
 */
int retention_clear(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RETENTION_ */
