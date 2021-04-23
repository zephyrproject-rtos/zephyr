/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/check.h>
#include <device.h>
#include <storage/flash_map.h>
#include <drivers/flash.h>
#include <soc.h>
#include <init.h>
#include "flash_map_priv.h"

void flash_area_foreach(flash_area_cb_t user_cb, void *user_data)
{
	extern const struct flash_area __flash_map_list_start[];
	extern const struct flash_area __flash_map_list_end[];
	const struct flash_area *iter = __flash_map_list_start;

	while (iter < __flash_map_list_end) {
		user_cb(iter, user_data);
		++iter;
	}
}

int flash_area_read(const struct flash_area *fa, off_t off, void *dst,
		    size_t len)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	if (!flash_area_is_in_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	if (!device_is_ready(fa->fa_dev)) {
		return -EIO;
	}

	return flash_read(fa->fa_dev, fa->fa_off + off, dst, len);
}

int flash_area_write(const struct flash_area *fa, off_t off, const void *src,
		     size_t len)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	if (!flash_area_is_in_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	if (!device_is_ready(fa->fa_dev)) {
		return -EIO;
	}

	return flash_write(fa->fa_dev, fa->fa_off + off, (void *)src, len);

}

int flash_area_erase(const struct flash_area *fa, off_t off, size_t len)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	if (!flash_area_is_in_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	if (!device_is_ready(fa->fa_dev)) {
		return -EIO;
	}

	return flash_erase(fa->fa_dev, fa->fa_off + off, len);
}

uint8_t flash_area_align(const struct flash_area *fa)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	return flash_get_write_block_size(fa->fa_dev);
}

uint8_t flash_area_erased_val(const struct flash_area *fa)
{
	const struct flash_parameters *param;

	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	param = flash_get_parameters(fa->fa_dev);

	return param->erase_value;
}
