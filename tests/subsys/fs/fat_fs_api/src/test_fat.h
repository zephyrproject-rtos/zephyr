/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <fs/fs.h>

#define FATFS_MNTP	"/NAND:"
#define TEST_FILE	FATFS_MNTP"/testfile.txt"
#define TEST_DIR	FATFS_MNTP"/testdir"
#define TEST_DIR_FILE	FATFS_MNTP"/testdir/testfile.txt"

extern struct fs_file_t filep;
extern const char test_str[];

int check_file_dir_exists(const char *path);

void test_fat_mount(void);
void test_fat_file(void);
void test_fat_dir(void);
void test_fat_fs(void);
void test_fat_rename(void);
