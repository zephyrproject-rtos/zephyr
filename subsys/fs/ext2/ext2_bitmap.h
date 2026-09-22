/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_BITMAP_H__
#define __EXT2_BITMAP_H__

#include <stdint.h>
#include <stdbool.h>

/* Functions to make operations on bitmaps.
 *
 * NOTICE: Assumed size of the bitmap is 256 B (1024 bits).
 *         (Hence, the greatest valid index is 1023.)
 */

/**
 * @brief Set bit at given index to one
 *
 * @param bm Pointer to bitmap
 * @param index Index in bitmap
 * @param size Size of bitmap in bytes
 *
 * @retval 0 on success;
 * @retval -EINVAL when index is too big;
 */
int ext2_bitmap_set(uint8_t *bm, uint32_t index, uint32_t size);

/**
 * @brief Set bit at given index to zero
 *
 * @param bm Pointer to bitmap
 * @param index Index in bitmap
 * @param size Size of bitmap in bytes
 *
 * @retval 0 on success;
 * @retval -EINVAL when index is too big;
 */
int ext2_bitmap_unset(uint8_t *bm, uint32_t index, uint32_t size);

/**
 * @brief Find first bit set to zero in bitmap
 *
 * @param bm Pointer to bitmap
 * @param size Size of bitmap in bytes
 *
 * @retval >0 found inode number;
 * @retval -ENOSPC when not found;
 */
int32_t ext2_bitmap_find_free(uint8_t *bm, uint32_t size);

/**
 * @brief Helper function to count bits set in bitmap
 *
 * @param bm Pointer to bitmap
 * @param size Size of bitmap in bits
 *
 * @retval Number of set bits in bitmap;
 */
uint32_t ext2_bitmap_count_set(uint8_t *bm, uint32_t size);

#endif /* __EXT2_BITMAP_H__ */
