/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ADDR_TRANSLATION_H_
#define ZEPHYR_DRIVERS_MISC_ADDR_TRANSLATION_H_

#include <zephyr/device.h>
#include <metal/io.h>

/**
 * @brief Return generic I/O operations
 *
 * @param	dev	Driver instance pointer
 * @return	metal_io_ops struct
 */
static const struct metal_io_ops *metal_io_get_ops(const struct device *dev)
{
	if (!dev || !dev->api) {
		return NULL;
	}

	return (const struct metal_io_ops *)(dev->api);
}

#endif /* ZEPHYR_DRIVERS_MISC_ADDR_TRANSLATION_H_ */
