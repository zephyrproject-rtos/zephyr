/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 * Copyright (c) 2023 Sensorfy B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>
#include "flash_map_priv.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>

#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
#define HAS_CUSTOM_BACKEND_ID(id) (((id) & \
		FLASH_MAP_CUSTOM_BACKEND_MASK) == FLASH_MAP_CUSTOM_BACKEND_MASK)
#endif

void flash_area_foreach(flash_area_cb_t user_cb, void *user_data)
{
	for (int i = 0; i < flash_map_entries; i++) {
		user_cb(&flash_map[i], user_data);
	}
}

int flash_area_open(uint8_t id, const struct flash_area **fap)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(id)) {
		return flash_area_open_custom(id, fap);
	}
#endif
#ifdef CONFIG_FLASH_HAS_DRIVER_ENABLED
	const struct flash_area *area;

	if (flash_map == NULL) {
		return -EACCES;
	}

	area = get_flash_area_from_id(id);
	if (area == NULL) {
		return -ENOENT;
	}

	if (!area->fa_dev || !device_is_ready(area->fa_dev)) {
		return -ENODEV;
	}

	*fap = area;
#endif
	return 0;
}

void flash_area_close(const struct flash_area *fa)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(fa->fa_id) && fa->api->close) {
		fa->api->close(fa);
	}
#endif
	/* nothing to do for now */
}

int flash_area_read(const struct flash_area *fa, off_t off, void *dst,
		    size_t len)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(fa->fa_id)) {
		if (fa->api->read) {
			return fa->api->read(fa, off, dst, len);
		}
		return -ENOTSUP;
	}
#endif
	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	return flash_read(fa->fa_dev, fa->fa_off + off, dst, len);
}

int flash_area_write(const struct flash_area *fa, off_t off, const void *src,
		     size_t len)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(fa->fa_id)) {
		if (fa->api->write) {
			return fa->api->write(fa, off, src, len);
		}
		return -ENOTSUP;
	}
#endif
	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	return flash_write(fa->fa_dev, fa->fa_off + off, (void *)src, len);
}

int flash_area_erase(const struct flash_area *fa, off_t off, size_t len)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(fa->fa_id)) {
		if (fa->api->erase) {
			return fa->api->erase(fa, off, len);
		}
		return -ENOTSUP;
	}
#endif
	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	return flash_erase(fa->fa_dev, fa->fa_off + off, len);
}

int flash_area_flatten(const struct flash_area *fa, off_t off, size_t len)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(fa->fa_id)) {
		if (fa->api->flatten) {
			return fa->api->flatten(fa, off, len);
		}
		return -ENOTSUP;
	}
#endif
	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	return flash_flatten(fa->fa_dev, fa->fa_off + off, len);
}

uint32_t flash_area_align(const struct flash_area *fa)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(fa->fa_id)) {
		if (fa->api->align) {
			return fa->api->align(fa);
		}
	}
	return 0;
#endif
	return flash_get_write_block_size(fa->fa_dev);
}

int flash_area_has_driver(const struct flash_area *fa)
{
	if (!device_is_ready(fa->fa_dev)) {
		return -ENODEV;
	}

	return 1;
}

const struct device *flash_area_get_device(const struct flash_area *fa)
{
	return fa->fa_dev;
}

#if CONFIG_FLASH_MAP_LABELS
const char *flash_area_label(const struct flash_area *fa)
{
	return fa->fa_label;
}
#endif

uint8_t flash_area_erased_val(const struct flash_area *fa)
{
#ifdef CONFIG_FLASH_MAP_CUSTOM_BACKEND
	if (HAS_CUSTOM_BACKEND_ID(fa->fa_id)) {
		if (fa->api->erased_val) {
			return fa->api->erased_val(fa);
		}
		return 0xff;
	}
#endif
	const struct flash_parameters *param;

	param = flash_get_parameters(fa->fa_dev);

	return param->erase_value;
}
