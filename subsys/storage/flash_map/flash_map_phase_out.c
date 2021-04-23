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

int flash_area_open(uint8_t id, const struct flash_area **fap)
{
	extern const struct flash_area __flash_map_list_start[];
	extern const struct flash_area __flash_map_list_end[];

	if ((uint8_t)(__flash_map_list_end - __flash_map_list_start) == 0) {
		return -EACCES;
	} else if (id >= (uint8_t)(__flash_map_list_end - __flash_map_list_start)) {
		fap = NULL;
		return -ENOENT;
	}

	*fap = &__flash_map_list_start[id];
	return 0;
}

int flash_area_has_driver(const struct flash_area *fa)
{
	return fa->fa_dev == NULL ? -ENODEV : 0;
}

const struct device *flash_area_get_device(const struct flash_area *fa)
{
	return fa->fa_dev;
}
