/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/predef_heap.h>
#include <zephyr/arch/common/sys_bitops.h>

static int predef_heap_check_config(struct predef_heap *heap,
				    const struct predef_heap_config *config,
				    const int config_entries)
{
	size_t last_mem_size = 0;
	size_t config_size = 0;
	size_t mem_size = 0;
	size_t last_size = 0;
	int i;

	if (config_entries <= 0) {
		return -EINVAL;
	}

	for (i = 0; i < config_entries; i++, config++) {
		/* Validate config entry */
		if (config->count <= 0 || config->size <= last_size) {
			return -EINVAL;
		}

		config_size += ceiling_fraction(config->count, BITS_PER_LONG) *
			sizeof(unsigned long);
		mem_size += config->size * config->count;
		last_size = config->size;

		/* Size variable overflow protection */
		if (mem_size <= last_mem_size) {
			return -E2BIG;
		}

		last_mem_size = mem_size;
	}

	config_size += sizeof(struct predef_heap_bundle) * config_entries;

	if (config_size > heap->config_size) {
		return -E2BIG;
	}

	if (mem_size > heap->size) {
		return -E2BIG;
	}

	return 0;
}

static void predef_heap_configure(struct predef_heap *heap,
				  const struct predef_heap_config *config,
				  const int config_entries)
{
	struct predef_heap_bundle *bundle = (struct predef_heap_bundle *)heap->config;
	unsigned long *bitfield = (unsigned long *)(bundle + config_entries);
	uintptr_t mem = POINTER_TO_UINT(heap->start);
	int count, i;

	heap->bundles_count = config_entries;
	for (i = 0; i < config_entries; i++, config++, bundle++) {
		bundle->buffer_size = config->size;
		bundle->buffers_count = config->count;
		bundle->free_count = config->count;
		bundle->bitfield = bitfield;
		bundle->first_buffer = mem;
		mem += config->size * config->count;

		/* Initialise bitmap - mark all buffers as free */
		for (count = config->count; count > 0; count -= BITS_PER_LONG) {
			if (count >= BITS_PER_LONG) {
				*bitfield++ = -1;
			} else {
				*bitfield++ = BIT_MASK(count);
			}
		}
	}
}

static int predef_heap_check_unused(struct predef_heap *heap)
{
	struct predef_heap_bundle *bundle = (struct predef_heap_bundle *)heap->config;
	int i;

	for (i = 0; i < heap->bundles_count; i++, bundle++) {
		if (bundle->free_count != bundle->buffers_count) {
			return -EBUSY;
		}
	}

	return 0;
}

int predef_heap_init(struct predef_heap *heap, const struct predef_heap_config *config,
		     const int config_entries, void *const memory, const uint32_t mem_size)
{
	int ret;

	if (!heap->config_size || !heap->config) {
		return -EBADF;
	}

	if (!mem_size || !config || !config_entries) {
		return -EINVAL;
	}

	heap->start = memory;
	heap->size = mem_size;

	ret = predef_heap_check_config(heap, config, config_entries);
	if (ret) {
		return ret;
	}

	predef_heap_configure(heap, config, config_entries);
	return 0;
}

int predef_heap_reconfigure(struct predef_heap *heap, const struct predef_heap_config *config,
			    const int config_entries)
{
	int ret;

	ret = predef_heap_check_unused(heap);
	if (ret) {
		return ret;
	}

	ret = predef_heap_check_config(heap, config, config_entries);
	if (ret) {
		return ret;
	}

	predef_heap_configure(heap, config, config_entries);
	return 0;
}

static inline int predef_heap_find_free(unsigned long *bitfield, const int bit_count)
{
	int i;

	for (i = 0; i < bit_count; i += BITS_PER_LONG, bitfield++) {
		if (*bitfield) {
			return i + __builtin_ffsl(*bitfield) - 1;
		}
	}

	return -1;
}

#define BUFFER_IS_ALIGNED(address, alignment) (((address) & ((alignment) - 1)) == 0)

int predef_heap_check_space(struct predef_heap *heap, size_t align, const size_t bytes)
{
	struct predef_heap_bundle *bundle = (struct predef_heap_bundle *)heap->config;
	int i, ret = -ENOSPC;

	if (!align) {
		align = 1;
	}

	__ASSERT(is_power_of_two(align), "align must be a power of 2");

	for (i = 0; i < heap->bundles_count; i++, bundle++) {
		if (bundle->buffer_size >= bytes && bundle->free_count &&
		    BUFFER_IS_ALIGNED(bundle->first_buffer, align) &&
		    BUFFER_IS_ALIGNED(bundle->buffer_size, align)) {
			ret = 0;
			break;
		}
	}

	return ret;
}

void *predef_heap_aligned_alloc(struct predef_heap *heap, size_t align, size_t bytes)
{
	struct predef_heap_bundle *bundle = (struct predef_heap_bundle *)heap->config;
	void *ptr = NULL;
	int i, slot;

	if (!align) {
		align = 1;
	}

	__ASSERT(is_power_of_two(align), "align must be a power of 2");

	for (i = 0; i < heap->bundles_count; i++, bundle++) {
		if (bundle->buffer_size >= bytes && bundle->free_count &&
		    BUFFER_IS_ALIGNED(bundle->first_buffer, align) &&
		    BUFFER_IS_ALIGNED(bundle->buffer_size, align)) {
			slot = predef_heap_find_free(bundle->bitfield, bundle->buffers_count);
			__ASSERT_NO_MSG(slot >= 0);

			bundle->free_count--;
			sys_bitfield_clear_bit((mem_addr_t)bundle->bitfield, slot);
			ptr = UINT_TO_POINTER(bundle->first_buffer + slot * bundle->buffer_size);
			break;
		}
	}

	return ptr;
}

int predef_heap_free(struct predef_heap *heap, void *mem)
{
	struct predef_heap_bundle *bundle = (struct predef_heap_bundle *)heap->config;
	uintptr_t addr, start, stop;
	int ret = -ENOENT;
	int i;

	addr = POINTER_TO_UINT(mem);

	for (i = 0; i < heap->bundles_count; i++, bundle++) {
		start = bundle->first_buffer;
		stop = start + bundle->buffer_size * bundle->buffers_count;

		if (addr >= start && addr < stop) {
			addr -= start;

			/* Check pointer correctness */
			if (addr % bundle->buffer_size) {
				break;
			}

			addr /= bundle->buffer_size;

			if (!sys_bitfield_test_and_set_bit((mem_addr_t)bundle->bitfield, addr)) {
				bundle->free_count++;
				ret = 0;
			}
			break;
		}
	}

	return ret;
}

size_t predef_heap_usable_size(struct predef_heap *heap, void *mem)
{
	struct predef_heap_bundle *bundle = (struct predef_heap_bundle *)heap->config;
	uintptr_t addr, start, stop;
	size_t size = 0;
	int i;

	addr = POINTER_TO_UINT(mem);

	for (i = 0; i < heap->bundles_count; i++, bundle++) {
		start = bundle->first_buffer;
		stop = start + bundle->buffer_size * bundle->buffers_count;

		if (addr >= start && addr < stop) {
			size = bundle->buffer_size;
			break;
		}
	}

	return size;
}
