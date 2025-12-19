/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEMREF_H_
#define ZEPHYR_INCLUDE_SYS_MEMREF_H_

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

/**
 * @file
 *
 * @brief Reference counted memory allocation API.
 */

/**
 * @defgroup memref Reference counted memory allocation
 * @ingroup memory_management
 * @{
 */

typedef void (*memref_free_cb_t)(void *mem);
typedef void (*memref_free_func_t)(void *be, void *mem);
typedef void *(*memref_alloc_func_t)(void *be, size_t size);

struct memref_backend {
/* @cond INTERNAL_HIDDEN */
	memref_alloc_func_t alloc;
	memref_free_func_t free;
	void *ctx;
/* @endcond */
};

/**
 * @cond INTERNAL_HIDDEN
 */
struct memref_ctrl_block {
	atomic_t ref_count;
	memref_free_cb_t free_cb;
	const struct memref_backend *backend;
};

#define MEMREF_CTRL_BLOCK_SIZE \
	ROUND_UP(sizeof(struct memref_ctrl_block), __alignof__(z_max_align_t))

/**
 * @endcond
 */

#define MEMREF_BACKEND_DEFINE(name, alloc_func, free_func, backend_context)	\
	const struct memref_backend name = {					\
		.alloc = alloc_func,						\
		.free = free_func,						\
		.ctx = backend_context,						\
	}


/** Allocate memory with reference counting.
 *
 * Initial reference count is one. Ownership must be released with
 * @ref memref_unref once the caller no longer needs it.
 *
 * @param backend Backend providing the raw memory.
 * @param size Size of the user-visible memory to allocate.
 * @param free_cb Optional callback invoked with the user pointer before the
 *                memory is returned to the backend when the last reference
 *                is dropped.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_alloc(const struct memref_backend *backend, size_t size, memref_free_cb_t free_cb);

/** Allocate zero-initialized memory with reference counting.
 *
 * See @ref memref_alloc for lifetime and alignment guarantees.
 *
 * @param backend Backend providing the raw memory.
 * @param nmemb Number of elements.
 * @param size Size of each element.
 * @param free_cb Callback function to free the memory when reference count reaches zero.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_calloc(const struct memref_backend *backend, size_t nmemb, size_t size,
		    memref_free_cb_t free_cb);

/** Decrease the reference count of the memory.
 *
 * If the reference count reaches zero the optional callback is invoked and
 * the memory is returned to the backend it was allocated from.
 *
 * The caller must hold a reference. Passing NULL is a no-op. The backend
 * and callback inherit the caller's execution context, so this must not be
 * called from ISR unless both are ISR-safe.
 *
 * @param mem Pointer to the memory to release.
 */
void memref_unref(void *mem);

/** Increase the reference count of the memory.
 *
 * @param mem Pointer to take ownership of.
 */
void memref_ref(void *mem);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SYS_MEMREF_H_ */
