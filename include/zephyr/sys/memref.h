/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
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

/* @cond INTERNAL_HIDDEN */
typedef void (*memref_free_func_t)(void *be, void *mem);
typedef void *(*memref_alloc_func_t)(void *be, size_t size);
/* @endcond */

#ifdef CONFIG_MEMREF_DESTROY_CB

/** Callback invoked with the user pointer before the memory is returned to the
 *  backend when the last reference is dropped.
 *
 *  @param mem Pointer to the user-visible memory.
 */
typedef void (*memref_destroy_cb_t)(void *mem);

#endif

/** Memory backend providing allocation and free functions.
 *
 * The backend is the function wrapper that provides the raw memory to the memref API. It is
 * responsible for allocating and freeing memory, and may use any underlying memory management
 * mechanism.
 */
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
#ifdef CONFIG_MEMREF_DESTROY_CB
	memref_destroy_cb_t destroy;
#endif
	const struct memref_backend *backend;
};

#define MEMREF_CTRL_BLOCK_SIZE \
	ROUND_UP(sizeof(struct memref_ctrl_block), __alignof__(z_max_align_t))

/**
 * @endcond
 */

/** Define a memory backend usable with the memref allocation API.
 *
 * @param name Name of the backend.
 * @param alloc_func Function to allocate memory from the backend.
 * @param free_func Function to free memory back to the backend.
 * @param backend_context Context pointer passed to the alloc and free functions.
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
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_alloc(const struct memref_backend *backend, size_t size);

/** Allocate zero-initialized memory with reference counting.
 *
 * See @ref memref_alloc for lifetime and alignment guarantees.
 *
 * @param backend Backend providing the raw memory.
 * @param nmemb Number of elements.
 * @param size Size of each element.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_calloc(const struct memref_backend *backend, size_t nmemb, size_t size);

#ifdef CONFIG_MEMREF_DESTROY_CB
/** Allocate memory with reference counting and a destroy callback.
 *
 * See @ref memref_alloc for lifetime and alignment guarantees.
 *
 * @kconfig_dep{CONFIG_MEMREF_DESTROY_CB}
 *
 * @param backend Backend providing the raw memory.
 * @param size Size of the user-visible memory to allocate.
 * @param destroy Callback invoked with the user pointer before the memory is
 *                returned to the backend when the last reference is dropped.
 *                May be NULL.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_alloc_cb(const struct memref_backend *backend, size_t size,
		      memref_destroy_cb_t destroy);

/** Allocate zero-initialized memory with reference counting and a destroy callback.
 *
 * See @ref memref_alloc_cb for lifetime and alignment guarantees.
 *
 * @kconfig_dep{CONFIG_MEMREF_DESTROY_CB}
 *
 * @param backend Backend providing the raw memory.
 * @param nmemb Number of elements.
 * @param size Size of each element.
 * @param destroy Callback invoked with the user pointer before the memory is
 *                returned to the backend when the last reference is dropped.
 *                May be NULL.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_calloc_cb(const struct memref_backend *backend, size_t nmemb, size_t size,
		       memref_destroy_cb_t destroy);
#endif /* CONFIG_MEMREF_DESTROY_CB */

/** Decrease the reference count of the memory.
 *
 * If the reference count reaches zero the optional destroy callback is invoked and
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
