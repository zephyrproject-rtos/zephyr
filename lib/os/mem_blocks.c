/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>

static void *alloc_blocks(sys_mem_blocks_t *mem_block, size_t num_blocks)
{
	size_t offset;
	int r;
	uint8_t *blk;
	void *ret = NULL;

	/* Find an unallocated block */
	r = sys_bitarray_alloc(mem_block->bitmap, num_blocks, &offset);
	if (r != 0) {
		goto out;
	}

	/* Calculate the start address of the newly allocated block */
	blk = mem_block->buffer + (offset << mem_block->blk_sz_shift);

	ret = blk;

out:
	return ret;
}

static int free_blocks(sys_mem_blocks_t *mem_block, void *ptr, size_t num_blocks)
{
	size_t offset;
	uint8_t *blk = ptr;
	int ret = 0;

	/* Make sure incoming block is within the mem_block buffer */
	if (blk < mem_block->buffer) {
		ret = -EFAULT;
		goto out;
	}

	offset = (blk - mem_block->buffer) >> mem_block->blk_sz_shift;
	if (offset >= mem_block->num_blocks) {
		ret = -EFAULT;
		goto out;
	}

	ret = sys_bitarray_free(mem_block->bitmap, num_blocks, offset);

out:
	return ret;
}

int sys_mem_blocks_alloc_contiguous(sys_mem_blocks_t *mem_block, size_t count,
				   void **out_block)
{
	int ret = 0;

	__ASSERT_NO_MSG(mem_block != NULL);
	__ASSERT_NO_MSG(out_block != NULL);

	if (count == 0) {
		/* Nothing to allocate */
		*out_block = NULL;
		goto out;
	}

	if (count > mem_block->num_blocks) {
		/* Definitely not enough blocks to be allocated */
		ret = -ENOMEM;
		goto out;
	}

	void *ptr = alloc_blocks(mem_block, count);

	if (ptr == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	*out_block = ptr;
#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(mem_block),
				   ptr, count << mem_block->blk_sz_shift);
#endif

out:
		return ret;
}

int sys_mem_blocks_alloc(sys_mem_blocks_t *mem_block, size_t count,
			 void **out_blocks)
{
	int ret = 0;
	int i;

	__ASSERT_NO_MSG(mem_block != NULL);
	__ASSERT_NO_MSG(out_blocks != NULL);
	__ASSERT_NO_MSG(mem_block->bitmap != NULL);
	__ASSERT_NO_MSG(mem_block->buffer != NULL);

	if (count == 0) {
		/* Nothing to allocate */
		goto out;
	}

	if (count > mem_block->num_blocks) {
		/* Definitely not enough blocks to be allocated */
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < count; i++) {
		void *ptr = alloc_blocks(mem_block, 1);

		if (ptr == NULL) {
			break;
		}

		out_blocks[i] = ptr;

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
		heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(mem_block),
					   ptr, BIT(mem_block->blk_sz_shift));
#endif
	}

	/* If error, free already allocated blocks. */
	if (i < count) {
		(void)sys_mem_blocks_free(mem_block, i, out_blocks);
		ret = -ENOMEM;
	}

out:
	return ret;
}

int sys_mem_blocks_get(sys_mem_blocks_t *mem_block, void *in_block, size_t count)
{
	int ret = 0;
	int offset;

	__ASSERT_NO_MSG(mem_block != NULL);
	__ASSERT_NO_MSG(mem_block->bitmap != NULL);
	__ASSERT_NO_MSG(mem_block->buffer != NULL);

	if (count == 0) {
		/* Nothing to allocate */
		goto out;
	}

	offset = ((uint8_t *)in_block - mem_block->buffer) >> mem_block->blk_sz_shift;

	if (offset + count > mem_block->num_blocks) {
		/* Definitely not enough blocks to be allocated */
		ret = -ENOMEM;
		goto out;
	}

	ret = sys_bitarray_test_and_set_region(mem_block->bitmap, count, offset, true);
	if (ret != 0) {
		ret = -ENOMEM;
		goto out;
	}

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(mem_block),
			in_block, count << mem_block->blk_sz_shift);
#endif

out:
	return ret;
}


int sys_mem_blocks_free(sys_mem_blocks_t *mem_block, size_t count,
			void **in_blocks)
{
	int ret = 0;
	int i;

	__ASSERT_NO_MSG(mem_block != NULL);
	__ASSERT_NO_MSG(in_blocks != NULL);
	__ASSERT_NO_MSG(mem_block->bitmap != NULL);
	__ASSERT_NO_MSG(mem_block->buffer != NULL);

	if (count == 0) {
		/* Nothing to be freed. */
		goto out;
	}

	if (count > mem_block->num_blocks) {
		ret = -EINVAL;
		goto out;
	}

	for (i = 0; i < count; i++) {
		void *ptr = in_blocks[i];

		int r = free_blocks(mem_block, ptr, 1);

		if (r != 0) {
			ret = r;
		}
#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
		else {
			/*
			 * Since we do not keep track of failed free ops,
			 * we need to notify free one-by-one, instead of
			 * notifying at the end of function.
			 */
			heap_listener_notify_free(HEAP_ID_FROM_POINTER(mem_block),
						  ptr, BIT(mem_block->blk_sz_shift));
		}
#endif
	}

out:
	return ret;
}

int sys_mem_blocks_free_contiguous(sys_mem_blocks_t *mem_block, void *block, size_t count)
{
	int ret = 0;

	__ASSERT_NO_MSG(mem_block != NULL);
	__ASSERT_NO_MSG(mem_block->bitmap != NULL);
	__ASSERT_NO_MSG(mem_block->buffer != NULL);

	if (count == 0) {
		/* Nothing to be freed. */
		goto out;
	}

	if (count > mem_block->num_blocks) {
		ret = -EINVAL;
		goto out;
	}

	ret = free_blocks(mem_block, block, count);

	if (ret != 0) {
		goto out;
	}
#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	heap_listener_notify_free(HEAP_ID_FROM_POINTER(mem_block),
			block, count << mem_block->blk_sz_shift);
#endif

out:
	return ret;
}

void sys_multi_mem_blocks_init(sys_multi_mem_blocks_t *group,
			       sys_multi_mem_blocks_choice_fn_t choice_fn)
{
	group->num_allocators = 0;
	group->choice_fn = choice_fn;
}

void sys_multi_mem_blocks_add_allocator(sys_multi_mem_blocks_t *group,
					sys_mem_blocks_t *alloc)
{
	__ASSERT_NO_MSG(group->num_allocators < ARRAY_SIZE(group->allocators));

	group->allocators[group->num_allocators++] = alloc;
}

int sys_multi_mem_blocks_alloc(sys_multi_mem_blocks_t *group,
			       void *cfg, size_t count,
			       void **out_blocks,
			       size_t *blk_size)
{
	sys_mem_blocks_t *allocator;
	int ret = 0;

	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(out_blocks != NULL);

	if (count == 0) {
		if (blk_size != NULL) {
			*blk_size = 0;
		}
		goto out;
	}

	allocator = group->choice_fn(group, cfg);
	if (allocator == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (count > allocator->num_blocks) {
		ret = -ENOMEM;
		goto out;
	}

	ret = sys_mem_blocks_alloc(allocator, count, out_blocks);

	if ((ret == 0) && (blk_size != NULL)) {
		*blk_size = BIT(allocator->blk_sz_shift);
	}

out:
	return ret;
}

int sys_multi_mem_blocks_free(sys_multi_mem_blocks_t *group,
			      size_t count, void **in_blocks)
{
	int i;
	int ret = 0;
	sys_mem_blocks_t *allocator = NULL;

	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(in_blocks != NULL);

	if (count == 0) {
		goto out;
	}

	for (i = 0; i < group->num_allocators; i++) {
		/*
		 * Find out which allocator the allocated blocks
		 * belong to.
		 */

		uint8_t *start, *end;
		sys_mem_blocks_t *one_alloc;

		one_alloc = group->allocators[i];
		start = one_alloc->buffer;
		end = start + (BIT(one_alloc->blk_sz_shift) * one_alloc->num_blocks);

		if (((uint8_t *)in_blocks[0] >= start) &&
		    ((uint8_t *)in_blocks[0] < end)) {
			allocator = one_alloc;
			break;
		}
	}

	if (allocator != NULL) {
		ret = sys_mem_blocks_free(allocator, count, in_blocks);
	} else {
		ret = -EINVAL;
	}

out:
	return ret;
}
