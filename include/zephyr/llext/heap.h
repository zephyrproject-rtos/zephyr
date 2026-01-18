/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_HEAP_H
#define ZEPHYR_LLEXT_HEAP_H

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief LLEXT heap context types.
 *
 * The following types are used to define the context of the heap
 * used by the \ref llext subsystem.
 *
 * @defgroup llext_heap_apis LLEXT heap context
 * @ingroup llext_apis
 * @{
 */

#include <zephyr/llext/llext.h>

/**
 * @brief Linkable loadable extension heap context
 *
 * This struct is used to access the heap while and after an
 * extension has been loaded by the LLEXT subsystem. Users should pass the
 * global llext_heap_inst struct pointer included in the header of their
 * chosen heap implementation to the functions herein.
 */
struct llext_heap {
	/**
	 * @brief Function to initialize the heap for Harvard architecture.
	 *
	 * @param[in] h Heap
	 * @param[in] instr_mem Pointer to instruction memory
	 * @param[in] instr_bytes Size of instruction memory in bytes
	 * @param[in] data_mem Pointer to data memory
	 * @param[in] data_bytes Size of data memory in bytes
	 *
	 *  @returns 0 if successful, or a negative error.
	 */
	int (*init_harvard)(struct llext_heap *h, void *instr_mem, size_t instr_bytes,
			    void *data_mem, size_t data_bytes);

	/**
	 * @brief Function to initialize the heap.
	 *
	 * @param[in] h Heap
	 * @param[in] mem Pointer to memory
	 * @param[in] bytes Size of memory in bytes
	 *
	 * @returns 0 if successful, or a negative error.
	 */
	int (*init)(struct llext_heap *h, void *mem, size_t bytes);

	/**
	 * @brief Function to uninitialize the heap.
	 *
	 * @param[in] h Heap
	 *
	 * @returns 0 on success, or a negative error.
	 */
	int (*uninit)(struct llext_heap *h);

	/**
	 * @brief Function to check if the heap is initialized.
	 *
	 * @param[in] h Heap
	 *
	 * @returns true if initialized, false otherwise.
	 */
	bool (*is_inited)(struct llext_heap *h);

	/**
	 * @brief Optional function to reset the heap to its initial state.
	 *
	 * @param[in] h Heap
	 *
	 */
	void (*reset)(struct llext_heap *h, struct llext *ext);

	/**
	 * @brief Function to allocate memory from the heap for llext metadata.
	 *
	 * @param[in] h Heap
	 * @param[in] bytes Number of bytes to allocate
	 *
	 * @returns Pointer to allocated memory, or NULL on failure.
	 */
	void *(*alloc_metadata)(struct llext_heap *h, size_t bytes);

	/**
	 * @brief Function to allocate memory from the heap for llext data region.
	 *
	 * @param[in] h Heap
	 * @param[in] ext Extension
	 * @param[in] align Alignment requirement
	 * @param[in] bytes Number of bytes to allocate
	 *
	 * @returns Pointer to allocated memory, or NULL on failure.
	 */
	void *(*aligned_alloc_data)(struct llext_heap *h, struct llext *ext, size_t align,
				    size_t bytes);
	/**
	 * @brief Function to allocate memory from the heap for llext instruction region.
	 *
	 * @param[in] h Heap
	 * @param[in] ext Extension
	 * @param[in] align Alignment requirement
	 * @param[in] bytes Number of bytes to allocate
	 *
	 * @returns Pointer to allocated memory, or NULL on failure.
	 */
	void *(*aligned_alloc_instr)(struct llext_heap *h, struct llext *ext, size_t align,
				     size_t bytes);

	/**
	 * @brief Function to free memory allocated for llext metadata.
	 *
	 * @param[in] h Heap
	 * @param[in] ptr Pointer to memory to free
	 */
	void (*free_metadata)(struct llext_heap *h, void *ptr);

	/**
	 * @brief Function to free memory allocated for llext data region.
	 *
	 * @param[in] h Heap
	 * @param[in] ext Extension
	 * @param[in] ptr Pointer to memory to free
	 */
	void (*free_data)(struct llext_heap *h, struct llext *ext, void *ptr);

	/**
	 * @brief Function to free memory allocated for llext instruction region.
	 *
	 * @param[in] h Heap
	 * @param[in] ext Extension
	 * @param[in] ptr Pointer to memory to free
	 */
	void (*free_instr)(struct llext_heap *h, struct llext *ext, void *ptr);
};

/** @cond ignore */
static inline int llext_heap_init_harvard(struct llext_heap *h, void *instr_mem, size_t instr_bytes,
					  void *data_mem, size_t data_bytes)
{
	return h->init_harvard(h, instr_mem, instr_bytes, data_mem, data_bytes);
}

static inline int llext_heap_init(struct llext_heap *h, void *mem, size_t bytes)
{
	return h->init(h, mem, bytes);
}

static inline int llext_heap_uninit(struct llext_heap *h)
{
	return h->uninit(h);
}

static inline bool llext_heap_is_inited(struct llext_heap *h)
{
	return h->is_inited(h);
}

static inline void llext_heap_reset(struct llext_heap *h, struct llext *ext)
{
	if (h->reset) {
		h->reset(h, ext);
	}
}

static inline void *llext_heap_alloc_metadata(struct llext_heap *h, size_t bytes)
{
	return h->alloc_metadata(h, bytes);
}

static inline void *llext_heap_aligned_alloc_data(struct llext_heap *h, struct llext *ext,
						  size_t align, size_t bytes)
{
	return h->aligned_alloc_data(h, ext, align, bytes);
}

static inline void *llext_heap_aligned_alloc_instr(struct llext_heap *h, struct llext *ext,
						   size_t align, size_t bytes)
{
	return h->aligned_alloc_instr(h, ext, align, bytes);
}

static inline void llext_heap_free_metadata(struct llext_heap *h, void *ptr)
{
	h->free_metadata(h, ptr);
}

static inline void llext_heap_free_data(struct llext_heap *h, struct llext *ext, void *ptr)
{
	h->free_data(h, ext, ptr);
}

static inline void llext_heap_free_instr(struct llext_heap *h, struct llext *ext, void *ptr)
{
	h->free_instr(h, ext, ptr);
}
/* @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_HEAP_H */
