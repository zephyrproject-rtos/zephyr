/*
 * This device driver model is imspired by Linux's NVMEM subsystem, which is
 * Copyright (C) 2015 Srinivas Kandagatla <srinivas.kandagatla@linaro.org>
 * Copyright (C) 2013 Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * Referred Zephyr EEPROM device driver API during the design process, which is
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Heavily based on drivers/flash.h which is:
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup nvmem_model_interface
 * @brief NVMEM provider driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NVMEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_NVMEM_H_

/**
 * @brief Interfaces for NVMEM provider devices (EEPROM, OTP, FUSE, FRAM, etc.).
 * @defgroup nvmem_model_interface NVMEM providers
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <sys/types.h>
#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Storage type classification. */
enum nvmem_type {
	NVMEM_TYPE_UNKNOWN = 0,
	NVMEM_TYPE_EEPROM,
	NVMEM_TYPE_OTP,
	NVMEM_TYPE_FRAM,
};

/**
 * @brief NVMEM provider information
 *
 * @type:	Storage classification hint (e.g. EEPROM, OTP, FUSE, FRAM). Used by
 *		upper layers for diagnostics and policy defaults; providers
 *		still enforce hardware semantics (such as OTP 0->1 programming)
 *		in their read/write paths.
 *
 * @read_only:	Effective runtime mutability of the device. When true,
 *		providers must reject writes with -EROFS. This may reflect
 *		hardware/state such as WP GPIOs, fuse/lifecycle locks, or
 *		build-time write gates, and can change at runtime.
 */
struct nvmem_info {
	enum nvmem_type type;
	bool read_only;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/** @brief Callback to read from an NVMEM provider. */
typedef int (*nvmem_read_t)(const struct device *dev, off_t offset,
			void *buf, size_t len);

/** @brief Callback to write to an NVMEM provider. */
typedef int (*nvmem_write_t)(const struct device *dev, off_t offset,
			const void *buf, size_t len);

/** @brief Callback to get total size from an NVMEM provider. */
typedef size_t (*nvmem_get_size_t)(const struct device *dev);

/** @brief Optional provider hook to expose information to the core. */
typedef const struct nvmem_info *(*nvmem_get_info_t)(const struct device *dev);

/** @brief NVMEM provider driver API structure. */
__subsystem struct nvmem_driver_api {
	nvmem_read_t read;
	nvmem_write_t write;
	nvmem_get_size_t get_size;
	nvmem_get_info_t get_info;
};

/** @endcond */

/**
 * @brief Read bytes from an NVMEM provider.
 *
 * Providers should implement device-specific semantics and alignment rules.
 *
 * @param dev		NVMEM provider device instance.
 * @param offset	Start byte offset within the provider's logical data space.
 *
 * @param buf		Destination buffer.
 * @param len		Number of bytes to read.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
__syscall int nvmem_read(const struct device *dev, off_t offset,
			void *buf, size_t len);
static inline int z_impl_nvmem_read(const struct device *dev, off_t offset,
			void *buf, size_t len)
{
	return DEVICE_API_GET(nvmem, dev)->read(dev, offset, buf, len);
}

/**
 * @brief Write bytes to an NVMEM provider.
 *
 * @note: Providers must enforce their semantics (e.g., OTP 0->1 only) and
 *	  return -EROFS for read-only configurations.
 *
 * @param dev		NVMEM provider device instance.
 * @param offset	Start byte offset within the provider's logical data space.
 *
 * @param buf		Source buffer.
 * @param len		Number of bytes to write.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
__syscall int nvmem_write(const struct device *dev, off_t offset,
			const void *buf, size_t len);
static inline int z_impl_nvmem_write(const struct device *dev, off_t offset,
			const void *buf, size_t len)
{
	return DEVICE_API_GET(nvmem, dev)->write(dev, offset, buf, len);
}

/**
 * @brief Get total size in bytes of an NVMEM provider.
 *
 * @param dev	NVMEM provider device instance.
 *
 * @return Size in bytes.
 */
__syscall size_t nvmem_get_size(const struct device *dev);
static inline size_t z_impl_nvmem_get_size(const struct device *dev)
{
	return DEVICE_API_GET(nvmem, dev)->get_size(dev);
}

/**
 * @brief Get NVMEM provider information when available.
 *
 * @param dev	NVMEM provider device instance.
 *
 * @note Providers may return NULL to indicate no exported information.
 *
 * @retval A pointer to the nvmem_info structure.
 * @retval NULL if no information is available.
 */
__syscall const struct nvmem_info * nvmem_get_info(const struct device *dev);
static inline const struct nvmem_info *z_impl_nvmem_get_info(const struct device *dev)
{
	const struct nvmem_driver_api *api = DEVICE_API_GET(nvmem, dev);
	return api->get_info ? api->get_info(dev) : NULL;
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/nvmem.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_NVMEM_H_ */
