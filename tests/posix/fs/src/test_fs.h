/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define FATFS_MNTP	"/RAM:"
#define TEST_FILE	FATFS_MNTP"/testfile.txt"
#define TEST_DIR	FATFS_MNTP"/testdir"
#define TEST_DIR_FILE	FATFS_MNTP"/testdir/testfile.txt"

extern const char test_str[];

void test_fs_mount(void);
void test_fs_open(void);
void test_fs_open_flags(void);
void test_fs_write(void);
void test_fs_read(void);
void test_fs_close(void);
void test_fs_fd_leak(void);
void test_fs_unlink(void);
void test_fs_mkdir(void);
void test_fs_readdir(void);
void test_fs_rmdir(void);
