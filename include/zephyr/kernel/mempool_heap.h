/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_MEMPOOL_HEAP_H_
#define ZEPHYR_INCLUDE_MEMPOOL_HEAP_H_

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

#endif /* ZEPHYR_INCLUDE_MEMPOOL_HEAP_H_ */
