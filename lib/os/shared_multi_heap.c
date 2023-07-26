/*
 * Copyright (c) 2021 Carlo Caione, <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/multi_heap.h>

#include <zephyr/multi_heap/shared_multi_heap.h>

static struct sys_multi_heap shared_multi_heap;
static struct sys_heap heap_pool[MAX_SHARED_MULTI_HEAP_ATTR][MAX_MULTI_HEAPS];

/*
 * Number of heaps filled for a given attribute
 */
static unsigned int heap_cnt[MAX_SHARED_MULTI_HEAP_ATTR];

static void *smh_choice(struct sys_multi_heap *mheap, void *cfg, size_t align, size_t size)
{
	struct sys_heap *h;
	enum shared_multi_heap_attr attr;
	void *block;

	attr = (enum shared_multi_heap_attr)(long) cfg;

	if (attr >= MAX_SHARED_MULTI_HEAP_ATTR || size == 0) {
		return NULL;
	}

	/* Set in case the user requested a non-existing attr */
	block = NULL;

	for (size_t hdx = 0; hdx < heap_cnt[attr]; hdx++) {
		h = &heap_pool[attr][hdx];

		if (h->heap == NULL) {
			return NULL;
		}

		block = sys_heap_aligned_alloc(h, align, size);
		if (block != NULL) {
			break;
		}
	}

	return block;
}

int shared_multi_heap_add(struct shared_multi_heap_region *region, void *user_data)
{
	enum shared_multi_heap_attr attr;
	struct sys_heap *h;
	unsigned int slot;

	attr = region->attr;

	if (attr >= MAX_SHARED_MULTI_HEAP_ATTR) {
		return -EINVAL;
	}

	/* No more heaps available */
	if (heap_cnt[attr] >= MAX_MULTI_HEAPS) {
		return -ENOMEM;
	}

	slot = heap_cnt[attr];
	h = &heap_pool[region->attr][slot];

	sys_heap_init(h, (void *) region->addr, region->size);
	sys_multi_heap_add_heap(&shared_multi_heap, h, user_data);

	heap_cnt[attr]++;

	return 0;
}

void shared_multi_heap_free(void *block)
{
	sys_multi_heap_free(&shared_multi_heap, block);
}

void *shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes)
{
	if (attr >= MAX_SHARED_MULTI_HEAP_ATTR) {
		return NULL;
	}

	return sys_multi_heap_alloc(&shared_multi_heap, (void *)(long) attr, bytes);
}

void *shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr,
				      size_t align, size_t bytes)
{
	if (attr >= MAX_SHARED_MULTI_HEAP_ATTR) {
		return NULL;
	}

	return sys_multi_heap_aligned_alloc(&shared_multi_heap, (void *)(long) attr,
					    align, bytes);
}

int shared_multi_heap_pool_init(void)
{
	static atomic_t state;

	if (!atomic_cas(&state, 0, 1)) {
		return -EALREADY;
	}

	sys_multi_heap_init(&shared_multi_heap, smh_choice);

	atomic_set(&state, 1);

	return 0;
}
