/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_BITFIELD_H_
#define ZEPHYR_INCLUDE_SYS_BITFIELD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// Compiler HANGS before crashing <= 32
#define Z_BITFIELD_DECLARE(bitfield, num_bits)				\
	static unsigned long bitfield[] =				\
			{				\
				[0 ... ((num_bits / (sizeof(long) * 8)) - 1) +				\
					((num_bits % (sizeof(long) * 8)) ? 1 : 0)				\
			 	] = ~0				\
			};

/**
 * Reserve contiguous bits in the bitfield
 *
 * If a contiguous region is found, it will be marked as in-use.
 *
 * @param bitfield Bitfield to examine
 * @param bitfield_size Number of bits in the bitfield
 * @param region_size Number of bits to allocate
 * @param offset Output parameter indicating offset (in bits) within the bitfield containing sufficient space
 * @retval 0 Success
 * @retval -1 No free space in the bitfield for the requested allocation
 */
int z_bitfield_alloc(void *bitfield, size_t bitfield_size, size_t region_size, size_t *offset);

/**
 * Release in-use bits in a bitfield
 *
 * A 'region_size' number of bits starting at bit position 'offset' will be marked as
 * not being in use any more.
 * @param bitfield Bitfield to manipulate
 * @param region_size Number of bits to mark as not in use
 * @param offset Starting bit position to mark as not in use
 */
void z_bitfield_free(void *bitfield, size_t region_size, size_t offset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_BITFIELD_H_ */