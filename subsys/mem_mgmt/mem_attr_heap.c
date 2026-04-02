/*
 * Copyright (c) 2021 Carlo Caione, <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/sys/multi_heap.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-sw.h>

struct ma_heap {
	struct sys_heap heap;
	uint32_t attr;
};

struct {
	struct ma_heap ma_heaps[MAX_MULTI_HEAPS];
	struct sys_multi_heap multi_heap;
	int nheaps;
} mah_data;

static void *mah_choice(struct sys_multi_heap *m_heap, void *cfg, size_t align, size_t size)
{
	uint32_t attr;
	void *block;

	if (size == 0) {
		return NULL;
	}

	attr = (uint32_t)(long) cfg;

	/* Set in case the user requested a non-existing attr */
	block = NULL;

	for (size_t hdx = 0; hdx < mah_data.nheaps; hdx++) {
		struct ma_heap *h;

		h = &mah_data.ma_heaps[hdx];

		if (h->attr != attr) {
			continue;
		}

		block = sys_heap_aligned_alloc(&h->heap, align, size);
		if (block != NULL) {
			break;
		}
	}

	return block;
}

void mem_attr_heap_free(void *block)
{
	sys_multi_heap_free(&mah_data.multi_heap, block);
}

void *mem_attr_heap_alloc(uint32_t attr, size_t bytes)
{
	return sys_multi_heap_alloc(&mah_data.multi_heap,
				    (void *)(long) attr, bytes);
}

void *mem_attr_heap_aligned_alloc(uint32_t attr, size_t align, size_t bytes)
{
	return sys_multi_heap_aligned_alloc(&mah_data.multi_heap,
					    (void *)(long) attr, align, bytes);
}

const struct mem_attr_region_t *mem_attr_heap_get_region(void *addr)
{
	const struct sys_multi_heap_rec *heap_rec;

	heap_rec = sys_multi_heap_get_heap(&mah_data.multi_heap, addr);

	return (const struct mem_attr_region_t *) heap_rec->user_data;
}

static int ma_heap_add(const struct mem_attr_region_t *region, uint32_t attr)
{
	struct ma_heap *mh;
	struct sys_heap *h;

	/* No more heaps available */
	if (mah_data.nheaps >= MAX_MULTI_HEAPS) {
		return -ENOMEM;
	}

	mh = &mah_data.ma_heaps[mah_data.nheaps++];
	h = &mh->heap;

	mh->attr = attr;

	sys_heap_init(h, (void *) region->dt_addr, region->dt_size);
	sys_multi_heap_add_heap(&mah_data.multi_heap, h, (void *) region);

	return 0;
}


int mem_attr_heap_pool_init(void)
{
	const struct mem_attr_region_t *regions;
	static atomic_t state;
	size_t num_regions;

	if (!atomic_cas(&state, 0, 1)) {
		return -EALREADY;
	}

	sys_multi_heap_init(&mah_data.multi_heap, mah_choice);

	num_regions = mem_attr_get_regions(&regions);

	for (size_t idx = 0; idx < num_regions; idx++) {
		uint32_t sw_attr;

		sw_attr = DT_MEM_SW_ATTR_GET(regions[idx].dt_attr);

		/* No SW attribute is present */
		if (!sw_attr) {
			continue;
		}

		if (ma_heap_add(&regions[idx], sw_attr)) {
			return -ENOMEM;
		}
	}

	return 0;
}
