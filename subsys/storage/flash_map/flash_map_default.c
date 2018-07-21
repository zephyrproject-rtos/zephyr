/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <flash_map.h>

#define FLASH_AREA_OFFSET(__name)                                              \
	UTIL_CAT(FLASH_AREA_, UTIL_CAT(__name, _OFFSET))
#define FLASH_AREA_SIZE(__name) UTIL_CAT(FLASH_AREA_, UTIL_CAT(__name, _SIZE))

#define FLASH_AREA_FOO(i, _)                                                   \
	{.fa_id = i,                                                           \
	 .fa_device_id = FLASH_AREA_##i##_DEV_ID,                              \
	 .fa_off = FLASH_AREA_OFFSET(FLASH_AREA_##i),                          \
	 .fa_size = FLASH_AREA_SIZE(FLASH_AREA_##i)},

const struct flash_area default_flash_map[] = {
	UTIL_LISTIFY(FLASH_AREA_NUM, FLASH_AREA_FOO, ~)
};

const int flash_map_entries = ARRAY_SIZE(default_flash_map);
const struct flash_area *flash_map = default_flash_map;
