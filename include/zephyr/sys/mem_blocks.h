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

#include <zephyr/kernel.h>
#include <zephyr/math/ilog2.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/mem_stats.h>

#define MAX_MULTI_ALLOCATORS 8

/**
 * @defgroup mem_blocks_apis Memory Blocks APIs
 * @{
 */

/**
 * @brief Memory Blocks Allocator
 */
struct sys_mem_blocks;

/**
 * @brief Multi Memory Blocks Allocator
 */
struct sys_multi_mem_blocks;

/**
 * @typedef sys_mem_blocks_t
 *
 * @brief Memory Blocks Allocator
 */
typedef struct sys_mem_blocks sys_mem_blocks_t;

/**
 * @typedef sys_multi_mem_blocks_t
 *
 * @brief Multi Memory Blocks Allocator
 */
typedef struct sys_multi_mem_blocks sys_multi_mem_blocks_t;

/**
 * @brief Multi memory blocks allocator choice function
 *
 * This is a user-provided functions whose responsibility is selecting
 * a specific memory blocks allocator based on the opaque cfg value,
 * which is specified by the user as an argument to
 * sys_multi_mem_blocks_alloc(). The callback returns a pointer to
 * the chosen allocator where the allocation is performed.
 *
 * NULL may be returned, which will cause the
 * allocation to fail and a -EINVAL reported to the calling code.
 *
 * @param group Multi memory blocks allocator structure.
 * @param cfg   An opaque user-provided value. It may be interpreted in
 *              any way by the application.
 *
 * @return A pointer to the chosen allocator, or NULL if none is chosen.
 */
typedef sys_mem_blocks_t *(*sys_multi_mem_blocks_choice_fn_t)
	(struct sys_multi_mem_blocks *group, void *cfg);

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

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	/* Spinlock guarding access to memory block internals */
	struct k_spinlock  lock;

	uint32_t used_blocks;
	uint32_t max_used_blocks;
#endif

};

struct sys_multi_mem_blocks {
	/* Number of allocators in this group */
	int num_allocators;
	sys_multi_mem_blocks_choice_fn_t choice_fn;
	sys_mem_blocks_t *allocators[MAX_MULTI_ALLOCATORS];
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
 * @brief Allocate a contiguous set of memory blocks
 *
 * Allocate multiple memory blocks, and place their pointers into
 * the output array.
 *
 * @param[in]  mem_block Pointer to memory block object.
 * @param[in]  count     Number of blocks to allocate.
 * @param[out] out_block Output pointer to the start of the allocated block set
 *
 * @retval 0       Successful
 * @retval -EINVAL Invalid argument supplied.
 * @retval -ENOMEM Not enough contiguous blocks for allocation.
 */
int sys_mem_blocks_alloc_contiguous(sys_mem_blocks_t *mem_block, size_t count,
				   void **out_block);

/**
 * @brief Force allocation of a specified blocks in a memory block object
 *
 * Allocate a specified blocks in a memory block object.
 * Note: use caution when mixing sys_mem_blocks_get and sys_mem_blocks_alloc,
 * allocation may take any of the free memory space
 *
 *
 * @param[in]  mem_block  Pointer to memory block object.
 * @param[in]  in_block   Address of the first required block to allocate
 * @param[in]  count      Number of blocks to allocate.
 *
 * @retval 0       Successful
 * @retval -EINVAL Invalid argument supplied.
 * @retval -ENOMEM Some of blocks are taken and cannot be allocated
 */
int sys_mem_blocks_get(sys_mem_blocks_t *mem_block, void *in_block, size_t count);

/**
 * @brief check if the region is free
 *
 * @param[in]  mem_block  Pointer to memory block object.
 * @param[in]  in_block   Address of the first block to check
 * @param[in]  count      Number of blocks to check.
 *
 * @retval 1 All memory blocks are free
 * @retval 0 At least one of the memory blocks is taken
 */
int sys_mem_blocks_is_region_free(sys_mem_blocks_t *mem_block, void *in_block, size_t count);

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

/**
 * @brief Free contiguous multiple memory blocks
 *
 * Free contiguous multiple memory blocks
 *
 * @param[in] mem_block Pointer to memory block object.
 * @param[in] block     Pointer to the first memory block
 * @param[in] count     Number of blocks to free.
 *
 * @retval 0       Successful
 * @retval -EINVAL Invalid argument supplied.
 * @retval -EFAULT Invalid pointer supplied.
 */
int sys_mem_blocks_free_contiguous(sys_mem_blocks_t *mem_block, void *block, size_t count);

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
/**
 * @brief Get the runtime statistics of a memory block
 *
 * This function retrieves the runtime stats for the specified memory block
 * @a mem_block and copies it into the memory pointed to by @a stats.
 *
 * @param mem_block Pointer to system memory block
 * @param stats Pointer to struct to copy statistics into
 *
 * @return -EINVAL if NULL pointer was passed, otherwise 0
 */
int sys_mem_blocks_runtime_stats_get(sys_mem_blocks_t *mem_block,
				     struct sys_memory_stats *stats);

/**
 * @brief Reset the maximum memory block usage
 *
 * This routine resets the maximum memory usage in the specified memory
 * block @a mem_block to match that block's current memory usage.
 *
 * @param mem_block Pointer to system memory block
 *
 * @return -EINVAL if NULL pointer was passed, otherwise 0
 */
int sys_mem_blocks_runtime_stats_reset_max(sys_mem_blocks_t *mem_block);
#endif

/**
 * @brief Initialize multi memory blocks allocator group
 *
 * Initialize a sys_multi_mem_block struct with the specified choice
 * function. Note that individual allocator must be added later with
 * sys_multi_mem_blocks_add_allocator.
 *
 * @param group     Multi memory blocks allocator structure.
 * @param choice_fn A sys_multi_mem_blocks_choice_fn_t callback used to
 *                  select the allocator to be used at allocation time
 */
void sys_multi_mem_blocks_init(sys_multi_mem_blocks_t *group,
			       sys_multi_mem_blocks_choice_fn_t choice_fn);

/**
 * @brief Add an allocator to an allocator group
 *
 * This adds a known allocator to an existing multi memory blocks
 * allocator group.
 *
 * @param group Multi memory blocks allocator structure.
 * @param alloc Allocator to add
 */
void sys_multi_mem_blocks_add_allocator(sys_multi_mem_blocks_t *group,
					sys_mem_blocks_t *alloc);

/**
 * @brief Allocate memory from multi memory blocks allocator group
 *
 * Just as for sys_mem_blocks_alloc(), allocates multiple blocks of
 * memory. Takes an opaque configuration pointer passed to the choice
 * function, which is used by integration code to choose an allocator.
 *
 * @param[in]  group      Multi memory blocks allocator structure.
 * @param[in]  cfg        Opaque configuration parameter,
 *                        as for sys_multi_mem_blocks_choice_fn_t
 * @param[in]  count      Number of blocks to allocate
 * @param[out] out_blocks Output array to be populated by pointers to
 *                        the memory blocks. It must have at least
 *                        @p count elements.
 * @param[out] blk_size   If not NULL, output the block size of
 *                        the chosen allocator.
 *
 * @retval 0       Successful
 * @retval -EINVAL Invalid argument supplied, or no allocator chosen.
 * @retval -ENOMEM Not enough blocks for allocation.
 */
int sys_multi_mem_blocks_alloc(sys_multi_mem_blocks_t *group,
			       void *cfg, size_t count,
			       void **out_blocks,
			       size_t *blk_size);

/**
 * @brief Free memory allocated from multi memory blocks allocator group
 *
 * Free previous allocated memory blocks from sys_multi_mem_blocks_alloc().
 *
 * Note that all blocks in @p in_blocks must be from the same allocator.
 *
 * @param[in] group     Multi memory blocks allocator structure.
 * @param[in] count     Number of blocks to free.
 * @param[in] in_blocks Input array of pointers to the memory blocks.
 *
 * @retval 0       Successful
 * @retval -EINVAL Invalid argument supplied, or no allocator chosen.
 * @retval -EFAULT Invalid pointer(s) supplied.
 */
int sys_multi_mem_blocks_free(sys_multi_mem_blocks_t *group,
			      size_t count, void **in_blocks);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MEM_BLOCKS_H_ */
