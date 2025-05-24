/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

#include <stdint.h>

#include <zephyr/sys/hash_map.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_TEST_USE_CUSTOM_EQ_FUNC
/* only valid pointers to char array must be stored in these maps, anything else will break */
#define EQ_FUNC eq_string
#define HASH_FUNC hash_string
#else
#define EQ_FUNC SYS_HASHMAP_DEFAULT_EQUALITY_FUNCTION
#define HASH_FUNC sys_hash32
#endif

SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(map, HASH_FUNC, EQ_FUNC,
				    SYS_HASHMAP_DEFAULT_ALLOCATOR,
				    SYS_HASHMAP_CONFIG(SIZE_MAX, SYS_HASHMAP_DEFAULT_LOAD_FACTOR));
SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(custom_load_factor_map, HASH_FUNC,
				    EQ_FUNC,
				    SYS_HASHMAP_DEFAULT_ALLOCATOR,
				    SYS_HASHMAP_CONFIG(SIZE_MAX, CUSTOM_LOAD_FACTOR));

static char **indices;

char *get_string_index(uint64_t key)
{
	return indices[key];
}

static void *setup(void)
{
#if defined(CONFIG_TEST_USE_CUSTOM_EQ_FUNC)
	printk("Running using custom equality function (CONFIG_TEST_USE_CUSTOM_EQ_FUNC set)\n");
	indices = calloc(MANY, sizeof(char *));
	for (uint64_t i = 0; i < MANY; ++i) {
		indices[i] = alloc_string_index(i);
	}
#endif

	printk("CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES: %u\n", CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES);

	return NULL;
}

static void teardown(void *arg)
{
#if defined(CONFIG_TEST_USE_CUSTOM_EQ_FUNC)
	for (size_t i = 0; i < MANY; ++i) {
		free(indices[i]);
	}
	free(indices);
#endif
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	(void) sys_hashmap_clear(&map, NULL, NULL);
	(void) sys_hashmap_clear(&custom_load_factor_map, NULL, NULL);
}

ZTEST_SUITE(hash_map, NULL, setup, NULL, after, teardown);
