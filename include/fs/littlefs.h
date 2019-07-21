/*
 * Copyright (c) 2019 Bolt Innovation Management, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_LITTLEFS_H_
#define ZEPHYR_INCLUDE_FS_LITTLEFS_H_

#include <zephyr/types.h>
#include <kernel.h>
#include <storage/flash_map.h>

#include <lfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Filesystem info structure for LittleFS mount */
struct fs_littlefs {
	/* These structures are filled automatically at mount. */
	struct lfs lfs;
	struct lfs_config cfg;
	const struct flash_area *area;
	struct k_mutex mutex;

	/* Static buffers */
	u8_t read_buffer[CONFIG_FS_LITTLEFS_CACHE_SIZE];
	u8_t prog_buffer[CONFIG_FS_LITTLEFS_CACHE_SIZE];
	/* Multiple of 8 bytes, but 4-byte aligned (littlefs #239) */
	u32_t lookahead_buffer[CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE / sizeof(u32_t)];
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_LITTLEFS_H_ */
