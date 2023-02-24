/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_IMPL_H__
#define __EXT2_IMPL_H__

#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>

#include "ext2_struct.h"

/* Memory allocation for ext2 implementation */
void *ext2_heap_alloc(size_t size);
void  ext2_heap_free(void *ptr);

/* Initialization of disk storage. */
int ext2_init_disk_access_backend(struct ext2_data *fs, const void *storage_dev, int flags);

/**
 * @brief Get block from the disk.
 */
struct ext2_block *ext2_get_block(struct ext2_data *fs, uint32_t block);

/**
 * @brief Free the block structure.
 */
void ext2_drop_block(struct ext2_block *b);

/**
 * @brief Write block to the disk.
 */
int ext2_sync_block(struct ext2_data *fs, struct ext2_block *b);

void ext2_init_blocks_slab(struct ext2_data *fs);

/* FS operations */

/**
 * @brief Initialize structure with data needed to access the storage device
 *
 * @param fs File system data structure to initialize
 * @param storage_dev Pointer to storage
 * @param flags Additional flags (e.g. RO flag)
 *
 * @retval 0 on success
 * @retval -EINVAL when superblock of ext2 was not detected
 * @retval -ENOTSUP when described file system is not supported
 * @retval <0 other error
 */
int ext2_init_storage(struct ext2_data **fsp, const void *storage_dev, int flags);

/**
 * @brief Verify superblock of file system
 *
 * Checks if file system is supported by the implementation.
 * @retval 0 when superblock is valid
 * @retval -EROFS when superblock is not valid but file system may be mounted read only
 * @retval -EINVAL when superblock is not valid and file system cannot be mounted at all
 * @retval -ENOTSUP when superblock has some field set to value that we don't support
 */
int ext2_verify_superblock(struct ext2_disk_superblock *sb);

/**
 * @brief Initialize all data needed to perform operations on file system
 *
 * Fetches the superblock. Initializes structure fields.
 */
int ext2_init_fs(struct ext2_data *fs);

/**
 * @brief Clear the data used by file system implementation
 *
 */
int ext2_close_fs(struct ext2_data *fs);

/**
 * @brief Clear the data used to communicate with storage device
 *
 */
int ext2_close_struct(struct ext2_data *fs);

/**
 * @brief Create the ext2 file system
 *
 * This function uses functions stored in `ext2_data` structure to create new
 * file system on storage device.
 *
 * NOTICE: fs structure must be first initialized with `ext2_init_fs` function.
 *
 * After this function succeeds the `ext2_clean` function must be called.
 *
 * @param fs File system data (must be initialized before)
 *
 * @retval 0 on success
 * @retval -ENOSPC when storage device is too small for ext2 file system
 * @retval -ENOTSUP when storage device is too big (file systems with more than
 *         8192 blocks are not supported)
 */
int ext2_format(struct ext2_data *fs, struct ext2_cfg *cfg);

#endif /* __EXT2_IMPL_H__ */
