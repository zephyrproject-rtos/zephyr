/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_MEMPOOL_HEAP_H_

/* Compatibility implementation of a k_mem_pool backend in terms of a
 * k_heap
 */

/* The "ID" of a k_heap-based mempool is just the tuple of the data
 * block pointer and the heap that allocated it
 */
struct k_mem_block_id {
	void *data;
	struct k_heap *heap;
};

/* Note the data pointer gets unioned with the same value stored in
 * the ID field to save space.
 */
struct k_mem_block {
	union {
		void *data;
		struct k_mem_block_id id;
	};
};

struct k_mem_pool {
	struct k_heap *heap;
};

/* Sizing is a heuristic, as k_mem_pool made promises about layout
 * that k_heap does not.  We make space for the number of maximum
 * objects defined, and include extra so there's enough metadata space
 * available for the maximum number of minimum-sized objects to be
 * stored: 8 bytes for each desired chunk header, and a 15 word block
 * to reserve room for a "typical" set of bucket list heads and the heap
 * footer(this size was picked more to conform with existing test
 * expectations than any rigorous theory -- we have tests that rely on being
 * able to allocate the blocks promised and ones that make assumptions about
 * when memory will run out).
 */
#define Z_MEM_POOL_DEFINE(name, minsz, maxsz, nmax, align)		\
		K_HEAP_DEFINE(poolheap_##name,				\
			      ((maxsz) * (nmax))			\
			      + 8 * ((maxsz) * (nmax) / (minsz))	\
			      + 15 * sizeof(void *));			\
		struct k_mem_pool name = {				\
			.heap = &poolheap_##name			\
		}


#endif /* ZEPHYR_INCLUDE_MEMPOOL_HEAP_H_ */
