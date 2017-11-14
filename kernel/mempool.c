/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <init.h>
#include <string.h>
#include <misc/__assert.h>

/* Linker-defined symbols bound the static pool structs */
extern struct k_mem_pool _k_mem_pool_list_start[];
extern struct k_mem_pool _k_mem_pool_list_end[];

s64_t _tick_get(void);

static struct k_mem_pool *get_pool(int id)
{
	return &_k_mem_pool_list_start[id];
}

static int pool_id(struct k_mem_pool *pool)
{
	return pool - &_k_mem_pool_list_start[0];
}

static void *block_ptr(struct k_mem_pool *p, size_t lsz, int block)
{
	return p->buf + lsz * block;
}

static int block_num(struct k_mem_pool *p, void *block, int sz)
{
	return (block - p->buf) / sz;
}

static bool level_empty(struct k_mem_pool *p, int l)
{
	return sys_dlist_is_empty(&p->levels[l].free_list);
}

/* Places a 32 bit output pointer in word, and an integer bit index
 * within that word as the return value
 */
static int get_bit_ptr(struct k_mem_pool *p, int level, int bn, u32_t **word)
{
	u32_t *bitarray = level <= p->max_inline_level ?
		&p->levels[level].bits : p->levels[level].bits_p;

	*word = &bitarray[bn / 32];

	return bn & 0x1f;
}

static void set_free_bit(struct k_mem_pool *p, int level, int bn)
{
	u32_t *word;
	int bit = get_bit_ptr(p, level, bn, &word);

	*word |= (1<<bit);
}

static void clear_free_bit(struct k_mem_pool *p, int level, int bn)
{
	u32_t *word;
	int bit = get_bit_ptr(p, level, bn, &word);

	*word &= ~(1<<bit);
}

/* Returns all four of the free bits for the specified blocks
 * "partners" in the bottom 4 bits of the return value
 */
static int partner_bits(struct k_mem_pool *p, int level, int bn)
{
	u32_t *word;
	int bit = get_bit_ptr(p, level, bn, &word);

	return (*word >> (4*(bit / 4))) & 0xf;
}

static size_t buf_size(struct k_mem_pool *p)
{
	return p->n_max * p->max_sz;
}

static bool block_fits(struct k_mem_pool *p, void *block, size_t bsz)
{
	return (block + bsz - 1 - p->buf) < buf_size(p);
}

static void init_mem_pool(struct k_mem_pool *p)
{
	int i;
	size_t buflen = p->n_max * p->max_sz, sz = p->max_sz;
	u32_t *bits = p->buf + buflen;

	sys_dlist_init(&p->wait_q);

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

int init_static_pools(struct device *unused)
{
	ARG_UNUSED(unused);
	struct k_mem_pool *p;

	for (p = _k_mem_pool_list_start; p < _k_mem_pool_list_end; p++) {
		init_mem_pool(p);
	}

	return 0;
}

SYS_INIT(init_static_pools, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

/* A note on synchronization: all manipulation of the actual pool data
 * happens in one of alloc_block()/free_block() or break_block().  All
 * of these transition between a state where the caller "holds" a
 * block pointer that is marked used in the store and one where she
 * doesn't (or else they will fail, e.g. if there isn't a free block).
 * So that is the basic operation that needs synchronization, which we
 * can do piecewise as needed in small one-block chunks to preserve
 * latency.  At most (in free_block) a single locked operation
 * consists of four bit sets and dlist removals. If the overall
 * allocation operation fails, we just free the block we have (putting
 * a block back into the list cannot fail) and return failure.
 */

static void *alloc_block(struct k_mem_pool *p, int l, size_t lsz)
{
	sys_dnode_t *block;
	int key = irq_lock();

	block = sys_dlist_get(&p->levels[l].free_list);
	if (block) {
		clear_free_bit(p, l, block_num(p, block, lsz));
	}
	irq_unlock(key);

	return block;
}

static void free_block(struct k_mem_pool *p, int level, size_t *lsizes, int bn)
{
	int i, key, lsz = lsizes[level];
	void *block = block_ptr(p, lsz, bn);

	key = irq_lock();

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

		irq_unlock(key);
		free_block(p, level-1, lsizes, bn / 4); /* tail recursion! */
		return;
	}

	if (block_fits(p, block, lsz)) {
		sys_dlist_append(&p->levels[level].free_list, block);
	}

	irq_unlock(key);
}

/* Takes a block of a given level, splits it into four blocks of the
 * next smaller level, puts three into the free list as in
 * free_block() but without the need to check adjacent bits or
 * recombine, and returns the remaining smaller block.
 */
static void *break_block(struct k_mem_pool *p, void *block,
			 int l, size_t *lsizes)
{
	int i, bn, key;

	key = irq_lock();

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

	irq_unlock(key);

	return block;
}

static int pool_alloc(struct k_mem_pool *p, struct k_mem_block *block,
		      size_t size)
{
	size_t lsizes[p->n_levels];
	int i, alloc_l = -1, free_l = -1, from_l;
	void *blk = NULL;

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
		block->data = NULL;
		return -ENOMEM;
	}

	/* Iteratively break the smallest enclosing block... */
	blk = alloc_block(p, free_l, lsizes[free_l]);

	if (!blk) {
		/* This can happen if we race with another allocator.
		 * It's OK, just back out and the timeout code will
		 * retry.  Note mild overloading: -EAGAIN isn't for
		 * propagation to the caller, it's to tell the loop in
		 * k_mem_pool_alloc() to try again synchronously.  But
		 * it means exactly what it says.
		 */
		return -EAGAIN;
	}

	for (from_l = free_l; from_l < alloc_l; from_l++) {
		blk = break_block(p, blk, from_l, lsizes);
	}

	/* ... until we have something to return */
	block->data = blk;
	block->id.pool = pool_id(p);
	block->id.level = alloc_l;
	block->id.block = block_num(p, block->data, lsizes[alloc_l]);
	return 0;
}

int k_mem_pool_alloc(struct k_mem_pool *p, struct k_mem_block *block,
		     size_t size, s32_t timeout)
{
	int ret, key;
	s64_t end = 0;

	__ASSERT(!(_is_in_isr() && timeout != K_NO_WAIT), "");

	if (timeout > 0) {
		end = _tick_get() + _ms_to_ticks(timeout);
	}

	while (1) {
		ret = pool_alloc(p, block, size);

		if (ret == 0 || timeout == K_NO_WAIT ||
		    ret == -EAGAIN || (ret && ret != -ENOMEM)) {
			return ret;
		}

		key = irq_lock();
		_pend_current_thread(&p->wait_q, timeout);
		_Swap(key);

		if (timeout != K_FOREVER) {
			timeout = end - _tick_get();

			if (timeout < 0) {
				break;
			}
		}
	}

	return -EAGAIN;
}

void k_mem_pool_free(struct k_mem_block *block)
{
	int i, key, need_sched = 0;
	struct k_mem_pool *p = get_pool(block->id.pool);
	size_t lsizes[p->n_levels];

	/* As in k_mem_pool_alloc(), we build a table of level sizes
	 * to avoid having to store it in precious RAM bytes.
	 * Overhead here is somewhat higher because free_block()
	 * doesn't inherently need to traverse all the larger
	 * sublevels.
	 */
	lsizes[0] = _ALIGN4(p->max_sz);
	for (i = 1; i <= block->id.level; i++) {
		lsizes[i] = _ALIGN4(lsizes[i-1] / 4);
	}

	free_block(get_pool(block->id.pool), block->id.level,
		   lsizes, block->id.block);

	/* Wake up anyone blocked on this pool and let them repeat
	 * their allocation attempts
	 */
	key = irq_lock();

	while (!sys_dlist_is_empty(&p->wait_q)) {
		struct k_thread *th = (void *)sys_dlist_peek_head(&p->wait_q);

		_unpend_thread(th);
		_abort_thread_timeout(th);
		_ready_thread(th);
		need_sched = 1;
	}

	if (need_sched && !_is_in_isr()) {
		_reschedule_threads(key);
	} else {
		irq_unlock(key);
	}
}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

/*
 * Heap is defined using HEAP_MEM_POOL_SIZE configuration option.
 *
 * This module defines the heap memory pool and the _HEAP_MEM_POOL symbol
 * that has the address of the associated memory pool struct.
 */

K_MEM_POOL_DEFINE(_heap_mem_pool, 64, CONFIG_HEAP_MEM_POOL_SIZE, 1, 4);
#define _HEAP_MEM_POOL (&_heap_mem_pool)

void *k_malloc(size_t size)
{
	struct k_mem_block block;

	/*
	 * get a block large enough to hold an initial (hidden) block
	 * descriptor, as well as the space the caller requested
	 */
	size += sizeof(struct k_mem_block);
	if (k_mem_pool_alloc(_HEAP_MEM_POOL, &block, size, K_NO_WAIT) != 0) {
		return NULL;
	}

	/* save the block descriptor info at the start of the actual block */
	memcpy(block.data, &block, sizeof(struct k_mem_block));

	/* return address of the user area part of the block to the caller */
	return (char *)block.data + sizeof(struct k_mem_block);
}


void k_free(void *ptr)
{
	if (ptr != NULL) {
		/* point to hidden block descriptor at start of block */
		ptr = (char *)ptr - sizeof(struct k_mem_block);

		/* return block to the heap memory pool */
		k_mem_pool_free(ptr);
	}
}

void *k_calloc(size_t nmemb, size_t size)
{
	void *ret;
	size_t bounds;

#ifdef CONFIG_ASSERT
	__ASSERT(!__builtin_mul_overflow(nmemb, size, &bounds),
		 "requested size overflow");
#else
	bounds = nmemb * size;
#endif
	ret = k_malloc(bounds);
	if (ret) {
		memset(ret, 0, bounds);
	}
	return ret;
}
#endif
