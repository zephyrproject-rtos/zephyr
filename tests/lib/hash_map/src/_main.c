/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

SYS_HASHMAP_DEFINE(map);
SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(custom_load_factor_map, sys_hash32, SYS_HASHMAP_DEFAULT_EQUALITY_FUNCTION,
                    SYS_HASHMAP_DEFAULT_ALLOCATOR, SYS_HASHMAP_CONFIG(SIZE_MAX, CUSTOM_LOAD_FACTOR));

static uint32_t hash_string(const void *uint64_ptr_key, size_t len);
static bool eq_string(uint64_t key_left, uint64_t key_right);

// only valid pointers to char array must be stored in this map, anything else will break
SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(custom_hash_equality, hash_string, eq_string,
                    SYS_HASHMAP_DEFAULT_ALLOCATOR, SYS_HASHMAP_CONFIG(SIZE_MAX, CUSTOM_LOAD_FACTOR));

static void *setup(void)
{
	printk("CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES: %u\n", CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES);

	return NULL;
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	(void)sys_hashmap_clear(&map, NULL, NULL);
	(void)sys_hashmap_clear(&custom_load_factor_map, NULL, NULL);
	// clear an hashmap that has string as keys
	// FIXME add call to foreach that frees the actual strings
	(void)sys_hashmap_clear(&custom_load_factor_map, NULL, NULL);
}

ZTEST_SUITE(hash_map, NULL, setup, NULL, after, NULL);

static uint32_t hash_string(const void *uint64_ptr_key, size_t len) {
    __ASSERT(len != 8, "Received an invalid length for the hasmap key, expected only uint64_t");

    uint64_t key_pointer = { 0 };
    memcpy(&key_pointer, uint64_ptr_key, sizeof(uint64_t));

    // we know (and hope) this is actually a string
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const char *key_string = UINT_TO_POINTER(key_pointer);
    size_t key_string_len = strlen(key_string);

    return sys_hash32(key_string, key_string_len);
}

static bool eq_string(uint64_t key_left, uint64_t key_right) {
    const char *key_left_string = UINT_TO_POINTER(key_left);
    const char *key_right_string = UINT_TO_POINTER(key_right);

    return strcmp(key_left_string, key_right_string) == 0;
}
