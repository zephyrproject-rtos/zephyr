/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_HASHMAP_API_H_
#define ZEPHYR_INCLUDE_SYS_HASHMAP_API_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/hash_function.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup hashmap_apis Hashmap
 * @ingroup datastructure_apis
 *
 * @brief Hashmap (Hash Table) API
 *
 * Hashmaps (a.k.a Hash Tables) sacrifice space for speed. All operations
 * on a Hashmap (insert, delete, search) are O(1) complexity (on average).
 *
 * @defgroup hashmap_implementations Hashmap Implementations
 * @ingroup hashmap_apis
 *
 * @addtogroup hashmap_apis
 * @{
 */

/**
 * @brief Generic Hashmap iterator interface
 *
 * @note @a next should not be used without first checking
 * @ref sys_hashmap_iterator_has_next
 */
struct sys_hashmap_iterator {
	/** Pointer to the associated Hashmap */
	const struct sys_hashmap *map;
	/** Modify the iterator in-place to point to the next Hashmap entry */
	void (*next)(struct sys_hashmap_iterator *it);
	/** Implementation-specific iterator state */
	void *state;
	/** Key associated with the current entry */
	uint64_t key;
	/** Value associated with the current entry */
	uint64_t value;
	/** Number of entries in the map */
	const size_t size;
	/** Number of entries already iterated */
	size_t pos;
};

/**
 * @brief Check if a Hashmap iterator has a next entry
 *
 * @param it Hashmap iterator
 * @return true if there is a next entry
 * @return false if there is no next entry
 */
static inline bool sys_hashmap_iterator_has_next(const struct sys_hashmap_iterator *it)
{
	return it->pos < it->size;
}

/**
 * @brief Allocator interface for @ref sys_hashmap
 *
 * The Hashmap allocator can be any allocator that behaves similarly to `realloc()` with the
 * additional specification that the allocator behaves like `free()` when @p new_size is zero.
 *
 * @param ptr Previously allocated memory region or `NULL` to make a new vallocation.
 * @param new_size the new size of the allocation, in bytes.
 *
 * @see <a href="https://en.cppreference.com/w/c/memory/realloc">realloc</a>
 */
typedef void *(*sys_hashmap_allocator_t)(void *ptr, size_t new_size);

/**
 * @brief In-place iterator constructor for @ref sys_hashmap
 *
 * Construct an iterator, @p it, for @p map.
 *
 * @param map Hashmap to iterate over.
 * @param it Iterator to initialize.
 */
typedef void (*sys_hashmap_iterator_t)(const struct sys_hashmap *map,
				       struct sys_hashmap_iterator *it);

/**
 * @brief Callback interface for @ref sys_hashmap
 *
 * This callback is used by some Hashmap methods.
 *
 * @param key Key corresponding to @p value
 * @param value Value corresponding to @p key
 * @param cookie User-specified variable
 */
typedef void (*sys_hashmap_callback_t)(uint64_t key, uint64_t value, void *cookie);

/**
 * @brief Clear all entries contained in a @ref sys_hashmap
 *
 * @note If the values in a particular Hashmap are
 *
 * @param map Hashmap to clear
 * @param cb Callback to call for each entry
 * @param cookie User-specified variable
 */
typedef void (*sys_hashmap_clear_t)(struct sys_hashmap *map, sys_hashmap_callback_t cb,
				    void *cookie);

/**
 * @brief Insert a new entry into a @ref sys_hashmap
 *
 * Insert a new @p key - @p value pair into @p map.
 *
 * @param map Hashmap to insert into
 * @param key Key to associate with @p value
 * @param value Value to associate with @p key
 * @param old_value Location to store the value previously associated with @p key or `NULL`
 * @retval 0 if @p value was inserted for an existing key, in which case @p old_value will contain
 * the previous value
 * @retval 1 if a new entry was inserted for the @p key - @p value pair
 * @retval -ENOMEM if memory allocation failed
 */
typedef int (*sys_hashmap_insert_t)(struct sys_hashmap *map, uint64_t key, uint64_t value,
				    uint64_t *old_value);

/**
 * @brief Remove an entry from a @ref sys_hashmap
 *
 * Erase the entry associated with key @p key, if one exists.
 *
 * @param map Hashmap to remove from
 * @param key Key to remove from @p map
 * @param value Location to store a potential value associated with @p key or `NULL`
 *
 * @retval true if @p map was modified as a result of this operation.
 * @retval false if @p map does not contain a value associated with @p key.
 */
typedef bool (*sys_hashmap_remove_t)(struct sys_hashmap *map, uint64_t key, uint64_t *value);

/**
 * @brief Get a value from a @ref sys_hashmap
 *
 * Look-up the @ref uint64_t associated with @p key, if one exists.
 *
 * @param map Hashmap to search through
 * @param key Key with which to search @p map
 * @param value Location to store a potential value associated with @p key or `NULL`
 *
 * @retval true if @p map contains a value associated with @p key.
 * @retval false if @p map does not contain a value associated with @p key.
 */
typedef bool (*sys_hashmap_get_t)(const struct sys_hashmap *map, uint64_t key, uint64_t *value);

/**
 * @brief Generic Hashmap API
 */
struct sys_hashmap_api {
	/** Iterator constructor (in-place) */
	sys_hashmap_iterator_t iter;
	/** Clear the hash table, freeing all resources */
	sys_hashmap_clear_t clear;
	/** Insert a key-value pair into the Hashmap */
	sys_hashmap_insert_t insert;
	/** Remove a key-value pair from the Hashmap */
	sys_hashmap_remove_t remove;
	/** Retrieve the value associated with a given key from the Hashmap */
	sys_hashmap_get_t get;
};

/**
 * @brief Generic Hashmap configuration
 *
 * When there is a known limit imposed on the number of entries in the Hashmap,
 * users should specify that via @a max_size. When the Hashmap should have
 * no artificial limitation in size (and be bounded only by the provided
 * allocator), users should specify `SIZE_MAX` here.
 *
 * The @a load_factor is defined as the size of the Hashmap divided by the
 * number of buckets. In this case, the size of the Hashmap is defined as
 * the number of valid entries plus the number of invalidated entries.
 *
 * The @a initial_n_buckets is defined as the number of buckets to allocate
 * when moving from size 0 to size 1 such that the maximum @a load_factor
 * property is preserved.
 */
struct sys_hashmap_config {
	/** Maximum number of entries */
	size_t max_size;
	/** Maximum load factor expressed in hundredths */
	uint8_t load_factor;
	/** Initial number of buckets to allocate */
	uint8_t initial_n_buckets;
};

/**
 * @brief Initializer for @p sys_hashmap_config
 *
 * This macro helps to initialize a structure of type @p sys_hashmap_config.
 *
 * @param _max_size Maximum number of entries
 * @param _load_factor Maximum load factor of expressed in hundredths
 */
#define SYS_HASHMAP_CONFIG(_max_size, _load_factor)                                                \
	{                                                                                          \
		.max_size = (size_t)_max_size, .load_factor = (uint8_t)_load_factor,               \
		.initial_n_buckets = NHPOT(DIV_ROUND_UP(100, _load_factor)),                   \
	}

/**
 * @brief Generic Hashmap data
 *
 * @note When @a size is zero, @a buckets should be `NULL`.
 */
struct sys_hashmap_data {
	/** Pointer for implementation-specific Hashmap storage */
	void *buckets;
	/** The number of buckets currently allocated */
	size_t n_buckets;
	/** The number of entries currently in the Hashmap */
	size_t size;
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HASHMAP_API_H_ */
