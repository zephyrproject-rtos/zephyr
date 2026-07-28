/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_EXT2_H_
#define ZEPHYR_INCLUDE_FS_EXT2_H_

#include <stdint.h>

/** @brief Configuration used to format ext2 file system.
 *
 * If a field is set to 0 then its default value is used.
 * (For the volume name, the first byte of the array must be 0 to use the
 * default value.)
 */
struct ext2_cfg {
	/** Requested size of block, in bytes. */
	uint32_t block_size;
	/** Requested size of file system, as a number of blocks. If 0 then the
	 *  whole available memory is used.
	 */
	uint32_t fs_size;
	/** Requested amount of memory per inode, in bytes. It is used to
	 *  calculate the number of inodes in the created file system.
	 */
	uint32_t bytes_per_inode;
	/** UUID for the created file system. Used only when @ref set_uuid is true. */
	uint8_t uuid[16];
	/** Name for the created file system. If the first byte is 0 then the
	 *  default name "ext2" is used.
	 */
	uint8_t volume_name[16];
	/** If true then the UUID from @ref uuid is used in the created file
	 *  system. If false then a version 4 UUID is generated.
	 */
	bool set_uuid;
};

#define FS_EXT2_DECLARE_DEFAULT_CONFIG(name)			\
	static struct ext2_cfg name = {				\
		.block_size = 1024,				\
		.fs_size = 0x800000,				\
		.bytes_per_inode = 4096,			\
		.volume_name = {'e', 'x', 't', '2', '\0'},	\
		.set_uuid = false,				\
	}


#endif /* ZEPHYR_INCLUDE_FS_EXT2_H_ */
