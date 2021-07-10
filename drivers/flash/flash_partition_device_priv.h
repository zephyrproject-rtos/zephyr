/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_PARTITION_DEVICE_PRIV_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_PARTITION_DEVICE_PRIV_H_

struct flash_partition_device {
	const struct device *const real_dev;
	off_t offset;
	size_t size;
};

struct flash_partition_device_priv {
	struct flash_parameters parameters;
	size_t page_count;
};

#endif
