/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"

/**
 * @brief Test file system interface implemented in kernel
 *
 * @defgroup filesystem File System
 *
 * @ingroup all_test
 * @{
 * @}
 */

static void fs_setup(void)
{
	fs_register(TEST_FS_1, &temp_fs);
}

static void dummy_teardown(void)
{
	return;
}

static void dummy_setup(void)
{
	return;
}

static void fs_teardown(void)
{
	fs_unregister(TEST_FS_1, &temp_fs);
}

/**
 * @brief Common file system operations through a general interface
 *
 * @details After register file system:
 *            - mount
 *            - statvfs
 *            - mkdir
 *            - opendir
 *            - readdir
 *            - closedir
 *            - open
 *            - write
 *            - read
 *            - lseek
 *            - tell
 *            - truncate
 *            - sync
 *            - close
 *            - rename
 *            - stat
 *            - unlink
 *            - unmount
 *            - unregister file system
 *          the order of test cases is critical, one case depend on the
 *          case before it.
 *
 * @ingroup filesystem
 *
 */
void test_main(void)
{
	ztest_test_suite(fat_fs_basic_test,
			 ztest_unit_test(test_fs_register),
			 ztest_unit_test_setup_teardown(test_mount,
							fs_setup,
							dummy_teardown),
			 ztest_unit_test(test_fs_file_t_init),
			 ztest_unit_test(test_fs_dir_t_init),
			 ztest_unit_test(test_file_statvfs),
			 ztest_unit_test(test_mkdir),
			 ztest_unit_test(test_opendir),
			 ztest_unit_test(test_closedir),
			 ztest_unit_test(test_opendir_closedir),
			 ztest_unit_test(test_lsdir),
			 ztest_unit_test(test_file_open),
			 ztest_unit_test(test_file_write),
			 ztest_unit_test(test_file_read),
			 ztest_unit_test(test_file_seek),
			 ztest_unit_test(test_file_truncate),
			 ztest_unit_test(test_file_close),
			 ztest_unit_test(test_file_sync),
			 ztest_unit_test(test_file_rename),
			 ztest_unit_test(test_file_stat),
			 ztest_unit_test(test_file_unlink),
			 ztest_unit_test(test_unmount),
			 ztest_unit_test_setup_teardown(test_mount_flags,
							dummy_setup,
							fs_teardown)
			 );
	ztest_run_test_suite(fat_fs_basic_test);
}
