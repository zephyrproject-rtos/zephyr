/*
 * Copyright (c) 2019 Bolt Innovation Management, LLC
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
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

typedef int (*lfs_device_open)(void **context, uintptr_t area_id);
typedef void (*lfs_device_close)(void *context);
typedef size_t (*lfs_device_get_block_size)(void *context);
typedef size_t (*lfs_device_get_size)(void *context);
typedef int (*lfs_device_read)(const struct lfs_config *c, lfs_block_t block,
	lfs_off_t off, void *buffer, lfs_size_t size);
typedef int (*lfs_device_program)(const struct lfs_config *c, lfs_block_t block,
	lfs_off_t off, const void *buffer, lfs_size_t size);
typedef int (*lfs_device_erase)(const struct lfs_config *c, lfs_block_t block);
typedef int (*lfs_device_sync)(const struct lfs_config *c);

/**
 * @brief device operations for LittleFS
 *
 * If they are not set the system will default to flash operations.
 */
struct fs_littlefs_device_services {
	lfs_device_open open;
	lfs_device_close close;
	lfs_device_get_block_size get_block_size;
	lfs_device_get_block_size get_size;
	lfs_device_read read;
	lfs_device_program program;
	lfs_device_erase erase;
	lfs_device_sync sync;
};

/**
 * @brief context set in lfs config to be able to get back to the device services
 */
struct fs_littlefs_context {
	/* operations used to access the underlying device */
	struct fs_littlefs_device_services device_services;
	/* context provided by external user */
	void *external_context;
};

/** @brief Filesystem info structure for LittleFS mount */
struct fs_littlefs {
	/* Defaulted in driver, customizable before mount. */
	struct lfs_config cfg;

	/* Must be cfg.cache_size */
	uint8_t *read_buffer;

	/* Must be cfg.cache_size */
	uint8_t *prog_buffer;

	/* handed over to lfs_config, contains all necessary operations */
	struct fs_littlefs_context lfs_context;

	/* Mustbe cfg.lookahead_size/4 elements, and
	 * cfg.lookahead_size must be a multiple of 8.
	 */
	uint32_t *lookahead_buffer[CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE / sizeof(uint32_t)];

	/* These structures are filled automatically at mount. */
	struct lfs lfs;
	struct k_mutex mutex;
};

int fs_littlefs_flash_open(void **context, uintptr_t area_id);
void fs_littlefs_flash_close(void *context);
size_t fs_littlefs_flash_get_block_size(void *context);
size_t fs_littlefs_flash_get_size(void *context);
int fs_littlefs_flash_read(const struct lfs_config *c, lfs_block_t block,
			lfs_off_t off, void *buffer, lfs_size_t size);
int fs_littlefs_flash_program(const struct lfs_config *c, lfs_block_t block,
			lfs_off_t off, const void *buffer, lfs_size_t size);
int fs_littlefs_flash_erase(const struct lfs_config *c, lfs_block_t block);
int fs_littlefs_flash_sync(const struct lfs_config *c);
int fs_littlefs_read(const struct lfs_config *c, lfs_block_t block,
			lfs_off_t off, void *buffer, lfs_size_t size);
int fs_littlefs_program(const struct lfs_config *c, lfs_block_t block,
			lfs_off_t off, const void *buffer, lfs_size_t size);
int fs_littlefs_erase(const struct lfs_config *c, lfs_block_t block);
int fs_littlefs_sync(const struct lfs_config *c);

/** @brief Define a littlefs configuration with customized size
 * characteristics and custom operations for the actual device access.
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
 * @param program_sz see @kconfig{CONFIG_FS_LITTLEFS_PROG_SIZE}
 * @param cache_sz see @kconfig{CONFIG_FS_LITTLEFS_CACHE_SIZE}
 * @param lookahead_sz see @kconfig{CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE}
 * @param read_op function pointer to read operation
 * @param program_op function pointer to program operation
 * @param erase_op function pointer to erase operation
 * @param sync_op function pointer to sync operation
 * @param open_op function pointer to open device operation
 * @param close_op function pointer to close device operation
 * @param get_block_size_op function pointer to get device block size operation
 * @param get_size_op function pointer to get device size operation
 */
#define FS_LITTLEFS_DECLARE_CUSTOM_CONFIG_CUSTOM_OPS(name, read_sz, program_sz, cache_sz, \
		lookahead_sz, read_op, program_op, erase_op, sync_op, \
		open_op, close_op, get_block_size_op, get_size_op) \
	static uint8_t __aligned(4) name ## _read_buffer[cache_sz]; \
	static uint8_t __aligned(4) name ## _prog_buffer[cache_sz]; \
	static uint32_t name ## _lookahead_buffer[(lookahead_sz) / sizeof(uint32_t)]; \
	static struct fs_littlefs name = { \
		.cfg = { \
			.read_size = (read_sz), \
			.prog_size = (program_sz), \
			.cache_size = (cache_sz), \
			.lookahead_size = (lookahead_sz), \
			.read_buffer = name ## _read_buffer, \
			.prog_buffer = name ## _prog_buffer, \
			.lookahead_buffer = name ## _lookahead_buffer, \
			.read = fs_littlefs_read, \
			.prog = fs_littlefs_program, \
			.erase = fs_littlefs_erase, \
			.sync = fs_littlefs_sync, \
		}, \
		.lfs_context = { \
			.external_context = 0, \
			.device_services = { \
				.open = open_op, \
				.close = close_op, \
				.get_block_size = get_block_size_op, \
				.get_size = get_size_op, \
				.read = read_op, \
				.program = program_op, \
				.erase = erase_op, \
				.sync = sync_op, \
			}, \
		}, \
	}

/** @brief Define a littlefs configuration with customized size
 * characteristics.
 *
 * For more details see :c:macro:`FS_LITTLEFS_DECLARE_CUSTOM_CONFIG_CUSTOM_OPS`.
 *
 * @param name the name for the structure.  The defined object has
 * file scope.
 * @param read_sz see @kconfig{CONFIG_FS_LITTLEFS_READ_SIZE}
 * @param program_sz see @kconfig{CONFIG_FS_LITTLEFS_PROG_SIZE}
 * @param cache_sz see @kconfig{CONFIG_FS_LITTLEFS_CACHE_SIZE}
 * @param lookahead_sz see @kconfig{CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE}
 */
#define FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(name, read_sz, program_sz, cache_sz, lookahead_sz) \
	BUILD_ASSERT(IS_ENABLED(CONFIG_FS_LITTLEFS_FLASH_BACKEND)); \
	FS_LITTLEFS_DECLARE_CUSTOM_CONFIG_CUSTOM_OPS(name, \
					  read_sz, \
					  program_sz, \
					  cache_sz, \
					  lookahead_sz, \
					  fs_littlefs_flash_read, \
					  fs_littlefs_flash_program, \
					  fs_littlefs_flash_erase, \
					  fs_littlefs_flash_sync, \
					  fs_littlefs_flash_open, \
					  fs_littlefs_flash_close, \
					  fs_littlefs_flash_get_block_size, \
					  fs_littlefs_flash_get_size)

/** @brief Define a littlefs configuration with customized size
 * characteristics.
 *
 * For more details see :c:macro:`FS_LITTLEFS_DECLARE_CUSTOM_CONFIG_CUSTOM_OPS`.
 *
 * @param name the name for the structure.  The defined object has
 * file scope.
 * @param read_op function pointer to read operation
 * @param program_op function pointer to program operation
 * @param erase_op function pointer to erase operation
 * @param sync_op function pointer to sync operation
 * @param open_op function pointer to open device operation
 * @param close_op function pointer to close device operation
 * @param get_block_size_op function pointer to get device block size operation
 * @param get_size_op function pointer to get device size operation
 */
#define FS_LITTLEFS_DECLARE_DEFAULT_CONFIG_CUSTOM_OPS(name, read_op, program_op, erase_op, \
		sync_op, open_op, close_op, get_block_size_op, get_size_op) \
	FS_LITTLEFS_DECLARE_CUSTOM_CONFIG_CUSTOM_OPS(name, \
					  CONFIG_FS_LITTLEFS_READ_SIZE, \
					  CONFIG_FS_LITTLEFS_PROG_SIZE, \
					  CONFIG_FS_LITTLEFS_CACHE_SIZE, \
					  CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE, \
					  read_op, \
					  program_op, \
					  erase_op, \
					  sync_op, \
					  open_op, \
					  close_op, \
					  get_block_size_op, \
					  get_size_op)

/** @brief Define a littlefs configuration with default characteristics.
 *
 * This defines static arrays and initializes the littlefs
 * configuration structure to use the default size configuration
 * provided by Kconfig.
 *
 * @param name the name for the structure.  The defined object has
 * file scope.
 */
#define FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(name) \
	FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(name, \
					  CONFIG_FS_LITTLEFS_READ_SIZE, \
					  CONFIG_FS_LITTLEFS_PROG_SIZE, \
					  CONFIG_FS_LITTLEFS_CACHE_SIZE, \
					  CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_LITTLEFS_H_ */
