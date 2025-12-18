/*
 * Copyright (c) 2025, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ELASTIPOOL_H_
#define ZEPHYR_INCLUDE_SYS_ELASTIPOOL_H_

#include <stdint.h>

#include <zephyr/kernel.h>
#ifdef CONFIG_SYS_HASH_MAP
#include <zephyr/sys/hash_map.h>
#endif
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup elastipool_apis Elastipool
 * @ingroup memory_management
 *
 * @brief Elastipool (Elastic Object Pool) API
 *
 * Object pools provide a bounded, semi-dynamic means of memory management, with guarantees. They
 * are typically used when a number of homomorphic objects are required, each of the same fixed
 * size. Object pools guarantee a set number of allocations will be successful in a narrow scope,
 * and that allocation and deallocation operations have predictable, bounded latencies.
 *
 * Elastipool bridges the gap between having a number of statically-allocated, guaranteed
 * fixed-size allocations, and the ability to grow dynamically based on demand, up to a maximum
 * pool size.
 *
 * @{
 */

struct sys_hashmap;

/**
 * @brief Elastipool configuration
 *
 * A structure used to contain constant configuration details for a specific elastipool instance.
 *
 * When `min_obj` is zero, then the elastipool instance only performs dynamic allocation.
 * When `min_obj` is non-zero, the elastipool instance statically allocates the specified number
 * of objects and guarantees they are allocatable. The `storage` field specifies the memory region
 * claimed by static allocation.
 * When `min_obj` is equal to `max_obj`, the elastipool instance only supports guaranteed
 * allocation. When `max_obj` is greater than `min_obj`, the elastipool instance is capable of
 * allocating an additional `max_obj - min_obj` objects from the heap. Heap allocations are not
 * guaranteed.
 *
 * The `sys_elastipool_config` structure is typically compile-time constant and stored in read-only
 * memory.
 */
struct sys_elastipool_config {
	/** The un-padded size, in bytes, of each element */
	size_t obj_size;
	/** The alignment requirement, in bytes, of each element */
	size_t obj_align;
	/** The minimum number of elements guaranteed to be available */
	size_t min_obj;
	/** The maximum possible number of objects allocatable by the pool */
	size_t max_obj;
	/** A pointer to storage for statically allocated (guaranteed) objects */
	uint8_t *storage;
	/** A pointer to a bitmap that tracks static allocation */
	unsigned long *bmp;
	/** A pointer to the hash map that tracks dynamic (heap-based) allocation */
	struct sys_hashmap *map;
};

/**
 * @brief Elastipool runtime data
 *
 * A structure that contains runtime data for each elastipool instance.
 */
struct sys_elastipool_data {
	/** The number of objects allocated by the elastipool instance at any given time */
	size_t pool_size;
};

/* forward declaration */
struct sys_elastipool;

/**
 * @brief Elastipool allocation function type
 *
 * A function pointer type that implements the semantics of elastipool allocation.
 *
 * @param pool the elastipool instance
 * @param[out] obj storage for the pointer resulting from a successful allocation
 *
 * @retval 0 on success
 * @retval -ENOMEM when an allocation was not successful
 */
typedef int (*sys_elastipool_api_alloc_t)(const struct sys_elastipool *pool, void **obj,
					  uint32_t flags);

/**
 * @brief Elastipool pointer validation
 *
 * A function pointer type that implements the semantics of elastipool pointer validation.
 *
 * @param pool the elastipool instance
 * @param obj a pointer to check
 *
 * @retval 0 on success
 * @retval -EINVAL when an invalid pointer is provided
 */
typedef int (*sys_elastipool_api_check_t)(const struct sys_elastipool *pool, const void *ptr);

/**
 *
 * @brief Elastipool clear function type
 *
 * A function pointer type that implements the semantics of elastipool clear operation.
 *
 * Clearing an elastipool instance deallocates all previous allocations using that instance
 * and zeros the number of current allocations, effectively resetting the elastipool instance
 * to the same state it occupied during system initialization.
 *
 * Memory that is used for object storage is not zero'ed with this type of call.
 *
 * @param pool the elastipool instance to clear
 */
typedef void (*sys_elastipool_api_clear_t)(const struct sys_elastipool *pool);

/**
 * @brief Elastipool de-allocation function type
 *
 * A function pointer type that implements the semantics of elastipool de-allocation.
 *
 * @param pool the elastipool instance
 * @param obj a pointer to the memory to be deallocated
 *
 * @retval 0 on success
 * @retval -EINVAL when an invalid pointer is provided
 */
typedef int (*sys_elastipool_api_free_t)(const struct sys_elastipool *pool, const void *ptr);

/**
 * @brief Elastipool heap-allocation function type
 *
 * A function pointer type that implements the semantics of elastipool heap allocation. This part
 * of the API is not intended for users.
 *
 * Elastipool heap allocators behave similarly to `aligned_alloc()` when @p size is non-zero.
 * When @p size is zero, elastipool heap allocators behave like `free()` and de-allocate memory
 * pointed-to by @p ptr.
 *
 * @param ptr a pointer to previously allocated memory, when @p size is zero
 * @param size the size of the allocation, in bytes, or zero to de-allocate memory
 * @param align the alignment requirement of the allocation, when @p size is non-zero
 * @param flags allocator-specific flags
 *
 * @return `NULL` on failure or a non-`NULL` pointer to allocated memory on success.
 *
 * @see <a href="https://en.cppreference.com/w/c/memory/aligned_alloc">aligned_alloc</a>
 */
typedef void *(*sys_elastipool_api_heap_alloc_t)(const void *ptr, size_t size, size_t align,
						 uint32_t flags);

/**
 * @brief Generic Elastipool API
 */
struct sys_elastipool_api {
	sys_elastipool_api_alloc_t alloc;
	sys_elastipool_api_check_t check;
	sys_elastipool_api_clear_t clear;
	sys_elastipool_api_free_t free;
	sys_elastipool_api_heap_alloc_t heap_alloc;
};

/**
 * @brief Elastic Object Pool
 *
 * This is the top-level structure for elastipool instances.
 */
struct sys_elastipool {
	/** The elastipool API */
	const struct sys_elastipool_api *api;
	/** Constant elastipool configuration */
	const struct sys_elastipool_config *config;
	/** Runtime-modified elastipool data */
	struct sys_elastipool_data *data;
};

/**
 * @brief Define and initialize an elastipool instance at runtime
 *
 * Some parameters are optional or unused depending on the values of @p min_obj and @p max_obj.
 *
 * - @p storage is optional when @p min_obj is equal to zero
 * - @p bmp is optional when @p min_obj is equal to 0
 * - @p map is optional when @p max_obj is equal to @p min_obj
 * - @p allocator is optional when @p max_obj is equal to @p min_obj
 * - if @p allocator is `NULL`, and @p max_obj is greater than @p min_obj, the default allocator is
 * used
 *
 * @param pool pointer to storage of the elastipool instance
 * @param api pointer to storage for the elastipool api
 * @param config pointer to storage for the elastipool configuration
 * @param data pointer to storagge for the elastipool runtime data
 * @param obj_size the un-padded size of each object (must be > 0)
 * @param obj_align the alignment requirement of each object (must be >= 0)
 * @param min_obj the minimum number of objects guaranteed to be allocatable by the elastipool
 * instance
 * @param max_obj the maximum number of objects possible to allocate by the elastipool instance
 * @param storage pointer to storage for static allocations (optional)
 * @param bmp pointer to bitmap storage for static allocations (optional)
 * @param map pointer to a hash map object for dynamic allocations (optional)
 * @param allocator a user-supplied heap allocator for object-specific allocation requirements
 * (optional)
 *
 * @return 0 on success
 * @return -EINVAL if an argument was invalid
 */
int sys_elastipool_init(struct sys_elastipool *pool, struct sys_elastipool_api *api,
			struct sys_elastipool_config *config, struct sys_elastipool_data *data,
			size_t obj_size, size_t obj_align, size_t min_obj, size_t max_obj,
			uint8_t *storage, unsigned long *bmp, struct sys_hashmap *map,
			sys_elastipool_api_heap_alloc_t allocator);

/**
 * @brief Allocate an object using an elastic object pool
 *
 * @param pool the elastipool instance
 * @param[out] obj storage for the pointer resulting from a successful allocation
 *
 * @retval 0 on success
 * @retval -ENOMEM when an allocation was not successful
 */
static inline int sys_elastipool_alloc(const struct sys_elastipool *pool, void **obj)
{
	return pool->api->alloc(pool, obj, 0);
}

/**
 * @brief Allocate an object using an elastic object pool
 *
 * @param pool the elastipool instance
 * @param[out] obj storage for the pointer resulting from a successful allocation
 * @param flags allocation flags that are passed to the elastipool heap allocator when heap
 * allocation is necessary
 *
 * @retval 0 on success
 * @retval -ENOMEM when an allocation was not successful
 */
static inline int sys_elastipool_alloc_flags(const struct sys_elastipool *pool, void **obj,
					     uint32_t flags)
{
	return pool->api->alloc(pool, obj, flags);
}

/**
 * @brief Clear an elastic object pool instance of all previous allocations
 *
 * @param pool the elastipool instance
 */
static inline void sys_elastipool_clear(const struct sys_elastipool *pool)
{
	pool->api->clear(pool);
}

/**
 * @brief De-allocate an object obtained from an elastic object pool instance
 *
 * @param pool the elastipool instance
 * @param obj a pointer to the memory to be deallocated
 *
 * @retval 0 on success
 * @retval -EINVAL when an invalid pointer is provided
 */
static inline int sys_elastipool_free(const struct sys_elastipool *pool, const void *obj)
{
	return pool->api->free(pool, obj);
}

/**
 * @brief Verify that an object obtained from an elastic object pool instance
 *
 * @param pool the elastipool instance
 * @param obj a pointer to the memory to be deallocated
 *
 * @retval 0 on success
 * @retval -EINVAL when an invalid pointer is provided
 */
static inline int sys_elastipool_check(const struct sys_elastipool *pool, const void *obj)
{
	return pool->api->check(pool, obj);
}

/** @cond INTERNAL_HIDDEN */

int sys_elastipool_api_alloc_dyn(const struct sys_elastipool *pool, void **obj, uint32_t flags);
int sys_elastipool_api_alloc_elastic(const struct sys_elastipool *pool, void **obj, uint32_t flags);
int sys_elastipool_api_alloc_static(const struct sys_elastipool *pool, void **obj, uint32_t flags);

int sys_elastipool_api_check_dyn(const struct sys_elastipool *pool, const void *ptr);
int sys_elastipool_api_check_elastic(const struct sys_elastipool *pool, const void *ptr);
int sys_elastipool_api_check_static(const struct sys_elastipool *pool, const void *ptr);

void sys_elastipool_api_clear_dyn(const struct sys_elastipool *pool);
void sys_elastipool_api_clear_elastic(const struct sys_elastipool *pool);
void sys_elastipool_api_clear_static(const struct sys_elastipool *pool);

int sys_elastipool_api_free_dyn(const struct sys_elastipool *pool, const void *ptr);
int sys_elastipool_api_free_elastic(const struct sys_elastipool *pool, const void *ptr);
int sys_elastipool_api_free_static(const struct sys_elastipool *pool, const void *ptr);

void *sys_elastipool_api_kernel_heap_alloc(const void *ptr, size_t size, size_t align,
					   uint32_t flags);
void *sys_elastipool_api_thread_heap_alloc(const void *ptr, size_t size, size_t align,
					   uint32_t flags);

#define SYS_ELASTIPOOL_ALLOC_FN(min, max)                                                          \
	(((min) == 0) ? sys_elastipool_api_alloc_dyn                                               \
		      : (((min) == (max)) ? sys_elastipool_api_alloc_static                        \
					  : sys_elastipool_api_alloc_elastic))
#define SYS_ELASTIPOOL_CHECK_FN(min, max)                                                          \
	(((min) == 0) ? sys_elastipool_api_check_dyn                                               \
		      : (((min) == (max)) ? sys_elastipool_api_check_static                        \
					  : sys_elastipool_api_check_elastic))
#define SYS_ELASTIPOOL_CLEAR_FN(min, max)                                                          \
	(((min) == 0) ? sys_elastipool_api_clear_dyn                                               \
		      : (((min) == (max)) ? sys_elastipool_api_clear_static                        \
					  : sys_elastipool_api_clear_elastic))
#define SYS_ELASTIPOOL_FREE_FN(min, max)                                                           \
	(((min) == 0) ? sys_elastipool_api_free_dyn                                                \
		      : (((min) == (max)) ? sys_elastipool_api_free_static                         \
					  : sys_elastipool_api_free_elastic))
#define SYS_ELASTIPOOL_KERNEL_HEAP_ALLOC_FN(min, max)                                              \
	(((max) > (min)) ? sys_elastipool_api_kernel_heap_alloc : NULL)
#define SYS_ELASTIPOOL_THREAD_HEAP_ALLOC_FN(min, max)                                              \
	(((max) > (min)) ? sys_elastipool_api_thread_heap_alloc : NULL)
#define SYS_ELASTIPOOL_SLAB_STORAGE(name, min, max) _elastipool_slab_storage_##name
#define SYS_ELASTIPOOL_SLAB_BITMAP(name, min, max) _elastipool_slab_bitmap_##name

#ifdef CONFIG_SYS_HASH_MAP
/* pool operations run under the caller's spinlock: the map uses the kernel heap when there is one */
void *sys_elastipool_map_alloc(void *ptr, size_t size);
#define SYS_ELASTIPOOL_DEFINE_MAP(name)                                                            \
	__maybe_unused SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(                                 \
		_elastipool_map_##name, sys_hash32, sys_elastipool_map_alloc,                    \
		SYS_HASHMAP_CONFIG(SIZE_MAX, SYS_HASHMAP_DEFAULT_LOAD_FACTOR))
#define SYS_ELASTIPOOL_MAP(name, min, max) (((max) > (min)) ? &_elastipool_map_##name : NULL)
#else
#define SYS_ELASTIPOOL_DEFINE_MAP(name)
#define SYS_ELASTIPOOL_MAP(name, min, max) NULL
#endif

/** @endcond */

/**
 * @brief Define and initialize an elastipool instance at compile time (advanced)
 *
 * This is the advanced interface that may be used as an alternative to @ref SYS_ELASTIPOOL_DEFINE
 * or @ref SYS_ELASTIPOOL_DEFINE_STATIC.
 *
 * @param name the name of the elastipool instance
 * @param size the un-padded size of each object
 * @param align the alignment requirement of each object
 * @param min the minimum number of objects guaranteed to be allocatable by the elastipool instance
 * @param max the maximum number of objects possible to allocate by the elastipool instance
 * @param allocator a user-supplied heap allocator for object-specific allocation requirements
 * @param slab_storage pointer to a block of appropriately sized and aligned memory for pool
 * allocation
 *
 * @note Users may specify the `static` keyword after @p allocator in case the elastipool instance
 * should not be visible in the global namespace.
 */
#define SYS_ELASTIPOOL_DEFINE_ADVANCED(name, size, align, min, max, allocator, slab_storage, ...)  \
	BUILD_ASSERT((size) > 0, "size must be > 0");                                              \
	BUILD_ASSERT((align) >= 0, "align must be >= 0");                                          \
	BUILD_ASSERT((min) >= 0, "min must be >= 0");                                              \
	BUILD_ASSERT((max) >= (min), "max must be >= min");                                        \
	BUILD_ASSERT(((align) == 0) || IS_POWER_OF_TWO(align), "align must be a power of two");    \
	__maybe_unused static unsigned long _elastipool_slab_bitmap_##name[DIV_ROUND_UP(           \
		(min), BITS_PER_BYTE * sizeof(unsigned long))];                                    \
	SYS_ELASTIPOOL_DEFINE_MAP(name);                                                           \
	static const struct sys_elastipool_config _elastipool_config_##name = {                    \
		.obj_size = (size),                                                                \
		.obj_align = (((align) == 0) ? 1 : (align)),                                       \
		.min_obj = (min),                                                                  \
		.max_obj = IS_ENABLED(CONFIG_SYS_HASH_MAP) ? (max) : (min),                        \
		.storage = (uint8_t *)slab_storage,                                                \
		.bmp = SYS_ELASTIPOOL_SLAB_BITMAP(name, (min), (max)),                             \
		.map = SYS_ELASTIPOOL_MAP(name, (min), (max)),                                     \
	};                                                                                         \
	static struct sys_elastipool_data _elastipool_data_##name;                                 \
	static const struct sys_elastipool_api _elastipool_api_##name = {                          \
		.alloc = SYS_ELASTIPOOL_ALLOC_FN((min), (max)),                                    \
		.check = SYS_ELASTIPOOL_CHECK_FN((min), (max)),                                    \
		.clear = SYS_ELASTIPOOL_CLEAR_FN((min), (max)),                                    \
		.free = SYS_ELASTIPOOL_FREE_FN((min), (max)),                                      \
		.heap_alloc = (allocator),                                                         \
	};                                                                                         \
	__VA_ARGS__ const struct sys_elastipool name = {                                           \
		.api = &_elastipool_api_##name,                                                    \
		.config = &_elastipool_config_##name,                                              \
		.data = &_elastipool_data_##name,                                                  \
	}

/**
 * @brief Define and initialize an elastipool instance at compile time
 *
 * @param name the name of the elastipool instance
 * @param size the un-padded size of each object
 * @param align the alignment requirement of each object
 * @param min the minimum number of objects guaranteed to be allocatable by the elastipool instance
 * @param max the maximum number of objects possible to allocate by the elastipool instance
 *
 * @note Using this macro to define an elastipool instance will result in @p name being visible in
 * the global C namespace. Consider @ref SYS_ELASTIPOOL_DEFINE_STATIC for static definition.
 */
#define SYS_ELASTIPOOL_DEFINE(name, size, align, min, max, ...)                                    \
	__maybe_unused __noinit __aligned(WB_UP(align)) static uint8_t                             \
		_elastipool_slab_storage_##name[ROUND_UP((size), (align)) * (min)];                \
	SYS_ELASTIPOOL_DEFINE_ADVANCED(name, (size), (align), (min), (max),                        \
				       SYS_ELASTIPOOL_THREAD_HEAP_ALLOC_FN((min), (max)),          \
				       _elastipool_slab_storage_##name, __VA_ARGS__)

/**
 * @brief Statically define and initialize an elastipool instance at compile time
 *
 * @param name the name of the elastipool instance
 * @param size the un-padded size of each object
 * @param align the alignment requirement of each object
 * @param min the minimum number of objects guaranteed to be allocatable by the elastipool instance
 * @param max the maximum number of objects possible to allocate by the elastipool instance
 */
#define SYS_ELASTIPOOL_DEFINE_STATIC(name, size, align, min, max)                                  \
	SYS_ELASTIPOOL_DEFINE(name, (size), (align), (min), (max), static)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ELASTIPOOL_H_ */
