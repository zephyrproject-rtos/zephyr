/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for retained memory drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_
#define ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/sys/math_extras.h>

#ifdef __cplusplus
extern "C" {
#endif

BUILD_ASSERT(!(sizeof(off_t) > sizeof(size_t)),
	     "Size of off_t must be equal or less than size of size_t");

/**
 * @brief Retained memory driver interface
 * @defgroup retained_mem_interface Retained memory driver interface
 * @since 3.4
 * @version 0.8.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @typedef	retained_mem_size_api
 * @brief	Callback API to get size of retained memory area.
 * See retained_mem_size() for argument description.
 */
typedef ssize_t (*retained_mem_size_api)(const struct device *dev);

/**
 * @typedef	retained_mem_read_api
 * @brief	Callback API to read from retained memory area.
 * See retained_mem_read() for argument description.
 */
typedef int (*retained_mem_read_api)(const struct device *dev, off_t offset, uint8_t *buffer,
				     size_t size);

/**
 * @typedef	retained_mem_write_api
 * @brief	Callback API to write to retained memory area.
 * See retained_mem_write() for argument description.
 */
typedef int (*retained_mem_write_api)(const struct device *dev, off_t offset,
				      const uint8_t *buffer, size_t size);

/**
 * @typedef	retained_mem_clear_api
 * @brief	Callback API to clear retained memory area (reset all data to 0x00).
 * See retained_mem_clear() for argument description.
 */
typedef int (*retained_mem_clear_api)(const struct device *dev);

/**
 * @brief Retained memory driver API
 * API which can be used by a device to store data in a retained memory area. Retained memory is
 * memory that is retained while the device is powered but is lost when power to the device is
 * lost (note that low power modes in some devices may clear the data also). This may be in a
 * non-initialised RAM region, or in specific registers, but is not reset when a different
 * application begins execution or the device is rebooted (without power loss). It must support
 * byte-level reading and writing without a need to erase data before writing.
 *
 * Note that drivers must implement all functions, none of the functions are optional.
 */
__subsystem struct retained_mem_driver_api {
	retained_mem_size_api size;
	retained_mem_read_api read;
	retained_mem_write_api write;
	retained_mem_clear_api clear;
};

/**
 * @brief		Returns the size of the retained memory area.
 *
 * @param dev		Retained memory device to use.
 *
 * @retval		Positive value indicating size in bytes on success, else negative errno
 *			code.
 */
__syscall ssize_t retained_mem_size(const struct device *dev);

static inline ssize_t z_impl_retained_mem_size(const struct device *dev)
{
	struct retained_mem_driver_api *api = (struct retained_mem_driver_api *)dev->api;

	return api->size(dev);
}

/**
 * @brief		Reads data from the Retained memory area.
 *
 * @param dev		Retained memory device to use.
 * @param offset	Offset to read data from.
 * @param buffer	Buffer to store read data in.
 * @param size		Size of data to read.
 *
 * @retval		0 on success else negative errno code.
 */
__syscall int retained_mem_read(const struct device *dev, off_t offset, uint8_t *buffer,
				size_t size);

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

/**
 * @brief		Writes data to the Retained memory area - underlying data does not need to
 *			be cleared prior to writing.
 *
 * @param dev		Retained memory device to use.
 * @param offset	Offset to write data to.
 * @param buffer	Data to write.
 * @param size		Size of data to be written.
 *
 * @retval		0 on success else negative errno code.
 */
__syscall int retained_mem_write(const struct device *dev, off_t offset, const uint8_t *buffer,
				 size_t size);

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

/**
 * @brief		Clears data in the retained memory area by setting it to 0x00.
 *
 * @param dev		Retained memory device to use.
 *
 * @retval		0 on success else negative errno code.
 */
__syscall int retained_mem_clear(const struct device *dev);

static inline int z_impl_retained_mem_clear(const struct device *dev)
{
	struct retained_mem_driver_api *api = (struct retained_mem_driver_api *)dev->api;

	return api->clear(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/retained_mem.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_ */
