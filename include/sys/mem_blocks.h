/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Memory Blocks Allocator
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEM_BLOCKS_H_
#define ZEPHYR_INCLUDE_SYS_MEM_BLOCKS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <kernel.h>
#include <math/ilog2.h>
#include <sys/bitarray.h>

/**
 * @defgroup mem_blocks_apis Memory Blocks APIs
 * @{
 */

/**
 * @brief Memory Blocks Allocator
 */
struct sys_mem_blocks;

/**
 * @typedef sys_mem_blocks_t
 *
 * @brief Memory Blocks Allocator
 */
typedef struct sys_mem_blocks sys_mem_blocks_t;

/**
 * @cond INTERNAL_HIDDEN
 */

struct sys_mem_blocks {
	/* Number of blocks */
	uint32_t num_blocks;

	/* Bit shift for block size */
	uint8_t blk_sz_shift;

	/* Memory block buffer */
	uint8_t *buffer;

	/* Bitmap of allocated blocks */
	sys_bitarray_t *bitmap;

};

/**
 * @def _SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF
 *
 * @brief Create a memory block object with a providing backing buffer.
 *
 * @param name     Name of the memory block object.
 * @param blk_sz   Size of each memory block (in bytes, power of 2).
 * @param num_blks Total number of memory blocks.
 * @param buf      Backing buffer of type uint8_t.
 * @param mbmod    Modifier to the memory block struct
 */
#define _SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks, buf, mbmod) \
	_SYS_BITARRAY_DEFINE(_sys_mem_blocks_bitmap_##name,		\
			     num_blks, mbmod);				\
	mbmod sys_mem_blocks_t name = {					\
		.num_blocks = num_blks,					\
		.blk_sz_shift = ilog2(blk_sz),				\
		.buffer = buf,						\
		.bitmap = &_sys_mem_blocks_bitmap_##name,		\
	}

/**
 * @def _SYS_MEM_BLOCKS_DEFINE
 *
 * @brief Create a memory block object with a new backing buffer.
 *
 * @param name     Name of the memory block object.
 * @param blk_sz   Size of each memory block (in bytes, power of 2).
 * @param num_blks Total number of memory blocks.
 * @param balign   Alignment of the memory block buffer (power of 2).
 * @param mbmod    Modifier to the memory block struct
 */
#define _SYS_MEM_BLOCKS_DEFINE(name, blk_sz, num_blks, balign, mbmod)	\
	mbmod uint8_t __noinit_named(sys_mem_blocks_buf_##name)		\
		__aligned(WB_UP(balign))				\
		_sys_mem_blocks_buf_##name[num_blks * WB_UP(blk_sz)];	\
	_SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks,	\
					   _sys_mem_blocks_buf_##name,	\
					   mbmod);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @def SYS_MEM_BLOCKS_DEFINE
 *
 * @brief Create a memory block object with a new backing buffer.
 *
 * @param name      Name of the memory block object.
 * @param blk_sz    Size of each memory block (in bytes).
 * @param num_blks  Total number of memory blocks.
 * @param buf_align Alignment of the memory block buffer (power of 2).
 */
#define SYS_MEM_BLOCKS_DEFINE(name, blk_sz, num_blks, buf_align) \
	_SYS_MEM_BLOCKS_DEFINE(name, blk_sz, num_blks, buf_align,)

/**
 * @def SYS_MEM_BLOCKS_DEFINE_STATIC
 *
 * @brief Create a static memory block object with a new backing buffer.
 *
 * @param name      Name of the memory block object.
 * @param blk_sz    Size of each memory block (in bytes).
 * @param num_blks  Total number of memory blocks.
 * @param buf_align Alignment of the memory block buffer (power of 2).
 */
#define SYS_MEM_BLOCKS_DEFINE_STATIC(name, blk_sz, num_blks, buf_align) \
	_SYS_MEM_BLOCKS_DEFINE(name, blk_sz, num_blks, buf_align, static)


/**
 * @def SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF
 *
 * @brief Create a memory block object with a providing backing buffer.
 *
 * @param name     Name of the memory block object.
 * @param blk_sz   Size of each memory block (in bytes).
 * @param num_blks Total number of memory blocks.
 * @param buf      Backing buffer of type uint8_t.
 */
#define SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks, buf) \
	_SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks, buf,)

/**
 * @def SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF
 *
 * @brief Create a static memory block object with a providing backing buffer.
 *
 * @param name     Name of the memory block object.
 * @param blk_sz   Size of each memory block (in bytes).
 * @param num_blks Total number of memory blocks.
 * @param buf      Backing buffer of type uint8_t.
 */
#define SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(name, blk_sz, num_blks, buf) \
	_SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks, buf, static)

/**
 * @brief Allocate multiple memory blocks
 *
 * Allocate multiple memory blocks, and place their pointers into
 * the output array.
 *
 * @param[in]  mem_block  Pointer to memory block object.
 * @param[in]  count      Number of blocks to allocate.
 * @param[out] out_blocks Output array to be populated by pointers to
 *                        the memory blocks. It must have at least
 *                        @p count elements.
 *
 * @retval 0       Successful
 * @retval -EINVAL Invalid argument supplied.
 * @retval -ENOMEM Not enough blocks for allocation.
 */
int sys_mem_blocks_alloc(sys_mem_blocks_t *mem_block, size_t count,
			 void **out_blocks);

/**
 * @brief Free multiple memory blocks
 *
 * Free multiple memory blocks according to the array of memory
 * block pointers.
 *
 * @param[in] mem_block Pointer to memory block object.
 * @param[in] count     Number of blocks to free.
 * @param[in] in_blocks Input array of pointers to the memory blocks.
 *
 * @retval 0       Successful
 * @retval -EINVAL Invalid argument supplied.
 * @retval -EFAULT Invalid pointers supplied.
 */
int sys_mem_blocks_free(sys_mem_blocks_t *mem_block, size_t count,
			void **in_blocks);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MEM_BLOCKS_H_ */
