/*
 * Copyright (c) 2018 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_common.h"
#include "test_fat.h"
#include "test_littlefs.h"
#include "test_fs_shell.h"

ZTEST(multi_fs_fat_dir, test_multi_fs_fat)
{
	/*
	 * fat dir operation.
	 * These tests are order-dependent.
	 * They have to be executed in order.
	 */
	test_fat_mkdir();
	test_fat_readdir();
	test_fat_rmdir();
}
ZTEST(multi_fs_fat_file, test_multi_fs_fat)
{
	/*
	 * fat file operation.
	 * These tests are order-dependent.
	 * They have to be executed in order.
	 */
	test_fat_open();
	test_fat_write();
	test_fat_read();
	test_fat_close();
	test_fat_unlink();
}
ZTEST(multi_fs_littlefs_dir, test_multi_fs_littlefs)
{
	/*
	 * littlefs dir operation.
	 * These tests are order-dependent.
	 * They have to be executed in order.
	 */
	test_littlefs_mkdir();
	test_littlefs_readdir();
	test_littlefs_rmdir();
}
ZTEST(multi_fs_littlefs_file, test_multi_fs_littlefs)
{
	/*
	 * littlefs file operation.
	 * These tests are order-dependent.
	 * They have to be executed in order.
	 */
	test_littlefs_open();
	test_littlefs_write();
	test_littlefs_read();
	test_littlefs_close();
	test_littlefs_unlink();
}
static void *multi_fs_fat_setup(void)
{
	test_clear_flash();
	test_fat_mount();
	return NULL;
}
static void *multi_fs_littlefs_setup(void)
{
	test_clear_flash();
	test_littlefs_mount();
	return NULL;
}
ZTEST_SUITE(multi_fs_fat_dir, NULL, multi_fs_fat_setup, NULL, NULL, NULL);
ZTEST_SUITE(multi_fs_fat_file, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(multi_fs_littlefs_dir, NULL, multi_fs_littlefs_setup, NULL, NULL, NULL);
ZTEST_SUITE(multi_fs_littlefs_file, NULL, NULL, NULL, NULL, NULL);
