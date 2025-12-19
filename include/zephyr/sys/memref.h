/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEMREF_H_
#define ZEPHYR_INCLUDE_SYS_MEMREF_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/ref.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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
typedef void *(*memref_alloc_func_t)(void *ctx, size_t size);
typedef void (*memref_free_func_t)(void *ctx, void *mem);
/* @endcond */


/** Callback invoked with the user pointer before the memory is returned to the
 *  backend when the last reference is dropped.
 *
 *  @param mem Pointer to the user-visible memory.
 */
typedef void (*memref_destroy_cb_t)(void *mem);

/** Variable length memory backend providing allocation and free functions.
 *
 * The backend is the function wrapper that provides the raw memory to the memref API. It is
 * responsible for allocating and freeing memory, and may use any underlying memory management
 * mechanism. A variable length backend honours the requested allocation size.
 */
struct memref_backend {
/* @cond INTERNAL_HIDDEN */
	memref_alloc_func_t alloc;
	memref_free_func_t free;
	void *ctx;
/* @endcond */
};

/** Fixed block size memory backend.
 *
 * A block backend wraps a @ref memref_backend whose underlying allocator serves fixed size blocks
 * decided at compile or initialization time. The requested allocation size is disregarded; the
 * block size lives entirely in the underlying allocator.
 *
 * The distinct type exists so that calling @ref memref_alloc on a block backend, or
 * @ref memref_block_alloc on a variable length backend, is a compile time error.
 */
struct memref_block_backend {
/* @cond INTERNAL_HIDDEN */
	struct memref_backend base;
/* @endcond */
};

/** Size of the memref control block.
 *
 * A memref allocation starts with the control block and the payload follows it, so a backend
 * must provide this many bytes on top of the payload it wants to expose.
 *
 * The value depends on the enabled options, as @kconfig{CONFIG_MEMREF_DESTROY_CB} and
 * @kconfig{CONFIG_ASSERT} each add a field to the control block, so it must not be hardcoded.
 */
#define MEMREF_CTRL_BLOCK_SIZE \
	ROUND_UP(sizeof(struct memref_ctrl_block), __alignof__(z_max_align_t))

/**
 * @cond INTERNAL_HIDDEN
 */
struct memref_ctrl_block {
	struct sys_ref ref;
#ifdef CONFIG_ASSERT
	uint32_t magic;
#endif
#ifdef CONFIG_MEMREF_DESTROY_CB
	memref_destroy_cb_t destroy;
#endif
	const struct memref_backend *backend;
};

void *z_memref_k_heap_alloc(void *ctx, size_t size);
void z_memref_k_heap_free(void *ctx, void *mem);
void *z_memref_k_mem_slab_alloc(void *ctx, size_t size);
void z_memref_k_mem_slab_free(void *ctx, void *mem);
/**
 * @endcond
 */

/** Define a variable length memory backend usable with the memref allocation API.
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

/** Define a fixed block size memory backend usable with the memref allocation API.
 *
 * The alloc function receives the allocation size for signature compatibility but must disregard
 * it, as the block size is decided by the underlying allocator.
 *
 * @param name Name of the backend.
 * @param alloc_func Function to allocate a block from the backend.
 * @param free_func Function to free a block back to the backend.
 * @param backend_context Context pointer passed to the alloc and free functions.
 */
#define MEMREF_BLOCK_BACKEND_DEFINE(name, alloc_func, free_func, backend_context)	\
	const struct memref_block_backend name = {					\
		.base = {								\
			.alloc = alloc_func,						\
			.free = free_func,						\
			.ctx = backend_context,						\
		},									\
	}

/** Define a k_heap and a matching variable length memref backend.
 *
 * @param name Name of the backend.
 * @param size Size of the heap in bytes.
 */
#define MEMREF_K_HEAP_DEFINE(name, size)						\
	K_HEAP_DEFINE(name##_heap, size);						\
	MEMREF_BACKEND_DEFINE(name, z_memref_k_heap_alloc, z_memref_k_heap_free,	\
			      &name##_heap)

/** Define a k_mem_slab and a matching fixed block size memref backend.
 *
 * The slab block size accounts for the memref control block, so @p block_size is the user-visible
 * size available from every block.
 *
 * @p align is the alignment of the slab block, not of the payload.
 *
 * @param name Name of the block backend.
 * @param block_size User-visible size of each block in bytes.
 * @param count Number of blocks in the slab.
 * @param align Block alignment in bytes, a power of two.
 */
#define MEMREF_K_MEM_SLAB_DEFINE(name, block_size, count, align)				\
	BUILD_ASSERT((block_size) > 0, "memref block_size must be greater than zero");		\
	K_MEM_SLAB_DEFINE(name##_slab, MEMREF_CTRL_BLOCK_SIZE + (block_size), count, align);	\
	MEMREF_BLOCK_BACKEND_DEFINE(name, z_memref_k_mem_slab_alloc, z_memref_k_mem_slab_free,	\
			     &name##_slab)

/** Allocate memory with reference counting.
 *
 * Initial reference count is one. Ownership must be released with
 * @ref memref_unref once the caller no longer needs it.
 *
 * @note The returned payload inherits the alignment of the backend. The control block is padded
 *       so the backend alignment is carried over to the payload, but memref does not strengthen
 *       it: a backend returning 8 byte aligned memory yields an 8 byte aligned payload.
 *
 * @param backend Variable length backend providing the raw memory.
 * @param size Size of the user-visible memory to allocate.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_alloc(const struct memref_backend *backend, size_t size);

/** Allocate a fixed size block with reference counting.
 *
 * Initial reference count is one. Ownership must be released with @ref memref_unref once the
 * caller no longer needs it. The block size is decided by the underlying allocator.
 *
 * @param backend Fixed block size backend providing the raw memory.
 *
 * @return Pointer to the allocated block, or NULL on failure.
 */
void *memref_block_alloc(const struct memref_block_backend *backend);

/** Allocate memory with reference counting and a destroy callback.
 *
 * See @ref memref_alloc for lifetime and alignment guarantees.
 *
 * @kconfig_dep{CONFIG_MEMREF_DESTROY_CB}
 *
 * @param backend Variable length backend providing the raw memory.
 * @param size Size of the user-visible memory to allocate.
 * @param destroy Callback invoked with the user pointer before the memory is
 *                returned to the backend when the last reference is dropped.
 *                May be NULL.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *memref_alloc_with_destroy(const struct memref_backend *backend, size_t size,
				memref_destroy_cb_t destroy);

/** Allocate a fixed size block with reference counting and a destroy callback.
 *
 * See @ref memref_block_alloc for lifetime and alignment guarantees.
 *
 * @kconfig_dep{CONFIG_MEMREF_DESTROY_CB}
 *
 * @param backend Fixed block size backend providing the raw memory.
 * @param destroy Callback invoked with the user pointer before the block is
 *                returned to the backend when the last reference is dropped.
 *                May be NULL.
 *
 * @return Pointer to the allocated block, or NULL on failure.
 */
void *memref_block_alloc_with_destroy(const struct memref_block_backend *backend,
				      memref_destroy_cb_t destroy);

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
 * The caller must already hold a reference, which keeps the block alive and its reference count
 * non-zero. This cannot be used to establish the first reference, see @ref memref_alloc, nor to
 * revive a block whose count has already reached zero, as such a block may already have been
 * returned to its backend.
 *
 * @param mem Pointer to take ownership of.
 */
void memref_ref(void *mem);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MEMREF_H_ */
