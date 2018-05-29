/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**************************************************************************
 * FILE NAME
 *
 *       sh_mem.c
 *
 * COMPONENT
 *
 *         OpenAMP stack.
 *
 * DESCRIPTION
 *
 *       Source file for fixed buffer size memory management service. Currently
 *       it is only being used to manage shared memory.
 *
 **************************************************************************/
#include <string.h>
#include <openamp/sh_mem.h>
#include <metal/alloc.h>

/**
 * sh_mem_create_pool
 *
 * Creates new memory pool with the given parameters.
 *
 * @param start_addr - start address of the memory region
 * @param size       - size of the memory
 * @param buff_size  - fixed buffer size
 *
 * @return - pointer to memory pool
 *
 */
struct sh_mem_pool *sh_mem_create_pool(void *start_addr, unsigned int size,
				       unsigned int buff_size)
{
	struct sh_mem_pool *mem_pool;
	int pool_size;
	int num_buffs, bmp_size;

	if (!start_addr || !size || !buff_size)
		return NULL;

	/* Word align the buffer size */
	buff_size = WORD_ALIGN(buff_size);

	/* Get number of buffers. */
	num_buffs = (size / buff_size) + ((size % buff_size) == 0 ? 0 : 1);

	/*
	 * Size of the bitmap required to maintain buffers info. One word(32 bit) can
	 * keep track of 32 buffers.
	 */
	bmp_size = (num_buffs / BITMAP_WORD_SIZE)
	    + ((num_buffs % BITMAP_WORD_SIZE) == 0 ? 0 : 1);

	/* Total size required for pool control block. */
	pool_size = sizeof(struct sh_mem_pool) + BITMAP_WORD_SIZE * bmp_size;

	/* Create pool control block. */
	mem_pool = metal_allocate_memory(pool_size);

	if (mem_pool) {
		/* Initialize pool parameters */
		memset(mem_pool, 0x00, pool_size);
		metal_mutex_init(&mem_pool->lock);
		mem_pool->start_addr = start_addr;
		mem_pool->buff_size = buff_size;
		mem_pool->bmp_size = bmp_size;
		mem_pool->total_buffs = num_buffs;
	}

	return mem_pool;
}

/**
 * sh_mem_get_buffer
 *
 * Allocates fixed size buffer from the given memory pool.
 *
 * @param pool - pointer to memory pool
 *
 * @return - pointer to allocated buffer
 *
 */
void *sh_mem_get_buffer(struct sh_mem_pool *pool)
{
	void *buff = NULL;
	int bit_idx;
	unsigned int idx;

	if (!pool)
		return NULL;

	metal_mutex_acquire(&pool->lock);

	if (pool->used_buffs >= pool->total_buffs) {
		metal_mutex_release(&pool->lock);
		return NULL;
	}

	for (idx = 0; idx < pool->bmp_size; idx++) {
		/*
		 * Find the first 0 bit in the buffers bitmap. The 0th bit
		 * represents a free buffer.
		 */
		bit_idx = get_first_zero_bit(
			*(unsigned long*)SH_MEM_POOL_LOCATE_BITMAP(pool,idx));
		if (bit_idx >= 0) {
			/* Set bit to mark it as consumed. */
			*(unsigned long*)(SH_MEM_POOL_LOCATE_BITMAP(pool,idx))
				|= ((unsigned long)1 << (unsigned long)bit_idx);
			buff = (char *)pool->start_addr +
			    pool->buff_size * (idx * BITMAP_WORD_SIZE +
					       bit_idx);
			pool->used_buffs++;
			break;
		}
	}

	metal_mutex_release(&pool->lock);

	return buff;
}

/**
 * sh_mem_free_buffer
 *
 * Frees the given buffer.
 *
 * @param pool - pointer to memory pool
 * @param buff - pointer to buffer
 *
 * @return  - none
 */
void sh_mem_free_buffer(void *buff, struct sh_mem_pool *pool)
{
	unsigned long *bitmask;
	int bmp_idx, bit_idx, buff_idx;

	if (!pool || !buff)
		return;

	/* Acquire the pool lock */
	metal_mutex_acquire(&pool->lock);

	/* Map the buffer address to its index. */
	buff_idx = ((char *)buff - (char *)pool->start_addr) / pool->buff_size;

	/* Translate the buffer index to bitmap index. */
	bmp_idx = buff_idx / BITMAP_WORD_SIZE;
	bit_idx = buff_idx % BITMAP_WORD_SIZE;
	bitmask = (unsigned long*)(SH_MEM_POOL_LOCATE_BITMAP(pool, bmp_idx));

	/* Mark the buffer as free */
	*bitmask ^= (1 << bit_idx);

	pool->used_buffs--;

	/* Release the pool lock. */
	metal_mutex_release(&pool->lock);

}

/**
 * sh_mem_delete_pool
 *
 * Deletes the given memory pool.
 *
 * @param pool - pointer to memory pool
 *
 * @return  - none
 */
void sh_mem_delete_pool(struct sh_mem_pool *pool)
{

	if (pool) {
		metal_mutex_deinit(&pool->lock);
		metal_free_memory(pool);
	}
}

/**
 * get_first_zero_bit
 *
 * Provides position of first 0 bit in a 32 bit value
 *
 * @param value - given value
 *
 * @return - 0th bit position
 */
int get_first_zero_bit(unsigned long value)
{
	unsigned int idx = 0;
	value = ((~value) & (value + 1));
	while (value) {
		idx++;
		value >>= 1;
	}
	return ((int)idx-1);
}
