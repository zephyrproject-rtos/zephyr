/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MALIGN
/**
 * @brief Force compiler to place memory at-least on a x-byte boundary
 * @details Compiler extension. Supported by GCC and Clang
 */
#define MALIGN(x) __attribute__((aligned(x)))
#endif

#ifndef MROUND
/**
 * @brief Round up to nearest multiple of 4, for unsigned integers
 * @details
 *   The addition of 3 forces x into the next multiple of 4. This is responsible
 *   for the rounding in the the next step, to be Up.
 *   For ANDing of ~3: We observe y & (~3) == (y>>2)<<2, and we recognize
 *   (y>>2) as a floored division, which is almost undone by the left-shift. The
 *   flooring can't be undone so have achieved a rounding.
 *
 *   Examples:
 *    MROUND( 0) =  0
 *    MROUND( 1) =  4
 *    MROUND( 2) =  4
 *    MROUND( 3) =  4
 *    MROUND( 4) =  4
 *    MROUND( 5) =  8
 *    MROUND( 8) =  8
 *    MROUND( 9) = 12
 *    MROUND(13) = 16
 */
#define MROUND(x) (((uint32_t)(x)+3) & (~((uint32_t)3)))
#endif

/* When memory is free in the mem_pool, first few bytes are reserved for
 * maintaining the next pointer and free count.
 */
#define MEM_FREE_MEMBER_NEXT_SIZE  (sizeof(void *))
#define MEM_FREE_MEMBER_COUNT_SIZE (sizeof(uint16_t))
#define MEM_FREE_RESERVED_SIZE     ((MEM_FREE_MEMBER_NEXT_SIZE) + \
				    (MEM_FREE_MEMBER_COUNT_SIZE))

/* Define to check if a structure member is safe to be accessed when the memory
 * is in the mem_pool free list. I.e. whether a structure member be safely
 * accessed after the mem being freed.
 */
#define MEM_FREE_MEMBER_ACCESS_BUILD_ASSERT(type, member) \
	BUILD_ASSERT(offsetof(type, member) >= MEM_FREE_RESERVED_SIZE, \
		     "Possible unsafe member access inside free mem pool")

void mem_init(void *mem_pool, uint16_t mem_size, uint16_t mem_count, void **mem_head);
void *mem_acquire(void **mem_head);
void mem_release(void *mem, void **mem_head);

uint16_t mem_free_count_get(void *mem_head);
void *mem_get(const void *mem_pool, uint16_t mem_size, uint16_t index);
uint16_t mem_index_get(const void *mem, const void *mem_pool, uint16_t mem_size);

void mem_rcopy(uint8_t *dst, uint8_t const *src, uint16_t len);
uint8_t mem_nz(uint8_t *src, uint16_t len);

uint32_t mem_ut(void);
