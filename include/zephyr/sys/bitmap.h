/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the bitmap API.
 * @ingroup sys_bitmap
 */

#ifndef ZEPHYR_INCLUDE_SYS_BITMAP_H_
#define ZEPHYR_INCLUDE_SYS_BITMAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys_bitmap Bitmap
 * @ingroup datastructure_apis
 * @since 4.6
 * @version 0.1.0
 *
 * @brief Arrays of bits in plain memory, without atomicity or locking.
 *
 * Bit @c n of a bitmap is bit <tt>n % 8</tt> of byte <tt>n / 8</tt>, the layout of
 * little-endian bit masks in protocol messages, so these functions also work on such
 * byte arrays. They take no lock: the caller serializes access. The single-bit
 * functions do not check bounds; the caller passes them only bit numbers below the
 * size of the bitmap.
 *
 * @{
 */

/**
 * @brief Number of bytes of a bitmap with @p num_bits bits.
 *
 * @param num_bits Number of bits.
 */
#define SYS_BITMAP_SIZE(num_bits) DIV_ROUND_UP(num_bits, BITS_PER_BYTE)

/**
 * @brief Define a bitmap.
 *
 * Defines an array of bytes that holds at least @p num_bits bits. At file scope the
 * bits start cleared; inside a function they start uninitialized.
 *
 * @param name Name of the bitmap.
 * @param num_bits Number of bits.
 */
#define SYS_BITMAP_DEFINE(name, num_bits) uint8_t name[SYS_BITMAP_SIZE(num_bits)]

/**
 * @brief Test a bit.
 *
 * @param bitmap Bitmap.
 * @param bit Bit number, starting from 0.
 *
 * @retval true The bit is set.
 * @retval false The bit is cleared.
 */
static inline bool sys_bitmap_test_bit(const uint8_t *bitmap, size_t bit)
{
	return (bitmap[bit / BITS_PER_BYTE] & BIT(bit % BITS_PER_BYTE)) != 0U;
}

/**
 * @brief Set a bit.
 *
 * @param bitmap Bitmap.
 * @param bit Bit number, starting from 0.
 */
static inline void sys_bitmap_set_bit(uint8_t *bitmap, size_t bit)
{
	bitmap[bit / BITS_PER_BYTE] |= (uint8_t)BIT(bit % BITS_PER_BYTE);
}

/**
 * @brief Clear a bit.
 *
 * @param bitmap Bitmap.
 * @param bit Bit number, starting from 0.
 */
static inline void sys_bitmap_clear_bit(uint8_t *bitmap, size_t bit)
{
	bitmap[bit / BITS_PER_BYTE] &= (uint8_t)~BIT(bit % BITS_PER_BYTE);
}

/**
 * @brief Set a bit to a given value.
 *
 * @param bitmap Bitmap.
 * @param bit Bit number, starting from 0.
 * @param val true to set the bit, false to clear it.
 */
static inline void sys_bitmap_set_bit_to(uint8_t *bitmap, size_t bit, bool val)
{
	if (val) {
		sys_bitmap_set_bit(bitmap, bit);
	} else {
		sys_bitmap_clear_bit(bitmap, bit);
	}
}

/**
 * @brief Set a bit and return its old value.
 *
 * @param bitmap Bitmap.
 * @param bit Bit number, starting from 0.
 *
 * @retval true The bit was already set.
 * @retval false The bit was cleared.
 */
static inline bool sys_bitmap_test_and_set_bit(uint8_t *bitmap, size_t bit)
{
	bool old = sys_bitmap_test_bit(bitmap, bit);

	sys_bitmap_set_bit(bitmap, bit);

	return old;
}

/**
 * @brief Clear a bit and return its old value.
 *
 * @param bitmap Bitmap.
 * @param bit Bit number, starting from 0.
 *
 * @retval true The bit was set.
 * @retval false The bit was already cleared.
 */
static inline bool sys_bitmap_test_and_clear_bit(uint8_t *bitmap, size_t bit)
{
	bool old = sys_bitmap_test_bit(bitmap, bit);

	sys_bitmap_clear_bit(bitmap, bit);

	return old;
}

/**
 * @brief Find the next set bit.
 *
 * @param bitmap Bitmap.
 * @param num_bits Number of bits in the bitmap.
 * @param start Bit number to start the search from.
 *
 * @return Number of the first set bit at or after @p start, or @p num_bits if there is
 *         none.
 */
size_t sys_bitmap_find_next_set(const uint8_t *bitmap, size_t num_bits, size_t start);

/**
 * @brief Find the next cleared bit.
 *
 * @param bitmap Bitmap.
 * @param num_bits Number of bits in the bitmap.
 * @param start Bit number to start the search from.
 *
 * @return Number of the first cleared bit at or after @p start, or @p num_bits if
 *         there is none.
 */
size_t sys_bitmap_find_next_clear(const uint8_t *bitmap, size_t num_bits, size_t start);

/**
 * @brief Count the set bits.
 *
 * @param bitmap Bitmap.
 * @param num_bits Number of bits in the bitmap.
 *
 * @return Number of set bits below @p num_bits.
 */
size_t sys_bitmap_count_set(const uint8_t *bitmap, size_t num_bits);

/**
 * @brief Iterate over the set bits of a bitmap, in ascending order.
 *
 * @param bitmap Bitmap.
 * @param num_bits Number of bits in the bitmap.
 * @param bit Variable of type size_t that holds the number of the current bit.
 */
#define SYS_BITMAP_FOR_EACH_SET_BIT(bitmap, num_bits, bit)                                         \
	for ((bit) = sys_bitmap_find_next_set(bitmap, num_bits, 0); (bit) < (num_bits);            \
	     (bit) = sys_bitmap_find_next_set(bitmap, num_bits, (bit) + 1U))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_BITMAP_H_ */
