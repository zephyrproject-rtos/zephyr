/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"
#include "zephyr/sys/printk.h"
#include "zephyr/sys/util.h"

static uint32_t hash_string(const void *uint64_ptr_key, size_t len);
static bool eq_string(uint64_t key_left, uint64_t key_right);

/* only valid pointers to char array must be stored in this map, anything else will break */
SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(map, hash_string, eq_string, SYS_HASHMAP_DEFAULT_ALLOCATOR,
				    SYS_HASHMAP_CONFIG(SIZE_MAX, CUSTOM_LOAD_FACTOR));

bool string_map_contains_key(const struct sys_hashmap *map, const char *key)
{
	return string_map_get(map, key, NULL);
}

bool string_map_remove(struct sys_hashmap *map, const char *key, uint64_t *value, char **old_key)
{
	uint64_t uint_old_key = 0;
	bool res = 0;

	res = sys_hashmap_remove(map, POINTER_TO_UINT(key), value, &uint_old_key);

	if (old_key && uint_old_key) {
		/* NOLINTNEXTLINE(performance-no-int-to-ptr) */
		*old_key = UINT_TO_POINTER(uint_old_key);
	}

	return res;
}

int string_map_insert(struct sys_hashmap *map, const char *key, uint64_t value, uint64_t *old_value,
		      char **old_key)
{
	uint64_t uint_old_key = 0;
	int res = 0;

	res = sys_hashmap_insert(map, POINTER_TO_UINT(key), POINTER_TO_UINT(value), old_value,
				 &uint_old_key);

	if (old_key && uint_old_key) {
		/* NOLINTNEXTLINE(performance-no-int-to-ptr) */
		*old_key = UINT_TO_POINTER(uint_old_key);
	}

	return res;
}

bool string_map_get(const struct sys_hashmap *map, const char *key, uint64_t *value)
{
	return sys_hashmap_get(map, POINTER_TO_UINT(key), value);
}

void string_map_free_callback(uint64_t key, uint64_t value, void *cookie)
{
	/* NOLINTNEXTLINE(performance-no-int-to-ptr) */
	char *key_string = UINT_TO_POINTER(key);

	free(key_string);
}

/* could be improved currently 26 should become aa */
char *alloc_string_from_index(size_t index)
{
	const char first = 'a';
	const char last = 'z' + 1;
	const size_t chars = (last - first);
	const size_t string_len = (index / chars) + 2;
	char *result = calloc(string_len, sizeof(char));

	for (size_t i = 0; i < index; ++i) {
		result[i / chars] = first + (i % chars);
	}

	return result;
}

static void *setup(void)
{
	printk("CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES: %u\n", CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES);

	return NULL;
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	(void)sys_hashmap_clear(&map, NULL, NULL);
}

ZTEST_SUITE(hash_map_string, NULL, setup, NULL, after, NULL);

static uint32_t hash_string(const void *uint64_ptr_key, size_t len)
{
	__ASSERT(len == sizeof(uint64_t),
		 "Received an invalid length for the hasmap key, expected only uint64_t");

	uint64_t key_pointer = {0};

	memcpy(&key_pointer, uint64_ptr_key, sizeof(uint64_t));

	const char *key_string = UINT_TO_POINTER(key_pointer);
	size_t key_string_len = strlen(key_string);

	return sys_hash32(key_string, key_string_len);
}

static bool eq_string(uint64_t key_left, uint64_t key_right)
{
	const char *key_left_string = UINT_TO_POINTER(key_left);
	const char *key_right_string = UINT_TO_POINTER(key_right);

	return strcmp(key_left_string, key_right_string) == 0;
}
