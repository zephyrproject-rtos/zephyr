/*
 * Copyright (c) 2019 Bolt Innovation Management, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_LITTLEFS_H_
#define ZEPHYR_INCLUDE_FS_LITTLEFS_H_

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

#include <lfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Filesystem info structure for LittleFS mount */
struct fs_littlefs {
	/* Defaulted in driver, customizable before mount. */
	struct lfs_config cfg;

	/* Must be cfg.cache_size */
	uint8_t *read_buffer;

	/* Must be cfg.cache_size */
	uint8_t *prog_buffer;

	/* Must be cfg.lookahead_size/4 elements, and
	 * cfg.lookahead_size must be a multiple of 8.
	 */
	uint32_t *lookahead_buffer[CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE / sizeof(uint32_t)];

	/* These structures are filled automatically at mount. */
	struct lfs lfs;
	void *backend;
	struct k_mutex mutex;
};

/** @brief Define a littlefs configuration with customized size
 * characteristics.
 *
 * This defines static arrays required for caches, and initializes the
 * littlefs configuration structure to use the provided values instead
 * of the global Kconfig defaults.  A pointer to the named object must
 * be stored in the ``.fs_data`` field of a :c:type:`struct fs_mount`
 * object.
 *
 * To define an instance for the Kconfig defaults, use
 * :c:macro:`FS_LITTLEFS_DECLARE_DEFAULT_CONFIG`.
 *
 * To completely control file system configuration the application can
 * directly define and initialize a :c:type:`struct fs_littlefs`
 * object.  The application is responsible for ensuring the configured
 * values are consistent with littlefs requirements.
 *
 * @note If you use a non-default configuration for cache size, you
 * must also select @kconfig{CONFIG_FS_LITTLEFS_FC_HEAP_SIZE} to relax
 * the size constraints on per-file cache allocations.
 *
 * @param name the name for the structure.  The defined object has
 * file scope.
 * @param read_sz see @kconfig{CONFIG_FS_LITTLEFS_READ_SIZE}
 * @param prog_sz see @kconfig{CONFIG_FS_LITTLEFS_PROG_SIZE}
 * @param cache_sz see @kconfig{CONFIG_FS_LITTLEFS_CACHE_SIZE}
 * @param lookahead_sz see @kconfig{CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE}
 */
#define FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(name, read_sz, prog_sz, cache_sz, lookahead_sz) \
	static uint8_t __aligned(4) name ## _read_buffer[cache_sz];			  \
	static uint8_t __aligned(4) name ## _prog_buffer[cache_sz];			  \
	static uint32_t name ## _lookahead_buffer[(lookahead_sz) / sizeof(uint32_t)];		  \
	static struct fs_littlefs name = {						  \
		.cfg = {								  \
			.read_size = (read_sz),						  \
			.prog_size = (prog_sz),						  \
			.cache_size = (cache_sz),					  \
			.lookahead_size = (lookahead_sz),				  \
			.read_buffer = name ## _read_buffer,				  \
			.prog_buffer = name ## _prog_buffer,				  \
			.lookahead_buffer = name ## _lookahead_buffer,			  \
		},									  \
	}

/** @brief Define a littlefs configuration with default characteristics.
 *
 * This defines static arrays and initializes the littlefs
 * configuration structure to use the default size configuration
 * provided by Kconfig.
 *
 * @param name the name for the structure.  The defined object has
 * file scope.
 */
#define FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(name)			 \
	FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(name,				 \
					  CONFIG_FS_LITTLEFS_READ_SIZE,	 \
					  CONFIG_FS_LITTLEFS_PROG_SIZE,	 \
					  CONFIG_FS_LITTLEFS_CACHE_SIZE, \
					  CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_LITTLEFS_H_ */
