/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ext2 file system API
 */
#ifndef ZEPHYR_INCLUDE_FS_EXT2_H_
#define ZEPHYR_INCLUDE_FS_EXT2_H_

#include <stdint.h>

/**
 * @defgroup ext2 ext2 file system API
 * @ingroup file_system_storage
 * @{
 */

/**
 * @brief Configuration used to format ext2 file system.
 *
 * @note If a field is set to 0 then default value is used (see @ref
 * FS_EXT2_DECLARE_DEFAULT_CONFIG).
 */
struct ext2_cfg {
	/**
	 * Requested block size.
	 */
	uint32_t block_size;
	/**
	 * Requested file system size (expressed in blocks).
	 */
	uint32_t fs_size;
	/**
	 * Requested memory for each inode.
	 *
	 * This is used to calculate number of inodes in created file system.
	 */
	uint32_t bytes_per_inode;
	/**
	 * UUID for created file system.
	 *
	 * Used when @ref set_uuid field is set to true.
	 */
	uint8_t uuid[16];
	/**
	 * Name for created file system.
	 *
	 * @note If first byte is 0, the name "ext2" is given.
	 */
	uint8_t volume_name[17];
	/**
	 * Flag indicating if UUID should be set.
	 *
	 * If true then UUID from @ref uuid field is used in created file system.
	 * If false then UUID (ver4) is generated.
	 */
	bool set_uuid;
};

/**
 * @brief Default configuration for new ext2 file system.
 */
#define FS_EXT2_DECLARE_DEFAULT_CONFIG(name)			\
	static struct ext2_cfg name = {				\
		.block_size = 1024,				\
		.fs_size = 0x800000,				\
		.bytes_per_inode = 4096,			\
		.volume_name = {'e', 'x', 't', '2', '\0'},	\
		.set_uuid = false,				\
	}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_FS_EXT2_H_ */
