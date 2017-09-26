/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <fs.h>

void test_mkdir(void);
void test_gc_on_oom(void);
void test_incomplete_block(void);
void test_lost_found(void);
void test_readdir(void);
void test_large_system(void);
void test_corrupt_block(void);
void test_split_file(void);
void test_large_unlink(void);
void test_corrupt_scratch(void);
void test_unlink(void);
void test_append(void);
void test_rename(void);
void test_truncate(void);
void test_open(void);
void test_wear_level(void);
void test_long_filename(void);
void test_overwrite_one(void);
void test_many_children(void);
void test_gc(void);
void test_overwrite_many(void);
void test_overwrite_two(void);
void test_overwrite_three(void);
void test_read(void);
void test_cache_large_file(void);
void test_large_write(void);
void test_performance(void);
