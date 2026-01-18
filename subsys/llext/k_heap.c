/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/llext/k_heap.h>

#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
#ifdef CONFIG_HARVARD
struct k_heap llext_instr_heap;
struct k_heap llext_data_heap;
#else
struct k_heap llext_ext_heap;
#endif
bool llext_heap_inited;
#else
#ifdef CONFIG_HARVARD
Z_HEAP_DEFINE_IN_SECT(llext_instr_heap, (CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1)),
		      __attribute__((section(CONFIG_LLEXT_INSTR_HEAP_SECTION))));
Z_HEAP_DEFINE_IN_SECT(llext_data_heap, (CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1)),
		      __attribute__((section(CONFIG_LLEXT_DATA_HEAP_SECTION))));
#define llext_metadata_heap llext_data_heap
#else
K_HEAP_DEFINE(llext_ext_heap, CONFIG_LLEXT_HEAP_SIZE * KB(1));
#endif
#endif

#ifdef CONFIG_HARVARD
#define llext_metadata_heap llext_data_heap
#else
#define llext_instr_heap    llext_ext_heap
#define llext_data_heap     llext_ext_heap
#define llext_metadata_heap llext_ext_heap
#endif

int llext_k_heap_init_harvard(struct llext_heap *h, void *instr_mem, size_t instr_bytes,
			      void *data_mem, size_t data_bytes)
{
#if !defined(CONFIG_LLEXT_HEAP_DYNAMIC) || !defined(CONFIG_HARVARD)
	return -ENOSYS;
#else
	if (llext_heap_inited) {
		return -EEXIST;
	}

	k_heap_init(&llext_instr_heap, instr_mem, instr_bytes);
	k_heap_init(&llext_data_heap, data_mem, data_bytes);

	llext_heap_inited = true;
	return 0;
#endif
}

int llext_k_heap_init(struct llext_heap *h, void *mem, size_t bytes)
{
#if !defined(CONFIG_LLEXT_HEAP_DYNAMIC) || defined(CONFIG_HARVARD)
	return -ENOSYS;
#else
	if (llext_heap_inited) {
		return -EEXIST;
	}

	k_heap_init(&llext_ext_heap, mem, bytes);

	llext_heap_inited = true;
	return 0;
#endif
}

#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
static int llext_loaded(struct llext *ext, void *arg)
{
	return 1;
}
#endif

int llext_k_heap_uninit(struct llext_heap *h)
{
#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
	if (!llext_heap_inited) {
		return -EEXIST;
	}
	if (llext_iterate(llext_loaded, NULL)) {
		return -EBUSY;
	}
	llext_heap_inited = false;
	return 0;
#else
	return -ENOSYS;
#endif
}

bool llext_k_heap_is_inited(struct llext_heap *h)
{
#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
	extern bool llext_heap_inited;

	return llext_heap_inited;
#else
	return true;
#endif
}

void *llext_k_heap_alloc_metadata(struct llext_heap *h, size_t bytes)
{
	if (!llext_k_heap_is_inited(h)) {
		return NULL;
	}

	return k_heap_alloc(&llext_metadata_heap, bytes, K_NO_WAIT);
}

void *llext_k_heap_aligned_alloc_data(struct llext_heap *h, struct llext *ext, size_t align,
				      size_t bytes)
{
	if (!llext_k_heap_is_inited(h)) {
		return NULL;
	}

	return k_heap_aligned_alloc(&llext_data_heap, align, bytes, K_NO_WAIT);
}

void *llext_k_heap_aligned_alloc_instr(struct llext_heap *h, struct llext *ext, size_t align,
				       size_t bytes)
{
	if (!llext_k_heap_is_inited(h)) {
		return NULL;
	}

	return k_heap_aligned_alloc(&llext_instr_heap, align, bytes, K_NO_WAIT);
}

void llext_k_heap_free_metadata(struct llext_heap *h, void *ptr)
{
	if (!llext_k_heap_is_inited(h)) {
		return;
	}

	k_heap_free(&llext_metadata_heap, ptr);
}

void llext_k_heap_free_data(struct llext_heap *h, struct llext *ext, void *ptr)
{
	if (!llext_k_heap_is_inited(h)) {
		return;
	}

	k_heap_free(&llext_data_heap, ptr);
}

void llext_k_heap_free_instr(struct llext_heap *h, struct llext *ext, void *ptr)
{
	if (!llext_k_heap_is_inited(h)) {
		return;
	}

	k_heap_free(&llext_instr_heap, ptr);
}

struct llext_k_heap k_heap_heap = Z_LLEXT_K_HEAP();
struct llext_heap *llext_heap_inst = &k_heap_heap.heap;
