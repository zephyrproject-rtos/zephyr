/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_TESTS_H_
#define  _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_TESTS_H_

/* Tests in test_util */
void test_util_path_init_base(void);
void test_util_path_init_overrun(void);
void test_util_path_init_extended(void);
void test_util_path_extend(void);
void test_util_path_extend_up(void);
void test_util_path_extend_overrun(void);

/* Tests in test_lfs_basic */
void test_lfs_basic(void);

/* Tests in test_lfs_dirops */
void test_lfs_dirops(void);

/* Tests in test_lfs_perf */
void test_lfs_perf(void);

#endif /* _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_TESTS_H_ */
