/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @addtogroup hashmap_apis
 * @{
 */

#ifndef ZEPHYR_INCLUDE_SYS_HASH_MAP_H_
#define ZEPHYR_INCLUDE_SYS_HASH_MAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/hash_map_api.h>
#include <zephyr/sys/hash_map_cxx.h>
#include <zephyr/sys/hash_map_oa_lp.h>
#include <zephyr/sys/hash_map_sc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Declare a Hashmap (advanced)
 *
 * Declare a Hashmap with control over advanced parameters.
 *
 * @note The allocator @p _alloc is used for allocating internal Hashmap
 * entries and does not interact with any user-provided keys or values.
 *
 * @param _name Name of the Hashmap.
 * @param _api API pointer of type @ref sys_hashmap_api.
 * @param _config_type Variant of @ref sys_hashmap_config.
 * @param _data_type Variant of @ref sys_hashmap_data.
 * @param _hash_func Hash function pointer of type @ref sys_hash_func32_t.
 * @param _alloc_func Allocator function pointer of type @ref sys_hashmap_allocator_t.
 * @param ... Variant-specific details for @p _config_type.
 */
#define SYS_HASHMAP_DEFINE_ADVANCED(_name, _api, _config_type, _data_type, _hash_func,             \
				    _alloc_func, ...)                                              \
	const struct _config_type _name##_config = __VA_ARGS__;                                    \
	struct _data_type _name##_data;                                                            \
	struct sys_hashmap _name = {                                                               \
		.api = (const struct sys_hashmap_api *)(_api),                                     \
		.config = (const struct sys_hashmap_config *)&_name##_config,                      \
		.data = (struct sys_hashmap_data *)&_name##_data,                                  \
		.hash_func = (_hash_func),                                                         \
		.alloc_func = (_alloc_func),                                                       \
	}

/**
 * @brief Declare a Hashmap statically (advanced)
 *
 * Declare a Hashmap statically with control over advanced parameters.
 *
 * @note The allocator @p _alloc is used for allocating internal Hashmap
 * entries and does not interact with any user-provided keys or values.
 *
 * @param _name Name of the Hashmap.
 * @param _api API pointer of type @ref sys_hashmap_api.
 * @param _config_type Variant of @ref sys_hashmap_config.
 * @param _data_type Variant of @ref sys_hashmap_data.
 * @param _hash_func Hash function pointer of type @ref sys_hash_func32_t.
 * @param _alloc_func Allocator function pointer of type @ref sys_hashmap_allocator_t.
 * @param ... Variant-specific details for @p _config_type.
 */
#define SYS_HASHMAP_DEFINE_STATIC_ADVANCED(_name, _api, _config_type, _data_type, _hash_func,      \
					   _alloc_func, ...)                                       \
	static const struct _config_type _name##_config = __VA_ARGS__;                             \
	static struct _data_type _name##_data;                                                     \
	static struct sys_hashmap _name = {                                                        \
		.api = (const struct sys_hashmap_api *)(_api),                                     \
		.config = (const struct sys_hashmap_config *)&_name##_config,                      \
		.data = (struct sys_hashmap_data *)&_name##_data,                                  \
		.hash_func = (_hash_func),                                                         \
		.alloc_func = (_alloc_func),                                                       \
	}

/**
 * @brief Declare a Hashmap
 *
 * Declare a Hashmap with default parameters.
 *
 * @param _name Name of the Hashmap.
 */
#define SYS_HASHMAP_DEFINE(_name) SYS_HASHMAP_DEFAULT_DEFINE(_name)

/**
 * @brief Declare a Hashmap statically
 *
 * Declare a Hashmap statically with default parameters.
 *
 * @param _name Name of the Hashmap.
 */
#define SYS_HASHMAP_DEFINE_STATIC(_name) SYS_HASHMAP_DEFAULT_DEFINE_STATIC(_name)

/*
 * A safe wrapper for realloc(), invariant of which libc provides it.
 */
static inline void *sys_hashmap_default_allocator(void *ptr, size_t size)
{
	if (size == 0) {
		free(ptr);
		return NULL;
	}

	return realloc(ptr, size);
}

/** @brief The default Hashmap allocator */
#define SYS_HASHMAP_DEFAULT_ALLOCATOR sys_hashmap_default_allocator

/** @brief The default Hashmap load factor (in hundredths) */
#define SYS_HASHMAP_DEFAULT_LOAD_FACTOR 75

/** @brief Generic Hashmap */
struct sys_hashmap {
	/** Hashmap API */
	const struct sys_hashmap_api *api;
	/** Hashmap configuration */
	const struct sys_hashmap_config *config;
	/** Hashmap data */
	struct sys_hashmap_data *data;
	/** Hash function */
	sys_hash_func32_t hash_func;
	/** Allocator */
	sys_hashmap_allocator_t alloc_func;
};

/**
 * @brief Iterate over all values contained in a @ref sys_hashmap
 *
 * @param map Hashmap to iterate over
 * @param cb Callback to call for each entry
 * @param cookie User-specified variable
 */
static inline void sys_hashmap_foreach(const struct sys_hashmap *map, sys_hashmap_callback_t cb,
				       void *cookie)
{
	struct sys_hashmap_iterator it = {0};

	for (map->api->iter(map, &it); sys_hashmap_iterator_has_next(&it);) {
		it.next(&it);
		cb(it.key, it.value, cookie);
	}
}

/**
 * @brief Clear all entries contained in a @ref sys_hashmap
 *
 * @note If the values in a particular Hashmap are
 *
 * @param map Hashmap to clear
 * @param cb Callback to call for each entry
 * @param cookie User-specified variable
 */
static inline void sys_hashmap_clear(struct sys_hashmap *map, sys_hashmap_callback_t cb,
				     void *cookie)
{
	map->api->clear(map, cb, cookie);
}

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
 * @retval -ENOSPC if the size limit has been reached
 */
static inline int sys_hashmap_insert(struct sys_hashmap *map, uint64_t key, uint64_t value,
				     uint64_t *old_value)
{
	return map->api->insert(map, key, value, old_value);
}

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
static inline bool sys_hashmap_remove(struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	return map->api->remove(map, key, value);
}

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
static inline bool sys_hashmap_get(const struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	return map->api->get(map, key, value);
}

/**
 * @brief Check if @p map contains a value associated with @p key
 *
 * @param map Hashmap to search through
 * @param key Key with which to search @p map
 *
 * @retval true if @p map contains a value associated with @p key.
 * @retval false if @p map does not contain a value associated with @p key.
 */
static inline bool sys_hashmap_contains_key(const struct sys_hashmap *map, uint64_t key)
{
	return sys_hashmap_get(map, key, NULL);
}

/**
 * @brief Query the number of entries contained within @p map
 *
 * @param map Hashmap to search through
 *
 * @return the number of entries contained within @p map.
 */
static inline size_t sys_hashmap_size(const struct sys_hashmap *map)
{
	return map->data->size;
}

/**
 * @brief Check if @p map is empty
 *
 * @param map Hashmap to query
 *
 * @retval true if @p map is empty.
 * @retval false if @p map is not empty.
 */
static inline bool sys_hashmap_is_empty(const struct sys_hashmap *map)
{
	return map->data->size == 0;
}

/**
 * @brief Query the load factor of @p map
 *
 * @note To convert the load factor to a floating-point value use
 * `sys_hash_load_factor(map) / 100.0f`.
 *
 * @param map Hashmap to query
 *
 * @return Load factor of @p map expressed in hundredths.
 */
static inline uint8_t sys_hashmap_load_factor(const struct sys_hashmap *map)
{
	if (map->data->n_buckets == 0) {
		return 0;
	}

	return (map->data->size * 100) / map->data->n_buckets;
}

/**
 * @brief Query the number of buckets used in @p map
 *
 * @param map Hashmap to query
 * @return Number of buckets used in @p map
 */
static inline size_t sys_hashmap_num_buckets(const struct sys_hashmap *map)
{
	return map->data->n_buckets;
}

/**
 * @brief Decide whether the Hashmap should be resized
 *
 * This is a simple opportunistic method that implementations
 * can choose to use. It will grow and shrink the Hashmap by a factor
 * of 2 when insertion / removal would exceed / fall into the specified
 * load factor.
 *
 * @note Users should call this prior to inserting a new key-value pair and after removing a
 * key-value pair.
 *
 * @note The number of reserved entries is implementation-defined, but it is only considered
 * as part of the load factor when growing the hash table.
 *
 * @param map Hashmap to examine
 * @param grow true if an entry is to be added. false if an entry has been removed
 * @param num_reserved the number of reserved entries
 * @param[out] new_num_buckets variable Hashmap size
 * @return true if the Hashmap should be rehashed
 * @return false if the Hashmap should not be rehashed
 */
static inline bool sys_hashmap_should_rehash(const struct sys_hashmap *map, bool grow,
					     size_t num_reserved, size_t *new_num_buckets)
{
	size_t size;
	bool should_grow;
	size_t n_buckets;
	bool should_shrink;
	const bool shrink = !grow;
	struct sys_hashmap_oa_lp_data *const data = (struct sys_hashmap_oa_lp_data *)map->data;
	const struct sys_hashmap_config *const config = map->config;

	/* All branchless calculations, so very cache-friendly */

	/* calculate new size */
	size = data->size;
	size += grow;
	/* maximum size imposed by the implementation */
	__ASSERT_NO_MSG(size < SIZE_MAX / 100);

	/* calculate new number of buckets */
	n_buckets = data->n_buckets;
	/* initial number of buckets */
	n_buckets += grow * (size == 1) * config->initial_n_buckets;
	/* grow at a rate of 2x */
	n_buckets <<= grow * (size != 1);
	/* shrink at a rate of 2x */
	n_buckets >>= shrink;

	/* shrink to zero if empty */
	n_buckets *= (size != 0);

	__ASSERT_NO_MSG(new_num_buckets != NULL);
	__ASSERT_NO_MSG(new_num_buckets != &data->n_buckets);
	*new_num_buckets = n_buckets;

	should_grow =
		grow && (data->n_buckets == 0 ||
			 (size + num_reserved) * 100 / data->n_buckets > map->config->load_factor);
	should_shrink =
		shrink && (n_buckets == 0 || (size * 100) / n_buckets <= map->config->load_factor);

	return should_grow || should_shrink;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HASH_MAP_H_ */
