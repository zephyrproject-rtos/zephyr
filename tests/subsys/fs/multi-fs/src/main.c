/*
 * Copyright (c) 2018 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_common.h"
#include "test_fat.h"
#include "test_littlefs.h"
#include "test_fs_shell.h"

void test_main(void)
{
	ztest_test_suite(multifs_fs_test,
			 ztest_unit_test(test_clear_flash),
			 ztest_unit_test(test_fat_mount),
			 ztest_unit_test(test_littlefs_mount),
			 ztest_unit_test(test_fat_mkdir),
			 ztest_unit_test(test_littlefs_mkdir),
			 ztest_unit_test(test_fat_readdir),
			 ztest_unit_test(test_littlefs_readdir),
			 ztest_unit_test(test_fat_rmdir),
			 ztest_unit_test(test_littlefs_rmdir),
			 ztest_unit_test(test_fat_open),
			 ztest_unit_test(test_littlefs_open),
			 ztest_unit_test(test_fat_write),
			 ztest_unit_test(test_littlefs_write),
			 ztest_unit_test(test_fat_read),
			 ztest_unit_test(test_littlefs_read),
			 ztest_unit_test(test_fat_close),
			 ztest_unit_test(test_littlefs_close),
			 ztest_unit_test(test_fat_unlink),
			 ztest_unit_test(test_littlefs_unlink),
			 ztest_unit_test(test_fs_help));
	ztest_run_test_suite(multifs_fs_test);
}
