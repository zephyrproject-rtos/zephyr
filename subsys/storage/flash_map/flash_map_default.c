/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <storage/flash_map.h>

#define FLASH_AREA_FOO(i, _)				\
	{.fa_id = i,					\
	 .fa_off = DT_FLASH_AREA_##i##_OFFSET,		\
	 .fa_dev_name = DT_FLASH_AREA_##i##_DEV,	\
	 .fa_size = DT_FLASH_AREA_##i##_SIZE,},

const struct flash_area default_flash_map[] = {
	UTIL_LISTIFY(DT_FLASH_AREA_NUM, FLASH_AREA_FOO, ~)
};

const int flash_map_entries = ARRAY_SIZE(default_flash_map);
const struct flash_area *flash_map = default_flash_map;
