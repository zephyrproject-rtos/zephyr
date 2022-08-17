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
ZTEST(fat_fs_basic, test_fat_fs)
{
	test_fs_register();
	test_mount();
	test_file_statvfs();
	test_mkdir();
	test_opendir();
	test_closedir();
	test_opendir_closedir();
	test_lsdir();
	test_file_open();
	test_file_write();
	test_file_read();
	test_file_seek();
	test_file_truncate();
	test_file_close();
	test_file_sync();
	test_file_rename();
	test_file_stat();
	test_file_unlink();
	test_unmount();
	test_mount_flags();
}

ZTEST_SUITE(fat_fs_basic, NULL, NULL, NULL, NULL, NULL);
