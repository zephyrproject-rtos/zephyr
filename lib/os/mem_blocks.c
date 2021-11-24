/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/check.h>
#include <sys/mem_blocks.h>
#include <sys/util.h>

static void *alloc_one(sys_mem_blocks_t *mem_block)
{
	size_t offset;
	int r;
	uint8_t *blk;
	void *ret = NULL;

	/* Find an unallocated block */
	r = sys_bitarray_alloc(mem_block->bitmap, 1, &offset);
	if (r != 0) {
		goto out;
	}

	/* Calculate the start address of the newly allocated block */
	blk = mem_block->buffer + (offset << mem_block->blk_sz_shift);

	ret = blk;

out:
	return ret;
}

static int free_one(sys_mem_blocks_t *mem_block, void *ptr)
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

	ret = sys_bitarray_free(mem_block->bitmap, 1, offset);

out:
	return ret;
}

int sys_mem_blocks_alloc(sys_mem_blocks_t *mem_block, size_t count,
			 void **out_blocks)
{
	int ret = 0;
	int i;

	if ((mem_block == NULL) || (out_blocks == NULL)) {
		ret = -EINVAL;
		goto out;
	}

	CHECKIF((mem_block->bitmap == NULL) || (mem_block->buffer == NULL)) {
		ret = -EINVAL;
		goto out;
	}

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
		void *ptr = alloc_one(mem_block);

		if (ptr == NULL) {
			break;
		}

		out_blocks[i] = ptr;
	}

	/* If error, free already allocated blocks. */
	if (i < count) {
		(void)sys_mem_blocks_free(mem_block, i, out_blocks);
		ret = -ENOMEM;
	}

out:
	return ret;
}

int sys_mem_blocks_free(sys_mem_blocks_t *mem_block, size_t count,
			void **in_blocks)
{
	int ret = 0;
	int i;

	if ((mem_block == NULL) || (in_blocks == NULL)) {
		ret = -EINVAL;
		goto out;
	}

	CHECKIF((mem_block->bitmap == NULL) || (mem_block->buffer == NULL)) {
		ret = -EINVAL;
		goto out;
	}

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

		int r = free_one(mem_block, ptr);

		if (r != 0) {
			ret = r;
		}
	}

out:
	return ret;
}
