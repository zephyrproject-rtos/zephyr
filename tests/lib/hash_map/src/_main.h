/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_LIB_HASH_MAP_SRC_MAIN_H_
#define ZEPHYR_TESTS_LIB_HASH_MAP_SRC_MAIN_H_

#include <zephyr/sys/hash_map.h>

#define MANY		   CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES
#define CUSTOM_LOAD_FACTOR 42

extern struct sys_hashmap map;
extern struct sys_hashmap custom_load_factor_map;

#endif
