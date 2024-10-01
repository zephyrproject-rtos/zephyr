/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>
#include <string.h>

static void *alloc_blocks(sys_mem_blocks_t *mem_block, size_t num_blocks)
{
	size_t offset;
	int r;
	uint8_t *blk;

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	k_spinlock_key_t  key = k_spin_lock(&mem_block->lock);
#endif

	/* Find an unallocated block */
	r = sys_bitarray_alloc(mem_block->bitmap, num_blocks, &offset);
	if (r != 0) {
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
		k_spin_unlock(&mem_block->lock, key);
#endif
		return NULL;
	}


#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	mem_block->info.used_blocks += (uint32_t)num_blocks;

	if (mem_block->info.max_used_blocks < mem_block->info.used_blocks) {
		mem_block->info.max_used_blocks = mem_block->info.used_blocks;
	}

	k_spin_unlock(&mem_block->lock, key);
#endif

	/* Calculate the start address of the newly allocated block */

	blk = mem_block->buffer + (offset << mem_block->info.blk_sz_shift);

	return blk;
}

static int free_blocks(sys_mem_blocks_t *mem_block, void *ptr,
		       size_t num_blocks)
{
	size_t offset;
	uint8_t *blk = ptr;
	int ret = 0;

	/* Make sure incoming block is within the mem_block buffer */
	if (blk < mem_block->buffer) {
		ret = -EFAULT;
		goto out;
	}

	offset = (blk - mem_block->buffer) >> mem_block->info.blk_sz_shift;
	if (offset >= mem_block->info.num_blocks) {
		ret = -EFAULT;
		goto out;
	}

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	k_spinlock_key_t  key = k_spin_lock(&mem_block->lock);
#endif
	ret = sys_bitarray_free(mem_block->bitmap, num_blocks, offset);

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	if (ret == 0) {
		mem_block->info.used_blocks -= (uint32_t)num_blocks;
	}

	k_spin_unlock(&mem_block->lock, key);
#endif

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

	if (count > mem_block->info.num_blocks) {
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
				   ptr, count << mem_block->info.blk_sz_shift);
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

	if (count > mem_block->info.num_blocks) {
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
					   ptr,
					   BIT(mem_block->info.blk_sz_shift));
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

int sys_mem_blocks_is_region_free(sys_mem_blocks_t *mem_block, void *in_block,
				  size_t count)
{
	bool result;
	size_t offset;

	__ASSERT_NO_MSG(mem_block != NULL);
	__ASSERT_NO_MSG(mem_block->bitmap != NULL);
	__ASSERT_NO_MSG(mem_block->buffer != NULL);

	offset = ((uint8_t *)in_block - mem_block->buffer) >>
		 mem_block->info.blk_sz_shift;

	__ASSERT_NO_MSG(offset + count <= mem_block->info.num_blocks);

	result = sys_bitarray_is_region_cleared(mem_block->bitmap, count,
						offset);
	return result;
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

	offset = ((uint8_t *)in_block - mem_block->buffer) >>
		 mem_block->info.blk_sz_shift;

	if (offset + count > mem_block->info.num_blocks) {
		/* Definitely not enough blocks to be allocated */
		ret = -ENOMEM;
		goto out;
	}

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	k_spinlock_key_t  key = k_spin_lock(&mem_block->lock);
#endif

	ret = sys_bitarray_test_and_set_region(mem_block->bitmap, count,
					       offset, true);

	if (ret != 0) {
#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
		k_spin_unlock(&mem_block->lock, key);
#endif
		ret = -ENOMEM;
		goto out;
	}

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
	mem_block->info.used_blocks += (uint32_t)count;

	if (mem_block->info.max_used_blocks < mem_block->info.used_blocks) {
		mem_block->info.max_used_blocks = mem_block->info.used_blocks;
	}

	k_spin_unlock(&mem_block->lock, key);
#endif

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(mem_block),
			in_block, count << mem_block->info.blk_sz_shift);
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

	if (count > mem_block->info.num_blocks) {
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
						  ptr, BIT(mem_block->info.blk_sz_shift));
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

	if (count > mem_block->info.num_blocks) {
		ret = -EINVAL;
		goto out;
	}

	ret = free_blocks(mem_block, block, count);

	if (ret != 0) {
		goto out;
	}
#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	heap_listener_notify_free(HEAP_ID_FROM_POINTER(mem_block),
			block, count << mem_block->info.blk_sz_shift);
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

	if (count > allocator->info.num_blocks) {
		ret = -ENOMEM;
		goto out;
	}

	ret = sys_mem_blocks_alloc(allocator, count, out_blocks);

	if ((ret == 0) && (blk_size != NULL)) {
		*blk_size = BIT(allocator->info.blk_sz_shift);
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
		end = start + (BIT(one_alloc->info.blk_sz_shift) *
			       one_alloc->info.num_blocks);

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

#ifdef CONFIG_SYS_MEM_BLOCKS_RUNTIME_STATS
int sys_mem_blocks_runtime_stats_get(sys_mem_blocks_t *mem_block,
				     struct sys_memory_stats *stats)
{
	if ((mem_block == NULL) || (stats == NULL)) {
		return -EINVAL;
	}

	stats->allocated_bytes = mem_block->info.used_blocks <<
				 mem_block->info.blk_sz_shift;
	stats->free_bytes = (mem_block->info.num_blocks <<
			     mem_block->info.blk_sz_shift) -
			    stats->allocated_bytes;
	stats->max_allocated_bytes = mem_block->info.max_used_blocks <<
				     mem_block->info.blk_sz_shift;

	return 0;
}

int sys_mem_blocks_runtime_stats_reset_max(sys_mem_blocks_t *mem_block)
{
	if (mem_block == NULL) {
		return -EINVAL;
	}

	mem_block->info.max_used_blocks = mem_block->info.used_blocks;

	return 0;
}
#endif

#ifdef CONFIG_OBJ_CORE_STATS_SYS_MEM_BLOCKS
static int sys_mem_blocks_stats_raw(struct k_obj_core *obj_core, void *stats)
{
	struct sys_mem_blocks *block;
	k_spinlock_key_t  key;

	block = CONTAINER_OF(obj_core, struct sys_mem_blocks, obj_core);

	key = k_spin_lock(&block->lock);

	memcpy(stats, &block->info, sizeof(block->info));

	k_spin_unlock(&block->lock, key);

	return 0;
}

static int sys_mem_blocks_stats_query(struct k_obj_core *obj_core, void *stats)
{
	struct sys_mem_blocks *block;
	k_spinlock_key_t  key;
	struct sys_memory_stats *ptr = stats;

	block = CONTAINER_OF(obj_core, struct sys_mem_blocks, obj_core);

	key = k_spin_lock(&block->lock);

	ptr->free_bytes = (block->info.num_blocks - block->info.used_blocks) <<
			  block->info.blk_sz_shift;
	ptr->allocated_bytes = block->info.used_blocks <<
			       block->info.blk_sz_shift;
	ptr->max_allocated_bytes = block->info.max_used_blocks <<
				   block->info.blk_sz_shift;

	k_spin_unlock(&block->lock, key);

	return 0;
}

static int sys_mem_blocks_stats_reset(struct k_obj_core *obj_core)
{
	struct sys_mem_blocks *block;
	k_spinlock_key_t  key;

	block = CONTAINER_OF(obj_core, struct sys_mem_blocks, obj_core);

	key = k_spin_lock(&block->lock);
	block->info.max_used_blocks = block->info.used_blocks;
	k_spin_unlock(&block->lock, key);

	return 0;
}

static struct k_obj_type obj_type_sys_mem_blocks;

static struct k_obj_core_stats_desc sys_mem_blocks_stats_desc = {
	.raw_size = sizeof(struct sys_mem_blocks_info),
	.query_size = sizeof(struct sys_memory_stats),
	.raw = sys_mem_blocks_stats_raw,
	.query = sys_mem_blocks_stats_query,
	.reset = sys_mem_blocks_stats_reset,
	.disable = NULL,
	.enable = NULL,
};
#endif

#ifdef CONFIG_OBJ_CORE_SYS_MEM_BLOCKS
static struct k_obj_type obj_type_sys_mem_blocks;

static int init_sys_mem_blocks_obj_core_list(void)
{
	/* Initialize the sys_mem_blocks object type */

	z_obj_type_init(&obj_type_sys_mem_blocks, K_OBJ_TYPE_MEM_BLOCK_ID,
			offsetof(struct sys_mem_blocks, obj_core));

#ifdef CONFIG_OBJ_CORE_STATS_SYS_MEM_BLOCKS
	k_obj_type_stats_init(&obj_type_sys_mem_blocks,
			      &sys_mem_blocks_stats_desc);
#endif

	/* Initialize statically defined sys_mem_blocks */

	STRUCT_SECTION_FOREACH_ALTERNATE(sys_mem_blocks_ptr,
					 sys_mem_blocks *, block_pp) {
		k_obj_core_init_and_link(K_OBJ_CORE(*block_pp),
					 &obj_type_sys_mem_blocks);
#ifdef CONFIG_OBJ_CORE_STATS_SYS_MEM_BLOCKS
		k_obj_core_stats_register(K_OBJ_CORE(*block_pp),
					  &(*block_pp)->info,
					  sizeof(struct sys_mem_blocks_info));
#endif
	}

	return 0;
}

SYS_INIT(init_sys_mem_blocks_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif
