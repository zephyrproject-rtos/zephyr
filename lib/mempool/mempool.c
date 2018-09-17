/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <misc/__assert.h>
#include <misc/mempool_base.h>
#include <misc/mempool.h>

static bool level_empty(struct sys_mem_pool_base *p, int l)
{
	return sys_dlist_is_empty(&p->levels[l].free_list);
}

static void *block_ptr(struct sys_mem_pool_base *p, size_t lsz, int block)
{
	return p->buf + lsz * block;
}

static int block_num(struct sys_mem_pool_base *p, void *block, int sz)
{
	return (block - p->buf) / sz;
}

/* Places a 32 bit output pointer in word, and an integer bit index
 * within that word as the return value
 */
static int get_bit_ptr(struct sys_mem_pool_base *p, int level, int bn,
		       u32_t **word)
{
	u32_t *bitarray = level <= p->max_inline_level ?
		&p->levels[level].bits : p->levels[level].bits_p;

	*word = &bitarray[bn / 32];

	return bn & 0x1f;
}

static void set_free_bit(struct sys_mem_pool_base *p, int level, int bn)
{
	u32_t *word;
	int bit = get_bit_ptr(p, level, bn, &word);

	*word |= (1<<bit);
}

static void clear_free_bit(struct sys_mem_pool_base *p, int level, int bn)
{
	u32_t *word;
	int bit = get_bit_ptr(p, level, bn, &word);

	*word &= ~(1<<bit);
}

/* Returns all four of the free bits for the specified blocks
 * "partners" in the bottom 4 bits of the return value
 */
static int partner_bits(struct sys_mem_pool_base *p, int level, int bn)
{
	u32_t *word;
	int bit = get_bit_ptr(p, level, bn, &word);

	return (*word >> (4*(bit / 4))) & 0xf;
}

static size_t buf_size(struct sys_mem_pool_base *p)
{
	return p->n_max * p->max_sz;
}

static bool block_fits(struct sys_mem_pool_base *p, void *block, size_t bsz)
{
	return (block + bsz - 1 - p->buf) < buf_size(p);
}

void _sys_mem_pool_base_init(struct sys_mem_pool_base *p)
{
	int i;
	size_t buflen = p->n_max * p->max_sz, sz = p->max_sz;
	u32_t *bits = p->buf + buflen;

	p->max_inline_level = -1;

	for (i = 0; i < p->n_levels; i++) {
		int nblocks = buflen / sz;

		sys_dlist_init(&p->levels[i].free_list);

		if (nblocks < 32) {
			p->max_inline_level = i;
		} else {
			p->levels[i].bits_p = bits;
			bits += (nblocks + 31)/32;
		}

		sz = _ALIGN4(sz / 4);
	}

	for (i = 0; i < p->n_max; i++) {
		void *block = block_ptr(p, p->max_sz, i);

		sys_dlist_append(&p->levels[0].free_list, block);
		set_free_bit(p, 0, i);
	}
}

/* A note on synchronization:
 *
 * For k_mem_pools which are interrupt safe, all manipulation of the actual
 * pool data happens in one of alloc_block()/free_block() or break_block().
 * All of these transition between a state where the caller "holds" a block
 * pointer that is marked used in the store and one where she doesn't (or else
 * they will fail, e.g. if there isn't a free block).  So that is the basic
 * operation that needs synchronization, which we can do piecewise as needed in
 * small one-block chunks to preserve latency.  At most (in free_block) a
 * single locked operation consists of four bit sets and dlist removals. If the
 * overall allocation operation fails, we just free the block we have (putting
 * a block back into the list cannot fail) and return failure.
 *
 * For user mode compatible sys_mem_pool pools, a semaphore is used at the API
 * level since using that does not introduce latency issues like locking
 * interrupts does.
 */

static inline int pool_irq_lock(struct sys_mem_pool_base *p)
{
	if (p->flags & SYS_MEM_POOL_KERNEL) {
		return irq_lock();
	} else {
		return 0;
	}
}

static inline void pool_irq_unlock(struct sys_mem_pool_base *p, int key)
{
	if (p->flags & SYS_MEM_POOL_KERNEL) {
		irq_unlock(key);
	}
}

static void *block_alloc(struct sys_mem_pool_base *p, int l, size_t lsz)
{
	sys_dnode_t *block;
	int key = pool_irq_lock(p);

	block = sys_dlist_get(&p->levels[l].free_list);
	if (block != NULL) {
		clear_free_bit(p, l, block_num(p, block, lsz));
	}
	pool_irq_unlock(p, key);

	return block;
}

static void block_free(struct sys_mem_pool_base *p, int level,
			      size_t *lsizes, int bn)
{
	int i, key, lsz = lsizes[level];
	void *block = block_ptr(p, lsz, bn);

	key = pool_irq_lock(p);

	set_free_bit(p, level, bn);

	if (level && partner_bits(p, level, bn) == 0xf) {
		for (i = 0; i < 4; i++) {
			int b = (bn & ~3) + i;

			clear_free_bit(p, level, b);
			if (b != bn &&
			    block_fits(p, block_ptr(p, lsz, b), lsz)) {
				sys_dlist_remove(block_ptr(p, lsz, b));
			}
		}

		pool_irq_unlock(p, key);

		/* tail recursion! */
		block_free(p, level-1, lsizes, bn / 4);
		return;
	}

	if (block_fits(p, block, lsz)) {
		sys_dlist_append(&p->levels[level].free_list, block);
	}

	pool_irq_unlock(p, key);
}

/* Takes a block of a given level, splits it into four blocks of the
 * next smaller level, puts three into the free list as in
 * block_free() but without the need to check adjacent bits or
 * recombine, and returns the remaining smaller block.
 */
static void *block_break(struct sys_mem_pool_base *p, void *block, int l,
				size_t *lsizes)
{
	int i, bn, key;

	key = pool_irq_lock(p);

	bn = block_num(p, block, lsizes[l]);

	for (i = 1; i < 4; i++) {
		int lbn = 4*bn + i;
		int lsz = lsizes[l + 1];
		void *block2 = (lsz * i) + (char *)block;

		set_free_bit(p, l + 1, lbn);
		if (block_fits(p, block2, lsz)) {
			sys_dlist_append(&p->levels[l + 1].free_list, block2);
		}
	}

	pool_irq_unlock(p, key);

	return block;
}

int _sys_mem_pool_block_alloc(struct sys_mem_pool_base *p, size_t size,
			      u32_t *level_p, u32_t *block_p, void **data_p)
{
	int i, from_l;
	int alloc_l = -1, free_l = -1;
	void *data;
	size_t lsizes[p->n_levels];

	/* Walk down through levels, finding the one from which we
	 * want to allocate and the smallest one with a free entry
	 * from which we can split an allocation if needed.  Along the
	 * way, we populate an array of sizes for each level so we
	 * don't need to waste RAM storing it.
	 */
	lsizes[0] = _ALIGN4(p->max_sz);
	for (i = 0; i < p->n_levels; i++) {
		if (i > 0) {
			lsizes[i] = _ALIGN4(lsizes[i-1] / 4);
		}

		if (lsizes[i] < size) {
			break;
		}

		alloc_l = i;
		if (!level_empty(p, i)) {
			free_l = i;
		}
	}

	if (alloc_l < 0 || free_l < 0) {
		*data_p = NULL;
		return -ENOMEM;
	}

	/* Iteratively break the smallest enclosing block... */
	data = block_alloc(p, free_l, lsizes[free_l]);

	if (data == NULL) {
		/* This can happen if we race with another allocator.
		 * It's OK, just back out and the timeout code will
		 * retry.  Note mild overloading: -EAGAIN isn't for
		 * propagation to the caller, it's to tell the loop in
		 * k_mem_pool_alloc() to try again synchronously.  But
		 * it means exactly what it says.
		 *
		 * This doesn't happen for user mode memory pools as this
		 * entire function runs with a semaphore held.
		 */
		return -EAGAIN;
	}

	for (from_l = free_l; from_l < alloc_l; from_l++) {
		data = block_break(p, data, from_l, lsizes);
	}

	*level_p = alloc_l;
	*block_p = block_num(p, data, lsizes[alloc_l]);
	*data_p = data;

	return 0;
}

void _sys_mem_pool_block_free(struct sys_mem_pool_base *p, u32_t level,
			      u32_t block)
{
	size_t lsizes[p->n_levels];
	int i;

	/* As in _sys_mem_pool_block_alloc(), we build a table of level sizes
	 * to avoid having to store it in precious RAM bytes.
	 * Overhead here is somewhat higher because block_free()
	 * doesn't inherently need to traverse all the larger
	 * sublevels.
	 */
	lsizes[0] = _ALIGN4(p->max_sz);
	for (i = 1; i <= level; i++) {
		lsizes[i] = _ALIGN4(lsizes[i-1] / 4);
	}

	block_free(p, level, lsizes, block);
}

/*
 * Functions specific to user-mode blocks
 */

void *sys_mem_pool_alloc(struct sys_mem_pool *p, size_t size)
{
	struct sys_mem_pool_block *blk;
	int level, block;
	char *ret;

	k_mutex_lock(p->mutex, K_FOREVER);

	size += sizeof(struct sys_mem_pool_block);
	if (_sys_mem_pool_block_alloc(&p->base, size, &level, &block,
				      (void **)&ret)) {
		ret = NULL;
		goto out;
	}

	blk = (struct sys_mem_pool_block *)ret;
	blk->level = level;
	blk->block = block;
	blk->pool = p;
	ret += sizeof(*blk);
out:
	k_mutex_unlock(p->mutex);
	return ret;
}

void sys_mem_pool_free(void *ptr)
{
	struct sys_mem_pool_block *blk;
	struct sys_mem_pool *p;

	if (ptr == NULL) {
		return;
	}

	blk = (struct sys_mem_pool_block *)((char *)ptr - sizeof(*blk));
	p = blk->pool;

	k_mutex_lock(p->mutex, K_FOREVER);
	_sys_mem_pool_block_free(&p->base, blk->level, blk->block);
	k_mutex_unlock(p->mutex);
}

