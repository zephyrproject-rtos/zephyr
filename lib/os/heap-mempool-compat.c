/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/sys_heap.h>
#include <sys/mempool.h>
#include "heap.h"

void *sys_compat_mem_pool_alloc(struct sys_mem_pool *p, size_t size)
{
	char *ret;

	sys_mutex_lock(&p->mutex, K_FOREVER);

	size += sizeof(struct sys_mem_pool **);
	ret = sys_heap_alloc(&p->heap, size);
	if (ret) {
		*(struct sys_mem_pool **)ret = p;
		ret += sizeof(struct sys_mem_pool **);
	}
	sys_mutex_unlock(&p->mutex);
	return ret;
}

void sys_compat_mem_pool_free(void *ptr)
{
	struct sys_mem_pool *p;

	if (ptr == NULL) {
		return;
	}

	ptr = (char *)ptr - sizeof(struct sys_mem_pool **);
	p = *(struct sys_mem_pool **)ptr;

	sys_mutex_lock(&p->mutex, K_FOREVER);
	sys_heap_free(&p->heap, ptr);
	sys_mutex_unlock(&p->mutex);
}

size_t sys_compat_mem_pool_try_expand_inplace(void *ptr, size_t requested_size)
{
	struct sys_mem_pool *p;
	struct z_heap *h;
	chunkid_t c;
	size_t required_chunks;

	requested_size += sizeof(struct sys_mem_pool **);
	ptr = (char *)ptr - sizeof(struct sys_mem_pool **);
	p = *(struct sys_mem_pool **)ptr;
	h = p->heap.heap;
	required_chunks = bytes_to_chunksz(h, requested_size);
	c = mem_to_chunk(h, ptr);

	/*
	 * We could eat into the right_chunk if it is not used, or relinquish
	 * part of our chunk if shrinking the size, but that requires
	 * modifying the heap pointers. Let's keep it simple for now.
	 */
	if (chunk_size(h, c) >= required_chunks) {
		return 0;
	}

	return chunk_size(h, c) * CHUNK_UNIT - chunk_header_bytes(h);
}

#define heap_blockid_to_chunkid(level, block) ((level) | ((block) << 4))
#define heap_chunkid_to_blockid_level(c) ((c) & ((1 << 4) - 1))
#define heap_chunkid_to_blockid_block(c) ((c) >> 4)

int z_sys_compat_mem_pool_block_alloc(struct sys_heap *p, size_t size,
				u32_t *level_p, u32_t *block_p, void **data_p)
{
	unsigned int key = irq_lock();
	char *ret = sys_heap_alloc(p, size);
	irq_unlock(key);
	*data_p = ret;
	if (ret) {
		struct z_heap *h = p->heap;
		chunkid_t c = mem_to_chunk(h, ret);
		*level_p = heap_chunkid_to_blockid_level(c);
		*block_p = heap_chunkid_to_blockid_block(c);
		return 0;
	}
	return -ENOMEM;
}

void z_sys_compat_mem_pool_block_free(struct sys_heap *p,
				      u32_t level, u32_t block)
{
	struct z_heap *h = p->heap;
	chunkid_t c = heap_blockid_to_chunkid(level, block);
	void *ptr = chunk_to_mem(h, c);
	unsigned int key = irq_lock();
	sys_heap_free(p, ptr);
	irq_unlock(key);
}
