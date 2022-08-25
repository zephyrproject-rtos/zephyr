/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

/* Make sure to match the drive name to ELM FAT Lib volume strings */
#define FATFS_MNTP	"/RAM:"
#define TEST_FILE	FATFS_MNTP"/testfile.txt"
#define TEST_DIR	FATFS_MNTP"/testdir"
#define TEST_DIR_FILE	FATFS_MNTP"/testdir/testfile.txt"

/* Make sure to match the drive name to ELM FAT Lib volume strings */
#define FATFS_MNTP1	"/CF:"
#define TEST_FILE1	FATFS_MNTP1"/testfile.txt"
#define TEST_DIR1	FATFS_MNTP1"/testdir"
#define TEST_DIR_FILE1	FATFS_MNTP1"/testdir/testfile.txt"

extern struct fs_file_t filep;
extern const char test_str[];

int check_file_dir_exists(const char *path);

void test_fat_mount(void);
void test_fat_file(void);
void test_fat_dir(void);
void test_fat_fs(void);
