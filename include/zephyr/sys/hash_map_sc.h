/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Separate Chaining Hashmap Implementation
 *
 * @note Enable with @kconfig{CONFIG_SYS_HASH_MAP_SC}
 */

#ifndef ZEPHYR_INCLUDE_SYS_HASH_MAP_SC_H_
#define ZEPHYR_INCLUDE_SYS_HASH_MAP_SC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/sys/hash_function.h>
#include <zephyr/sys/hash_map_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Declare a Separate Chaining Hashmap (advanced)
 *
 * Declare a Separate Chaining Hashmap with control over advanced parameters.
 *
 * @note The allocator @p _alloc_func is used for allocating internal Hashmap
 * entries and does not interact with any user-provided keys or values.
 *
 * @param _name Name of the Hashmap.
 * @param _hash_func Hash function pointer of type @ref sys_hash_func32_t.
 * @param _alloc_func Allocator function pointer of type @ref sys_hashmap_allocator_t.
 * @param ... Details for @ref sys_hashmap_config.
 */
#define SYS_HASHMAP_SC_DEFINE_ADVANCED(_name, _hash_func, _alloc_func, ...)                        \
	SYS_HASHMAP_DEFINE_ADVANCED(_name, &sys_hashmap_sc_api, sys_hashmap_config,                \
				    sys_hashmap_data, _hash_func, _alloc_func, __VA_ARGS__)

/**
 * @brief Declare a Separate Chaining Hashmap (advanced)
 *
 * Declare a Separate Chaining Hashmap with control over advanced parameters.
 *
 * @note The allocator @p _alloc is used for allocating internal Hashmap
 * entries and does not interact with any user-provided keys or values.
 *
 * @param _name Name of the Hashmap.
 * @param _hash_func Hash function pointer of type @ref sys_hash_func32_t.
 * @param _alloc_func Allocator function pointer of type @ref sys_hashmap_allocator_t.
 * @param ... Details for @ref sys_hashmap_config.
 */
#define SYS_HASHMAP_SC_DEFINE_STATIC_ADVANCED(_name, _hash_func, _alloc_func, ...)                 \
	SYS_HASHMAP_DEFINE_STATIC_ADVANCED(_name, &sys_hashmap_sc_api, sys_hashmap_config,         \
					   sys_hashmap_data, _hash_func, _alloc_func, __VA_ARGS__)

/**
 * @brief Declare a Separate Chaining Hashmap statically
 *
 * Declare a Separate Chaining Hashmap statically with default parameters.
 *
 * @param _name Name of the Hashmap.
 */
#define SYS_HASHMAP_SC_DEFINE_STATIC(_name)                                                        \
	SYS_HASHMAP_SC_DEFINE_STATIC_ADVANCED(                                                     \
		_name, sys_hash32, SYS_HASHMAP_DEFAULT_ALLOCATOR,                                  \
		SYS_HASHMAP_CONFIG(SIZE_MAX, SYS_HASHMAP_DEFAULT_LOAD_FACTOR))

/**
 * @brief Declare a Separate Chaining Hashmap
 *
 * Declare a Separate Chaining Hashmap with default parameters.
 *
 * @param _name Name of the Hashmap.
 */
#define SYS_HASHMAP_SC_DEFINE(_name)                                                               \
	SYS_HASHMAP_SC_DEFINE_ADVANCED(                                                            \
		_name, sys_hash32, SYS_HASHMAP_DEFAULT_ALLOCATOR,                                  \
		SYS_HASHMAP_CONFIG(SIZE_MAX, SYS_HASHMAP_DEFAULT_LOAD_FACTOR))

#ifdef CONFIG_SYS_HASH_MAP_CHOICE_SC
#define SYS_HASHMAP_DEFAULT_DEFINE(_name)	 SYS_HASHMAP_SC_DEFINE(_name)
#define SYS_HASHMAP_DEFAULT_DEFINE_STATIC(_name) SYS_HASHMAP_SC_DEFINE_STATIC(_name)
#define SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(_name, _hash_func, _alloc_func, ...)                   \
	SYS_HASHMAP_SC_DEFINE_ADVANCED(_name, _hash_func, _alloc_func, __VA_ARGS__)
#define SYS_HASHMAP_DEFAULT_DEFINE_STATIC_ADVANCED(_name, _hash_func, _alloc_func, ...)            \
	SYS_HASHMAP_SC_DEFINE_STATIC_ADVANCED(_name, _hash_func, _alloc_func, __VA_ARGS__)
#endif

extern const struct sys_hashmap_api sys_hashmap_sc_api;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HASH_MAP_SC_H_ */
