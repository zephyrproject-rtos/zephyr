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
#include <string.h>

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
 * @brief Callback API for getting the size of external memory.
 * See @a memc_get_size() for argument description.
 */
typedef int (*memc_api_get_size)(const struct device *dev, uint64_t *size);

/**
 * @brief Callback API for reading the ID of external memory.
 * See @a memc_read_id() for argument description.
 */
typedef int (*memc_api_read_id)(const struct device *dev, uint8_t *id, size_t len);

/**
 * @brief Callback API for getting the base address of external memory.
 * See @a memc_get_mem_base() for argument description.
 */
typedef void *(*memc_api_get_mem_base)(const struct device *dev);

/**
 * @driver_ops{MEMC}
 */
__subsystem struct memc_driver_api {
	/** @driver_ops_optional @copybrief memc_read */
	memc_api_read read;
	/** @driver_ops_optional @copybrief memc_write */
	memc_api_write write;
	/** @driver_ops_optional @copybrief memc_get_mem_base */
	memc_api_get_mem_base get_mem_base;
	/** @driver_ops_optional @copybrief memc_get_size */
	memc_api_get_size get_size;
	/** @driver_ops_optional @copybrief memc_read_id */
	memc_api_read_id read_id;
};

/** @} */

/**
 * @brief Read data from external memory.
 *
 * @param dev   MEMC device.
 * @param addr  Byte offset from the base address to read from.
 * @param data  Buffer to store read data.
 * @param len   Number of bytes to read.
 *
 * @return 0 on success, negative errno code on failure.
 * @retval -ENOTSUP if the device does not implement the memc API,
 *         or if neither @c get_mem_base nor @c read is implemented.
 */
__syscall int memc_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len);

static inline int z_impl_memc_read(const struct device *dev, uint32_t addr, uint8_t *data,
				   size_t len)
{
	if (!DEVICE_API_IS(memc, dev)) {
		return -ENOTSUP;
	}

	const struct memc_driver_api *api = DEVICE_API_GET(memc, dev);

	if (api->get_mem_base != NULL) {
		void *base = api->get_mem_base(dev);

		if (base != NULL) {
			memcpy(data, (const uint8_t *)base + addr, len);
			return 0;
		}
	}

	if (api->read != NULL) {
		return api->read(dev, addr, data, len);
	}

	return -ENOTSUP;
}

/**
 * @brief Write data to external memory.
 *
 * @param dev   MEMC device.
 * @param addr  Byte offset from the base address to write to.
 * @param data  Buffer containing data to write.
 * @param len   Number of bytes to write.
 *
 * @return 0 on success, negative errno code on failure.
 * @retval -ENOTSUP if the device does not implement the memc API,
 *         or if neither @c get_mem_base nor @c write is implemented.
 */
__syscall int memc_write(const struct device *dev, uint32_t addr, const uint8_t *data, size_t len);

static inline int z_impl_memc_write(const struct device *dev, uint32_t addr, const uint8_t *data,
				    size_t len)
{
	if (!DEVICE_API_IS(memc, dev)) {
		return -ENOTSUP;
	}

	const struct memc_driver_api *api = DEVICE_API_GET(memc, dev);

	if (api->get_mem_base != NULL) {
		void *base = api->get_mem_base(dev);

		if (base != NULL) {
			memcpy((uint8_t *)base + addr, data, len);
			return 0;
		}
	}

	if (api->write != NULL) {
		return api->write(dev, addr, data, len);
	}

	return -ENOTSUP;
}

/**
 * @brief Get the memory-mapped base address of a MEMC device.
 *
 * @note This function is available in kernel mode only (not a syscall).
 * The returned pointer is a kernel-space address and cannot be used from
 * userspace directly. Use @a memc_read() / @a memc_write() for portable access.
 *
 * @param dev   MEMC device.
 *
 * @return Pointer to the mapped base address, or NULL if not memory-mapped
 *         or if the device does not implement the memc API
 */
static inline void *memc_get_mem_base(const struct device *dev)
{
	if (!DEVICE_API_IS(memc, dev)) {
		return NULL;
	}

	const struct memc_driver_api *api = DEVICE_API_GET(memc, dev);

	if (api->get_mem_base == NULL) {
		return NULL;
	}

	return api->get_mem_base(dev);
}

/**
 * @brief Get the size of external memory.
 *
 * @param dev   MEMC device.
 * @param size  Pointer to variable to store size in bytes.
 *
 * @return 0 on success, negative errno code on failure.
 * @retval -ENOTSUP if the device does not implement the memc API
 *         or the optional callback.
 */
__syscall int memc_get_size(const struct device *dev, uint64_t *size);

static inline int z_impl_memc_get_size(const struct device *dev, uint64_t *size)
{
	if (!DEVICE_API_IS(memc, dev)) {
		return -ENOTSUP;
	}

	const struct memc_driver_api *api = DEVICE_API_GET(memc, dev);

	if (api->get_size == NULL) {
		return -ENOTSUP;
	}

	return api->get_size(dev, size);
}

/**
 * @brief Read the ID of external memory.
 *
 * @param dev   MEMC device.
 * @param id    Buffer to store ID (size depends on device, typically 2-4 bytes).
 * @param len   Number of bytes to read. Drivers may return fewer bytes
 *              than requested if the device ID is shorter.
 *
 * @return 0 on success, negative errno code on failure.
 * @retval -ENOTSUP if the device does not implement the memc API
 *         or the optional callback.
 */
__syscall int memc_read_id(const struct device *dev, uint8_t *id, size_t len);

static inline int z_impl_memc_read_id(const struct device *dev, uint8_t *id, size_t len)
{
	if (!DEVICE_API_IS(memc, dev)) {
		return -ENOTSUP;
	}

	const struct memc_driver_api *api = DEVICE_API_GET(memc, dev);

	if (api->read_id == NULL) {
		return -ENOTSUP;
	}

	return api->read_id(dev, id, len);
}

/** @} */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/memc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MEMC_H_ */
