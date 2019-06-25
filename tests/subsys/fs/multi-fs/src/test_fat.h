/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <fs/fs.h>

#define FATFS_MNTP	"/RAM:"
#define TEST_FILE	FATFS_MNTP"/testfile.txt"
#define TEST_DIR	FATFS_MNTP"/testdir"
#define TEST_DIR_FILE	FATFS_MNTP"/testdir/testfile.txt"

extern struct fs_file_t filep;
extern const char test_str[];

int check_file_dir_exists(const char *path);

void test_fat_mount(void);
void test_fat_open(void);
void test_fat_write(void);
void test_fat_read(void);
void test_fat_close(void);
void test_fat_unlink(void);
void test_fat_mkdir(void);
void test_fat_readdir(void);
void test_fat_rmdir(void);

