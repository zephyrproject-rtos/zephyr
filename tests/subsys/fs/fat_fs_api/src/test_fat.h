/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <fs.h>

#define TEST_FILE "testfile.txt"
#define TEST_DIR "testdir"
fs_file_t filep;
extern const char test_str[];

int check_file_dir_exists(const char *path);

void test_fat_file(void);
void test_fat_dir(void);
void test_fat_fs(void);

