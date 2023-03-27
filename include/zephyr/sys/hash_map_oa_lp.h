/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Open-Addressing / Linear Probe Hashmap Implementation
 *
 * @note Enable with @kconfig{CONFIG_SYS_HASH_MAP_OA_LP}
 */

#ifndef ZEPHYR_INCLUDE_SYS_HASH_MAP_OA_LP_H_
#define ZEPHYR_INCLUDE_SYS_HASH_MAP_OA_LP_H_

#include <stddef.h>

#include <zephyr/sys/hash_function.h>
#include <zephyr/sys/hash_map_api.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sys_hashmap_oa_lp_data {
	void *buckets;
	size_t n_buckets;
	size_t size;
	size_t n_tombstones;
};

/**
 * @brief Declare a Open Addressing Linear Probe Hashmap (advanced)
 *
 * Declare a Open Addressing Linear Probe Hashmap with control over advanced parameters.
 *
 * @note The allocator @p _alloc is used for allocating internal Hashmap
 * entries and does not interact with any user-provided keys or values.
 *
 * @param _name Name of the Hashmap.
 * @param _hash_func Hash function pointer of type @ref sys_hash_func32_t.
 * @param _alloc_func Allocator function pointer of type @ref sys_hashmap_allocator_t.
 * @param ... Variant-specific details for @ref sys_hashmap_config.
 */
#define SYS_HASHMAP_OA_LP_DEFINE_ADVANCED(_name, _hash_func, _alloc_func, ...)                     \
	SYS_HASHMAP_DEFINE_ADVANCED(_name, &sys_hashmap_oa_lp_api, sys_hashmap_config,             \
				    sys_hashmap_oa_lp_data, _hash_func, _alloc_func, __VA_ARGS__)

/**
 * @brief Declare a Open Addressing Linear Probe Hashmap (advanced)
 *
 * Declare a Open Addressing Linear Probe Hashmap with control over advanced parameters.
 *
 * @note The allocator @p _alloc is used for allocating internal Hashmap
 * entries and does not interact with any user-provided keys or values.
 *
 * @param _name Name of the Hashmap.
 * @param _hash_func Hash function pointer of type @ref sys_hash_func32_t.
 * @param _alloc_func Allocator function pointer of type @ref sys_hashmap_allocator_t.
 * @param ... Details for @ref sys_hashmap_config.
 */
#define SYS_HASHMAP_OA_LP_DEFINE_STATIC_ADVANCED(_name, _hash_func, _alloc_func, ...)              \
	SYS_HASHMAP_DEFINE_STATIC_ADVANCED(_name, &sys_hashmap_oa_lp_api, sys_hashmap_config,      \
					   sys_hashmap_oa_lp_data, _hash_func, _alloc_func,        \
					   __VA_ARGS__)

/**
 * @brief Declare a Open Addressing Linear Probe Hashmap statically
 *
 * Declare a Open Addressing Linear Probe Hashmap statically with default parameters.
 *
 * @param _name Name of the Hashmap.
 */
#define SYS_HASHMAP_OA_LP_DEFINE_STATIC(_name)                                                     \
	SYS_HASHMAP_OA_LP_DEFINE_STATIC_ADVANCED(                                                  \
		_name, sys_hash32, SYS_HASHMAP_DEFAULT_ALLOCATOR,                                  \
		SYS_HASHMAP_CONFIG(SIZE_MAX, SYS_HASHMAP_DEFAULT_LOAD_FACTOR))

/**
 * @brief Declare a Open Addressing Linear Probe Hashmap
 *
 * Declare a Open Addressing Linear Probe Hashmap with default parameters.
 *
 * @param _name Name of the Hashmap.
 */
#define SYS_HASHMAP_OA_LP_DEFINE(_name)                                                            \
	SYS_HASHMAP_OA_LP_DEFINE_ADVANCED(                                                         \
		_name, sys_hash32, SYS_HASHMAP_DEFAULT_ALLOCATOR,                                  \
		SYS_HASHMAP_CONFIG(SIZE_MAX, SYS_HASHMAP_DEFAULT_LOAD_FACTOR))

#ifdef CONFIG_SYS_HASH_MAP_CHOICE_OA_LP
#define SYS_HASHMAP_DEFAULT_DEFINE(_name)	 SYS_HASHMAP_OA_LP_DEFINE(_name)
#define SYS_HASHMAP_DEFAULT_DEFINE_STATIC(_name) SYS_HASHMAP_OA_LP_DEFINE_STATIC(_name)
#define SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(_name, _hash_func, _alloc_func, ...)                   \
	SYS_HASHMAP_OA_LP_DEFINE_ADVANCED(_name, _hash_func, _alloc_func, __VA_ARGS__)
#define SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(_name, _hash_func, _alloc_func, ...)            \
	SYS_HASHMAP_OA_LP_DEFINE_STATIC_ADVANCED(_name, _hash_func, _alloc_func, __VA_ARGS__)
#endif

extern const struct sys_hashmap_api sys_hashmap_oa_lp_api;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HASH_MAP_OA_LP_H_ */
