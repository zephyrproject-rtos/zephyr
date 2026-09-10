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
SYS_HASHMAP_DEFAULT_DEFINE_ADVANCED(custom_load_factor_map, sys_hash32, realloc,
				    SYS_HASHMAP_CONFIG(SIZE_MAX, CUSTOM_LOAD_FACTOR));

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
}

ZTEST_SUITE(hash_map, NULL, setup, NULL, after, NULL);
