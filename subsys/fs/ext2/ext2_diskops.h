/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_DISKOPS_H__
#define __EXT2_DISKOPS_H__

#include <stdint.h>

#include "ext2_struct.h"

/**
 * @brief Fetch inode into given buffer.
 *
 * @param fs File system data
 * @param ino Inode number
 * @param inode Buffer for fetched inode
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_fetch_inode(struct ext2_data *fs, uint32_t ino, struct ext2_inode *inode);

/**
 * @brief Fetch block into buffer in the inode structure.
 *
 * @param inode Inode structure
 * @param block Number of inode block to fetch (0 - first block in that inode)
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_fetch_inode_block(struct ext2_inode *inode, uint32_t block);

/**
 * @brief Fetch block group into buffer in fs structure.
 *
 * If the group was already fetched then this function has no effect.
 * If the group was not fetched but the block that holds this group is present in
 * the internal buffer, then the block isn't fetched again.
 *
 * @param fs File system data
 * @param group Block group number
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_fetch_block_group(struct ext2_data *fs, uint32_t group);

/*
 * @brief Fetch one block of inode table into internal buffer
 *
 * If the block of inode table was already fetched then this function has no
 * effect.
 *
 * @param bg Block group structure
 * @param block Number of inode table block to fetch (relative to start of the inode table)
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_fetch_bg_itable(struct ext2_bgroup *bg, uint32_t block);

/**
 * @brief Fetch inode bitmap into internal buffer
 *
 * If the bitmap was already fetched then this function has no effect.
 *
 * @param bg Block group structure
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_fetch_bg_ibitmap(struct ext2_bgroup *bg);

/**
 * @brief Fetch block bitmap into internal buffer
 *
 * If the bitmap was already fetched then this function has no effect.
 *
 * @param bg Block group structure
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_fetch_bg_bbitmap(struct ext2_bgroup *bg);

/**
 * @brief Clear the entry in the inode table of given inode
 *
 * This function triggers fetching of block group and inode table (where the
 * inode is described).
 *
 * @param fs File system data
 * @param inode Inode number of inode to clear
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_clear_inode(struct ext2_data *fs, uint32_t ino);

/**
 * @brief Commit changes made to inode structure
 *
 * The changes are committed only to the cached block. They are truly written to
 * storage when sync is performed.
 *
 * @param inode Inode structure
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_commit_inode(struct ext2_inode *inode);

/**
 * @brief Commit changes made to inode block
 *
 * The changes are committed only to the cached block. They are truly written to
 * storage when sync is performed.
 *
 * @param inode Inode structure
 *
 * @retval 0 on success
 * @retval <0 error
 */
int ext2_commit_inode_block(struct ext2_inode *inode);


/* Operations that reserve or free the block or inode in the file system. They
 * mark an inode or block as used in the bitmap and change free inode/block
 * count in superblock and block group.
 */

/**
 * @brief Reserve a block for future use.
 *
 * Search for a free block. If block is found, proper fields in superblock and
 * block group are updated and block is marked as used in block bitmap.
 *
 * @param fs File system data
 *
 * @retval >0 number of allocated block
 * @retval <0 error
 */
int64_t ext2_alloc_block(struct ext2_data *fs);

/**
 * @brief Reserve an inode for future use.
 *
 * Search for free inode. If inode is found, proper fields in superblock and
 * block group are updated and inode is marked as used in inode bitmap.
 *
 * @param fs File system data
 *
 * @retval >0 number of allocated inode
 * @retval <0 error
 */
int32_t ext2_alloc_inode(struct ext2_data *fs);

#endif /* __EXT2_DISKOPS_H__ */
