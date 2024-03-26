/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ZSAI drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZSAI_IOCTL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZSAI_IOCTL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/zsai.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZSAI_IOCTL_GET_INFOWORD	0x00
#define ZSAI_IOCTL_GET_SIZE		0x02
#define ZSAI_IOCTL_GET_PAGE_INFO	0x03

typedef struct zsai_ioctl_range zsai_ioctl_page;

/**
 * @brief Get page information from device.
 *
 * @param	dev	: device to query for page information.
 * @param	offset  : offset within memory to get page information on.
 * @param	pi	: zsai_ioctl_range type pointer for information about
 *			  page found within given range.
 *
 * @return 0 on success. -ENOTSUP if device does not have erase requirement
 *	   and page definitions, -ENOENT in case when @p offset is within
 *	   memory gap; other -ERRNO code in case of failure.
 */
static inline int zsai_get_page_info(const struct device *dev, size_t offset,
				     struct zsai_ioctl_range *pi)
{
	return zsai_ioctl(dev, ZSAI_IOCTL_GET_PAGE_INFO, (uintptr_t)offset,
			  (uintptr_t)pi);
}

/**
 * @brief Get device size
 *
 * @param	dev	: device to query for size.
 * @param	size	: pointer for size information.
 *
 * @return 0 on success, negative errno code on failure.
 */
static inline int zsai_get_size(const struct device *dev, size_t *size)
{
	return zsai_ioctl(dev, ZSAI_IOCTL_GET_SIZE, (uintptr_t)NULL,
			  (uintptr_t)size);
}

/**
 * @brief Fill device, within specified boundaries, with pattern.
 *
 * @param	dev	: device to operate on.
 * @param	pattern : pattern to fill range with.
 * @param	offset	: offset to start filling device at, needs to
 *			  be write block size aligned.
 * @param	size	: size of region to fill, from the @p offset
 *			  needs to be write block size aligned.
 *
 * @return 0 on success, negative errno code on failure.
 */
int zsai_fill(const struct device *dev, uint8_t pattern, size_t offset,
	      size_t size);

/**
 * @brief Fill device range with pattern.
 *
 * @param	dev	: device to operate on.
 * @param	pattern ; pattern to fill range with.
 * @param	range	: range to errase.
 */
static inline int zsai_fill_range(const struct device *dev, uint8_t pattern,
				  const struct zsai_ioctl_range *range)
{
	return zsai_fill(dev, pattern, range->offset, range->size);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ZSAI_IOCTL_H_ */
