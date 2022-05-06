/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_FS_H__
#define __TEST_FS_H__

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>

#define TEST_FS_MNTP	"/NAND:"
#define TEST_FILE	TEST_FS_MNTP"/testfile.txt"
#define TEST_FILE_RN	TEST_FS_MNTP"/testfile_renamed.txt"
#define TEST_FILE_EX	TEST_FS_MNTP"/testfile_exist.txt"
#define TEST_DIR	TEST_FS_MNTP"/testdir"
#define TEST_DIR_FILE	TEST_FS_MNTP"/testdir/testfile.txt"

/* kenel only reserve two slots for specific file system.
 * By disable that two file systems, test cases can make
 * use of that slots to register a file systems for test
 */
#define TEST_FS_1 FS_FATFS
#define TEST_FS_2 FS_LITTLEFS

extern struct fs_file_system_t temp_fs;

struct test_fs_data {
	int reserve;
};

void mock_opendir_result(int ret);

void test_opendir_closedir(void);
void test_fs_dir_t_init(void);
void test_fs_file_t_init(void);
void test_fs_register(void);
void test_mount(void);
void test_file_statvfs(void);
void test_mkdir(void);
void test_opendir(void);
void test_closedir(void);
void test_lsdir(void);
void test_file_open(void);
void test_file_write(void);
void test_file_read(void);
void test_file_seek(void);
void test_file_truncate(void);
void test_file_close(void);
void test_file_sync(void);
void test_file_rename(void);
void test_file_stat(void);
void test_file_unlink(void);
void test_unmount(void);
void test_mount_flags(void);
#endif
