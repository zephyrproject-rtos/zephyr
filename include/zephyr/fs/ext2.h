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
 * If a field is set to 0 then default value is used.
 * (In volume name the first cell of an array must be 0 to use default value.)
 *
 * @param block_size Requested size of block.
 * @param fs_size Requested size of file system. If 0 then whole available memory is used.
 * @param bytes_per_inode Requested memory for one inode. It is used to calculate number of inodes
 *	in created file system.
 * @param uuid UUID for created file system. Used when set_uuid is true.
 * @param volume_name Name for created file system.
 * @param set_uuid If true then UUID from that structure is used in created file system.
 *	If false then UUID (ver4) is generated.
 */
struct ext2_cfg {
	uint32_t block_size;
	uint32_t fs_size; /* Number of blocks that we want to take. */
	uint32_t bytes_per_inode;
	uint8_t uuid[16];
	uint8_t volume_name[17]; /* If first byte is 0 then name ext2" is given. */
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
