/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ZSAI drivers, aucilary functions
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZSAI_AUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZSAI_AUX_H_

/**
 * @brief ZSAI internal Interface
 * @defgroup zsai_internal_interface ZSAI internal Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/zsai_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @}
 */

/**
 * @brief Erase part of a device.
 *
 * Device needs to support erase procedure, otherwise -ENOTSUP will be returned..
 *
 * @param	dev	: device to erase.
 * @param	start	: start offset on device, needs to be aligned to page
 *			: offset of device.
 * @param	size	: size to erase, needs to be aligned to page size.
 *
 * @return 0 on success, -ENOTSUP when erase not supported; other negative errno
 * number in case of failure.
 */
int zsai_erase(const struct device *dev, size_t start, size_t size);

/**
 * @brief Erase device within specified boundaries or emulate erase by filling
 *	  device with provided pattern.
 *
 * @param	dev	: device to erase.
 * @param	pattern ; pattern to use when emulating erase.
 * @param	start	: start offset on device, needs to be aligned
 *			  to page size of device.
 * @param	size	: size to erase, needs to be aligned to page size.
 *
 * @return 0 on success. Negative errno in case of failure.
 */
int zsai_erase_or_fill(const struct device *dev, uint8_t pattern, size_t start,
		       size_t size);

/**
 * @brief Erase device within specified range or emulate erase by filling
 *	  device with provided pattern.
 *
 * @param	dev	: device to erase.
 * @param	pattern ; pattern to use when emulating erase.
 * @param	range	: page range.
 *
 * @return 0 on success. Negative errno in case of failure.
 */
int zsai_erase_range_or_fill(const struct device *dev, uint8_t pattern,
			     const struct zsai_ioctl_range *range);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/zsai_aux.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_ZSAI_AUX_H_ */
