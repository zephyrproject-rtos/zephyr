/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_LIB_HASH_MAP_SRC_MAIN_H_
#define ZEPHYR_TESTS_LIB_HASH_MAP_SRC_MAIN_H_

#include <zephyr/sys/hash_map.h>

#define MANY               CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES
#define CUSTOM_LOAD_FACTOR 42

extern struct sys_hashmap map;
extern struct sys_hashmap custom_load_factor_map;

uint32_t hash_string(const void *uint64_ptr_key, size_t len);
bool eq_string(uint64_t key_left, uint64_t key_right);
char *get_string_index(uint64_t key);

#ifdef CONFIG_TEST_USE_CUSTOM_EQ_FUNC
#include "string_map_helper.h"
#define KEY_TYPE char *
#define ALLOC_KEY(key) (alloc_string_index(key))
#define FREE_KEY(key) (free(key))
#define GET_KEY(key) (get_string_index(key))
#define KEY_FORMAT "%s"
#define GET_FUNC_NAME string_map_get
#define INSERT_FUNC_NAME string_map_insert
#define CONTAINS_KEY_FUNC_NAME string_map_contains_key
#define REMOVE_FUNC_NAME string_map_remove
#define FOREACH_FUNC_NAME string_map_foreach
#define CLEAR_FUNC_NAME string_map_clear
#else
#define KEY_TYPE uint64_t
#define ALLOC_KEY(key) (key)
#define FREE_KEY(key) ((void) key)
#define GET_KEY(key) (key)
#define KEY_FORMAT "%zu"
#define GET_FUNC_NAME sys_hashmap_get
#define INSERT_FUNC_NAME sys_hashmap_insert
#define CONTAINS_KEY_FUNC_NAME sys_hashmap_contains_key
#define REMOVE_FUNC_NAME sys_hashmap_remove
#define FOREACH_FUNC_NAME sys_hashmap_foreach
#define CLEAR_FUNC_NAME sys_hashmap_clear
#endif

#define GET_FUNC(map, key, value) (GET_FUNC_NAME(map, GET_KEY(key), value))
#define INSERT_FUNC(map, key, value, old_value, old_key)                                           \
	(INSERT_FUNC_NAME(map, GET_KEY(key), value, old_value, old_key))
#define CONTAINS_KEY_FUNC(map, key) (CONTAINS_KEY_FUNC_NAME(map, GET_KEY(key)))
#define REMOVE_FUNC(map, key, old_value, old_key)                                                  \
	(REMOVE_FUNC_NAME(map, GET_KEY(key), old_value, old_key))

#endif
