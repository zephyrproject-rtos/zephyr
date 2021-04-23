/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for flash map
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_PRIV_H_
#define ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

extern const struct flash_area *flash_map;
extern const int flash_map_entries;

static inline bool flash_area_is_in_area_bounds(const struct flash_area *fa,
					   off_t off, size_t len)
{
	return (off >= 0) && ((off + len) <= fa->fa_size);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_PRIV_H_ */
