/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_BLOCKS_H__
#define __EXT2_BLOCKS_H__

/* Flags for block structure. */
#define BLOCK_BACKEND_FLASH BIT(0)

/* Defined for future use */
#define BLOCK_BACKEND_ANON BIT(1)

struct ext2_data;

/* NOTE: Fields of block structure cannot be modified. Changes can be made only
 * to memory pointed by `memory` field.
 */
struct block {
	void *memory;
	uint32_t num;
	uint8_t backend;
};

/* Functions to read and write blocks to the memory */

/**
 * @brief Get block from memory
 *
 * If block is already cached in some page then no read from memory is
 * performed.
 *
 * NOTE: Block can be referenced only by one structure. There is no need to
 *       share blocks (inode structures may be shared and fs structures are
 *       unique during the runtime) hence it is forbidden. Thanks to this
 *       limitation structures of pages and blocks may be simpler.
 *
 * @param fs File system data.
 * @param num Number of block to read
 *
 * @returns Block structure or NULL on error
 */
struct block *get_block(struct ext2_data *fs, uint32_t num);

/**
 * @brief Free the block
 *
 * @param blk Block to be dropped
 *
 * NOTE: Block passed to this structure must be created with get_block);
 */
void drop_block(struct block *blk);

/**
 * @brief Set page of given block as dirty
 */
void block_set_dirty(struct block *blk);

/**
 * @brief Write all dirty pages to the memory
 *
 * @param fs File system data.
 *
 * @retval >=0 number of written pages
 * @retval <0 on error
 */
int sync_blocks(struct ext2_data *fs);

/**
 * @brief Initialize internal structures of blocks cache.
 *
 * The block size and write size must be obtained from file system (or configuration) and from
 * storage device. The write_size must be greater than or equal to the block_size (other cases are
 * not supported).
 *
 * @returns 0 on success or error code on failure.
 */
int init_blocks_cache(uint32_t block_size, uint32_t write_size);

/**
 * @brief Clear internal structures.
 *
 * There are no operations made to sync contents in blocks cache with memory. It has to be done
 * before this function is called.
 *
 * NOTE: If any page is still used when this function is called it will cause panic.
 */
void close_blocks_cache(void);

#endif /* __EXT2_BLOCKS_H__ */
