/*
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup fuse_interface
 * @brief Main header file for fuse driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FUSE_H_
#define ZEPHYR_INCLUDE_DRIVERS_FUSE_H_

/**
 * @brief Interfaces for One Time Programmable Read-Only Memory (fuse).
 * @defgroup fuse_interface
 * @since 4.2
 * @version 1.0.0
 * @ingroup io_interfaces
 */

#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @brief Callback API upon reading from the fuse.
 * See @a fuse_read() for argument description
 */
typedef int (*fuse_api_read)(const struct device *dev, off_t offset, void *data, size_t len);

/**
 * @brief Callback API upon writing to the fuse.
 * See @a fuse_program() for argument description
 */
typedef int (*fuse_api_program)(const struct device *dev, off_t offset, const void *data,
				size_t len);

__subsystem struct fuse_driver_api {
	fuse_api_read read;
	fuse_api_program program;
};

/** @endcond */

/**
 *  @brief Read data from fuse
 *
 *  @param dev Fuse device
 *  @param offset Address offset to read from.
 *  @param data Buffer to store read data.
 *  @param len Number of bytes to read.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int fuse_read(const struct device *dev, off_t offset, void *data, size_t len);

static inline int z_impl_fuse_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct fuse_driver_api *api = (const struct fuse_driver_api *)dev->api;

	if (api->read == NULL) {
		return -ENOTSUP;
	}

	return api->read(dev, offset, data, len);
}

/**
 *  @brief Data to be programmed in the given fuses
 *
 *  @param dev Fuse device
 *  @param offset Address offset to program data to.
 *  @param data Buffer with data to program.
 *  @param len Number of bytes to program.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int fuse_program(const struct device *dev, off_t offset, const void *data, size_t len);

static inline int z_impl_fuse_program(const struct device *dev, off_t offset, const void *data,
				      size_t len)
{
	const struct fuse_driver_api *api = (const struct fuse_driver_api *)dev->api;

	if (api->program == NULL) {
		return -ENOTSUP;
	}

	return api->program(dev, offset, data, len);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/fuse.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_FUSE_H_ */
