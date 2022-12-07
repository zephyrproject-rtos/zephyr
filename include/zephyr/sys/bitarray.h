/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_BITARRAY_H_
#define ZEPHYR_INCLUDE_SYS_BITARRAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

struct sys_bitarray {
	/* Number of bits */
	uint32_t num_bits;

	/* Number of bundles */
	uint32_t num_bundles;

	/* Bundle of bits */
	uint32_t *bundles;

	/* Spinlock guarding access to this bit array */
	struct k_spinlock lock;
};

typedef struct sys_bitarray sys_bitarray_t;

/**
 * @brief Create a bitarray object.
 *
 * @param name Name of the bitarray object.
 * @param total_bits Total number of bits in this bitarray object.
 * @param sba_mod Modifier to the bitarray variables.
 */
#define _SYS_BITARRAY_DEFINE(name, total_bits, sba_mod)			\
	sba_mod uint32_t _sys_bitarray_bundles_##name			\
		[(((total_bits + 8 - 1) / 8) + sizeof(uint32_t) - 1)	\
		 / sizeof(uint32_t)] = {0};				\
	sba_mod sys_bitarray_t name = {					\
		.num_bits = total_bits,					\
		.num_bundles = (((total_bits + 8 - 1) / 8)		\
				+ sizeof(uint32_t) - 1)			\
			       / sizeof(uint32_t),			\
		.bundles = _sys_bitarray_bundles_##name,		\
	}

/**
 * @brief Create a bitarray object.
 *
 * @param name Name of the bitarray object.
 * @param total_bits Total number of bits in this bitarray object.
 */
#define SYS_BITARRAY_DEFINE(name, total_bits)				\
	_SYS_BITARRAY_DEFINE(name, total_bits,)

/**
 * @brief Create a static bitarray object.
 *
 * @param name Name of the bitarray object.
 * @param total_bits Total number of bits in this bitarray object.
 */
#define SYS_BITARRAY_DEFINE_STATIC(name, total_bits)			\
	_SYS_BITARRAY_DEFINE(name, total_bits, static)

/**
 * Set a bit in a bit array
 *
 * @param[in] bitarray Bitarray struct
 * @param[in] bit      The bit to be set
 *
 * @retval 0       Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to set exceeds
 *                 the number of bits in bit array, etc.)
 */
int sys_bitarray_set_bit(sys_bitarray_t *bitarray, size_t bit);

/**
 * Clear a bit in a bit array
 *
 * @param[in] bitarray Bitarray struct
 * @param[in] bit      The bit to be cleared
 *
 * @retval 0       Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to clear exceeds
 *                 the number of bits in bit array, etc.)
 */
int sys_bitarray_clear_bit(sys_bitarray_t *bitarray, size_t bit);

/**
 * Test whether a bit is set or not
 *
 * @param[in]  bitarray Bitarray struct
 * @param[in]  bit      The bit to be tested
 * @param[out] val      The value of the bit (0 or 1)
 *
 * @retval 0       Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to test exceeds
 *                 the number of bits in bit array, etc.)
 */
int sys_bitarray_test_bit(sys_bitarray_t *bitarray, size_t bit, int *val);

/**
 * Test the bit and set it
 *
 * @param[in]  bitarray Bitarray struct
 * @param[in]  bit      The bit to be tested and set
 * @param[out] prev_val Previous value of the bit (0 or 1)
 *
 * @retval 0       Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to test exceeds
 *                 the number of bits in bit array, etc.)
 */
int sys_bitarray_test_and_set_bit(sys_bitarray_t *bitarray, size_t bit, int *prev_val);

/**
 * Test the bit and clear it
 *
 * @param[in]  bitarray Bitarray struct
 * @param[in]  bit      The bit to be tested and cleared
 * @param[out] prev_val Previous value of the bit (0 or 1)
 *
 * @retval 0       Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to test exceeds
 *                 the number of bits in bit array, etc.)
 */
int sys_bitarray_test_and_clear_bit(sys_bitarray_t *bitarray, size_t bit, int *prev_val);

/**
 * Allocate bits in a bit array
 *
 * This finds a number of bits (@p num_bits) in a contiguous of
 * previously unallocated region. If such a region exists, the bits are
 * marked as allocated and the offset to the start of this region is
 * returned via @p offset.
 *
 * @param[in]  bitarray Bitarray struct
 * @param[in]  num_bits Number of bits to allocate
 * @param[out] offset   Offset to the start of allocated region if
 *                      successful
 *
 * @retval 0       Allocation successful
 * @retval -EINVAL Invalid argument (e.g. allocating more bits than
 *                 the bitarray has, trying to allocate 0 bits, etc.)
 * @retval -ENOSPC No contiguous region big enough to accommodate
 *                 the allocation
 */
int sys_bitarray_alloc(sys_bitarray_t *bitarray, size_t num_bits,
		       size_t *offset);

/**
 * Free bits in a bit array
 *
 * This marks the number of bits (@p num_bits) starting from @p offset
 * as no longer allocated.
 *
 * @param bitarray Bitarray struct
 * @param num_bits Number of bits to free
 * @param offset   Starting bit position to free
 *
 * @retval 0       Free is successful
 * @retval -EINVAL Invalid argument (e.g. try to free more bits than
 *                 the bitarray has, trying to free 0 bits, etc.)
 * @retval -EFAULT The bits in the indicated region are not all allocated.
 */
int sys_bitarray_free(sys_bitarray_t *bitarray, size_t num_bits,
		      size_t offset);

/**
 * Test if bits in a region is all set.
 *
 * This tests if the number of bits (@p num_bits) in region starting
 * from @p offset are all set.
 *
 * @param bitarray Bitarray struct
 * @param num_bits Number of bits to test
 * @param offset   Starting bit position to test
 *
 * @retval true    All bits are set.
 * @retval false   Not all bits are set.
 */
bool sys_bitarray_is_region_set(sys_bitarray_t *bitarray, size_t num_bits,
				size_t offset);

/**
 * Test if bits in a region is all cleared.
 *
 * This tests if the number of bits (@p num_bits) in region starting
 * from @p offset are all cleared.
 *
 * @param bitarray Bitarray struct
 * @param num_bits Number of bits to test
 * @param offset   Starting bit position to test
 *
 * @retval true    All bits are cleared.
 * @retval false   Not all bits are cleared.
 */
bool sys_bitarray_is_region_cleared(sys_bitarray_t *bitarray, size_t num_bits,
				    size_t offset);

/**
 * Set all bits in a region.
 *
 * This sets the number of bits (@p num_bits) in region starting
 * from @p offset.
 *
 * @param bitarray Bitarray struct
 * @param num_bits Number of bits to test
 * @param offset   Starting bit position to test
 *
 * @retval 0       Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to set exceeds
 *                 the number of bits in bit array, etc.)
 */
int sys_bitarray_set_region(sys_bitarray_t *bitarray, size_t num_bits,
			    size_t offset);

/**
 * Test if all bits in a region are cleared/set and set/clear them
 * in a single atomic operation
 *
 * This checks if all the bits (@p num_bits) in region starting
 * from @p offset are in required state. If even one bit is not,
 * -EEXIST is returned.  If the whole region is set/cleared
 * it is set to opposite state. The check and set is performed as a single
 * atomic operation.
 *
 * @param bitarray Bitarray struct
 * @param num_bits Number of bits to test and set
 * @param offset   Starting bit position to test and set
 * @param to_set   if true the region will be set if all bits are cleared
 *		   if false the region will be cleard if all bits are set
 *
 * @retval 0	   Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to set exceeds
 *		   the number of bits in bit array, etc.)
 * @retval -EEXIST at least one bit in the region is set/cleared,
 *		   operation cancelled
 */
int sys_bitarray_test_and_set_region(sys_bitarray_t *bitarray, size_t num_bits,
				     size_t offset, bool to_set);

/**
 * Clear all bits in a region.
 *
 * This clears the number of bits (@p num_bits) in region starting
 * from @p offset.
 *
 * @param bitarray Bitarray struct
 * @param num_bits Number of bits to test
 * @param offset   Starting bit position to test
 *
 * @retval 0       Operation successful
 * @retval -EINVAL Invalid argument (e.g. bit to set exceeds
 *                 the number of bits in bit array, etc.)
 */
int sys_bitarray_clear_region(sys_bitarray_t *bitarray, size_t num_bits,
			      size_t offset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_BITARRAY_H_ */
