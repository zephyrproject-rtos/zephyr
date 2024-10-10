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

bool string_map_contains_key(const struct sys_hashmap *map, const char *key);
char *alloc_string_from_index(size_t i);
bool string_map_remove(struct sys_hashmap *map, const char *key, uint64_t *value, char **old_key);
int string_map_insert(struct sys_hashmap *map, const char *key, uint64_t value, uint64_t *old_value,
		      char **old_key);
bool string_map_get(const struct sys_hashmap *map, const char *key, uint64_t *value);

void string_map_free_callback(uint64_t key, uint64_t value, void *cookie);

#endif
