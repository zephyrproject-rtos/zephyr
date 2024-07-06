/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_STORAGE_FLASH_MAP_PRIV_H_
#define ZEPHYR_SUBSYS_STORAGE_FLASH_MAP_PRIV_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

extern const struct flash_area *flash_map;
extern const int flash_map_entries;

static inline struct flash_area const *get_flash_area_from_id(int idx)
{
	for (int i = 0; i < flash_map_entries; i++) {
		if (flash_map[i].fa_id == idx) {
			return &flash_map[i];
		}
	}

	return NULL;
}


static inline bool is_in_flash_area_bounds(const struct flash_area *fa,
					   k_off_t off, size_t len)
{
	return (off >= 0) && ((off + len) <= fa->fa_size);
}

#endif /* ZEPHYR_SUBSYS_STORAGE_FLASH_MAP_PRIV_H_ */
