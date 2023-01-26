/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#define FATFS_MNTP	"/NAND:"
#if defined(CONFIG_FS_FATFS_LFN)
#define TEST_FILE	FATFS_MNTP \
	"/testlongfilenamethatsmuchlongerthan8.3chars.text"
#else
#define TEST_FILE	FATFS_MNTP"/testfile.txt"
#endif /* IS_ENABLED(CONFIG_FS_FATFS_LFN) */
#define TEST_DIR	FATFS_MNTP"/testdir"
#define TEST_DIR_FILE	FATFS_MNTP"/testdir/testfile.txt"

extern struct fs_file_t filep;
extern const char test_str[];

int check_file_dir_exists(const char *path);

void test_fat_mount(void);
void test_fat_unmount(void);
void test_fat_file(void);
void test_fat_dir(void);
void test_fat_fs(void);
void test_fat_rename(void);
