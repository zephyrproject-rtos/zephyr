/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include "dmm.h"

#define _FILTER_MEM(node_id, fn)                                                                   \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, zephyr_memory_attr), (fn(node_id)), ())
#define DT_MEMORY_REGION_FOREACH_STATUS_OKAY_NODE(fn)                                              \
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(_FILTER_MEM, fn)

#define __BUILD_LINKER_END_VAR(_name) DT_CAT3(__, _name, _end)
#define _BUILD_LINKER_END_VAR(node_id)                                                             \
	__BUILD_LINKER_END_VAR(DT_STRING_UNQUOTED(node_id, zephyr_memory_region))

#define _BUILD_LINKER_END_VAR2(node_id)                                                            \
	ROUND_UP(__BUILD_LINKER_END_VAR(DT_STRING_UNQUOTED(node_id, zephyr_memory_region)),        \
		 DMM_REG_ALIGN_SIZE(node_id))

#define _BUILD_MEM_REGION(node_id)                                                                 \
	{.dt_addr = DT_REG_ADDR(node_id),                                                          \
	 .dt_size = DT_REG_SIZE(node_id),                                                          \
	 .dt_attr = DT_PROP(node_id, zephyr_memory_attr),                                          \
	 .dt_align = DMM_REG_ALIGN_SIZE(node_id),                                                  \
	 .dt_allc = &_BUILD_LINKER_END_VAR(node_id),                                               \
	 },

#define MEM_REGION_ALIGNED(node_id)                                                                \
	BUILD_ASSERT(IS_ALIGNED(DT_REG_ADDR(node_id), DMM_REG_ALIGN_SIZE(node_id)));
DT_MEMORY_REGION_FOREACH_STATUS_OKAY_NODE(MEM_REGION_ALIGNED)

#define HEAP_NUM_WORDS (CONFIG_DMM_HEAP_CHUNKS / 32)
BUILD_ASSERT(IS_ALIGNED(CONFIG_DMM_HEAP_CHUNKS, 32));

/* Generate declarations of linker variables used to determine size of preallocated variables
 * stored in memory sections spanning over memory regions.
 * These are used to determine memory left for dynamic bounce buffer allocator to work with.
 */
#define _DECLARE_LINKER_VARS(node_id) extern uint32_t _BUILD_LINKER_END_VAR(node_id);
DT_MEMORY_REGION_FOREACH_STATUS_OKAY_NODE(_DECLARE_LINKER_VARS);

struct dmm_region {
	uintptr_t dt_addr;
	size_t dt_size;
	uint32_t dt_attr;
	uint32_t dt_align;
	void *dt_allc;
};

struct dmm_heap {
	uint32_t mask[HEAP_NUM_WORDS];
	uintptr_t ptr;
	uintptr_t ptr_end;
	size_t blk_size;
	const struct dmm_region *region;
	sys_bitarray_t bitarray;
};

static const struct dmm_region dmm_regions[] = {
	DT_MEMORY_REGION_FOREACH_STATUS_OKAY_NODE(_BUILD_MEM_REGION)
};

struct {
	struct dmm_heap dmm_heaps[ARRAY_SIZE(dmm_regions)];
} dmm_heaps_data;

static struct dmm_heap *dmm_heap_find(void *region)
{
	struct dmm_heap *dh;

	for (size_t idx = 0; idx < ARRAY_SIZE(dmm_heaps_data.dmm_heaps); idx++) {
		dh = &dmm_heaps_data.dmm_heaps[idx];
		if (dh->region->dt_addr == (uintptr_t)region) {
			return dh;
		}
	}

	return NULL;
}

static bool is_region_cacheable(const struct dmm_region *region)
{
	return (IS_ENABLED(CONFIG_DCACHE) && (region->dt_attr & DT_MEM_CACHEABLE));
}

static bool is_buffer_within_region(uintptr_t start, size_t size,
				    uintptr_t reg_start, size_t reg_size)
{
	return ((start >= reg_start) && ((start + size) <= (reg_start + reg_size)));
}

static bool is_user_buffer_correctly_preallocated(void const *user_buffer, size_t user_length,
					const struct dmm_region *region)
{
	uintptr_t addr = (uintptr_t)user_buffer;

	if (!is_buffer_within_region(addr, user_length, region->dt_addr, region->dt_size)) {
		return false;
	}

	if (!is_region_cacheable(region)) {
		/* Buffer is contained within non-cacheable region - use it as it is. */
		return true;
	}

	if (IS_ALIGNED(addr, region->dt_align)) {
		/* If buffer is in cacheable region it must be aligned to data cache line size. */
		return true;
	}

	return false;
}

static void *dmm_buffer_alloc(struct dmm_heap *dh, size_t length)
{
	size_t num_bits, off;
	int rv;

	if (dh->ptr == 0) {
		/* Not initialized. */
		return NULL;
	}

	length = ROUND_UP(length, dh->region->dt_align);
	num_bits = DIV_ROUND_UP(length, dh->blk_size);

	rv = sys_bitarray_alloc(&dh->bitarray, num_bits, &off);
	if (rv < 0) {
		return NULL;
	}

	return (void *)(dh->ptr + dh->blk_size * off);
}

static void dmm_buffer_free(struct dmm_heap *dh, void *buffer, size_t length)
{
	size_t num_bits;
	size_t offset = ((uintptr_t)buffer - dh->region->dt_addr) / dh->blk_size;

	length = ROUND_UP(length, dh->region->dt_align);
	num_bits = DIV_ROUND_UP(length, dh->blk_size);
	int rv = sys_bitarray_free(&dh->bitarray, num_bits, offset);

	(void)rv;
	__ASSERT_NO_MSG(rv == 0);
}

static void dmm_memcpy(void *dst, const void *src, size_t len)
{
#define IS_ALIGNED32(x) IS_ALIGNED(x, sizeof(uint32_t))
#define IS_ALIGNED64(x) IS_ALIGNED(x, sizeof(uint64_t))
	if (IS_ALIGNED64(len) && IS_ALIGNED64(dst) && IS_ALIGNED64(src)) {
		for (uint32_t i = 0; i < len / sizeof(uint64_t); i++) {
			((uint64_t *)dst)[i] = ((uint64_t *)src)[i];
		}
		return;
	}

	if (IS_ALIGNED32(len) && IS_ALIGNED32(dst) && IS_ALIGNED32(src)) {
		for (uint32_t i = 0; i < len / sizeof(uint32_t); i++) {
			((uint32_t *)dst)[i] = ((uint32_t *)src)[i];
		}
		return;
	}

	memcpy(dst, src, len);
}

int dmm_buffer_out_prepare(void *region, void const *user_buffer, size_t user_length,
			   void **buffer_out)
{
	struct dmm_heap *dh;

	if (user_length == 0) {
		/* Assume that zero-length buffers are correct as they are. */
		*buffer_out = (void *)user_buffer;
		return 0;
	}

	/* Get memory region that specified device can perform DMA transfers from */
	dh = dmm_heap_find(region);
	if (dh == NULL) {
		return -EINVAL;
	}

	/* Check if:
	 * - provided user buffer is already in correct memory region,
	 * - provided user buffer is aligned and padded to cache line,
	 *   if it is located in cacheable region.
	 */
	if (is_user_buffer_correctly_preallocated(user_buffer, user_length, dh->region)) {
		/* If yes, assign buffer_out to user_buffer*/
		*buffer_out = (void *)user_buffer;
	} else {
		/* If no:
		 * - dynamically allocate buffer in correct memory region that respects cache line
		 *   alignment and padding
		 */
		*buffer_out = dmm_buffer_alloc(dh, user_length);
		/* Return error if dynamic allocation fails */
		if (*buffer_out == NULL) {
			return -ENOMEM;
		}
		/* - copy user buffer contents into allocated buffer */
		dmm_memcpy(*buffer_out, user_buffer, user_length);
	}

	/* Check if device memory region is cacheable
	 * If yes, writeback all cache lines associated with output buffer
	 * (either user or allocated)
	 */
	if (is_region_cacheable(dh->region)) {
		sys_cache_data_flush_range(*buffer_out, user_length);
	}
	/* If no, no action is needed */

	return 0;
}

int dmm_buffer_out_release(void *region, void *buffer_out, size_t alloc_length)
{
	struct dmm_heap *dh;
	uintptr_t addr = (uintptr_t)buffer_out;

	/* Get memory region that specified device can perform DMA transfers from */
	dh = dmm_heap_find(region);
	if (dh == NULL) {
		return -EINVAL;
	}

	/* Check if output buffer is contained within memory area
	 * managed by dynamic memory allocator
	 */
	if (is_buffer_within_region(addr, alloc_length, dh->ptr, dh->ptr_end)) {
		/* If yes, free the buffer */
		dmm_buffer_free(dh, buffer_out, alloc_length);
	}
	/* If no, no action is needed */

	return 0;
}

int dmm_buffer_in_prepare(void *region, void *user_buffer, size_t user_length, void **buffer_in)
{
	struct dmm_heap *dh;

	if (user_length == 0) {
		/* Assume that zero-length buffers are correct as they are. */
		*buffer_in = (void *)user_buffer;
		return 0;
	}

	/* Get memory region that specified device can perform DMA transfers to */
	dh = dmm_heap_find(region);
	if (dh == NULL) {
		return -EINVAL;
	}

	/* Check if:
	 * - provided user buffer is already in correct memory region,
	 * - provided user buffer is aligned and padded to cache line,
	 *   if it is located in cacheable region.
	 */
	if (is_user_buffer_correctly_preallocated(user_buffer, user_length, dh->region)) {
		/* If yes, assign buffer_in to user_buffer */
		*buffer_in = user_buffer;
	} else {
		/* If no, dynamically allocate buffer in correct memory region that respects cache
		 * line alignment and padding
		 */
		*buffer_in = dmm_buffer_alloc(dh, user_length);
		/* Return error if dynamic allocation fails */
		if (*buffer_in == NULL) {
			return -ENOMEM;
		}
	}

	/* Check if device memory region is cacheable
	 * If yes, invalidate all cache lines associated with input buffer
	 * (either user or allocated) to clear potential dirty bits.
	 */
	if (is_region_cacheable(dh->region)) {
		sys_cache_data_invd_range(*buffer_in, user_length);
	}
	/* If no, no action is needed */

	return 0;
}

int dmm_buffer_in_release(void *region, void *user_buffer, size_t user_length, void *buffer_in,
			  size_t alloc_length)
{
	struct dmm_heap *dh;
	uintptr_t addr = (uintptr_t)buffer_in;

	/* Get memory region that specified device can perform DMA transfers to, using devicetree */
	dh = dmm_heap_find(region);
	if (dh == NULL) {
		return -EINVAL;
	}

	/* Check if device memory region is cacheable
	 * If yes, invalidate all cache lines associated with input buffer
	 * (either user or allocated)
	 */
	if (is_region_cacheable(dh->region)) {
		sys_cache_data_invd_range(buffer_in, user_length);
	}
	/* If no, no action is needed */

	/* Check if user buffer and allocated buffer points to the same memory location
	 * If no, copy allocated buffer to the user buffer
	 */
	if (buffer_in != user_buffer) {
		dmm_memcpy(user_buffer, buffer_in, user_length);
	}
	/* If yes, no action is needed */

	/* Check if input buffer is contained within memory area
	 * managed by dynamic memory allocator
	 */
	if (is_buffer_within_region(addr, user_length, dh->ptr, dh->ptr_end)) {
		/* If yes, free the buffer */
		dmm_buffer_free(dh, buffer_in, alloc_length);
	}
	/* If no, no action is needed */

	return 0;
}

int dmm_init(void)
{
	struct dmm_heap *dh;
	int blk_cnt;
	int heap_space;

	for (size_t idx = 0; idx < ARRAY_SIZE(dmm_regions); idx++) {
		dh = &dmm_heaps_data.dmm_heaps[idx];
		dh->region = &dmm_regions[idx];
		dh->ptr = ROUND_UP(dh->region->dt_allc, dh->region->dt_align);
		heap_space = dh->region->dt_size - (dh->ptr - dh->region->dt_addr);
		dh->blk_size = ROUND_UP(heap_space / (32 * HEAP_NUM_WORDS), dh->region->dt_align);
		blk_cnt = heap_space / dh->blk_size;
		dh->ptr_end = dh->ptr + blk_cnt * dh->blk_size;
		dh->bitarray.num_bits = blk_cnt;
		dh->bitarray.num_bundles = HEAP_NUM_WORDS;
		dh->bitarray.bundles = dh->mask;
	}

	return 0;
}
