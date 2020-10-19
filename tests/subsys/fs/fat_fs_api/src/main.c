/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_fat.h"
void test_fs_open_flags(void);
const char *test_fs_open_flags_file_path =  FATFS_MNTP"/the_file.txt";

void test_main(void)
{
	ztest_test_suite(fat_fs_basic_test,
			 ztest_unit_test(test_fat_mount),
			 ztest_unit_test(test_fat_file),
			 ztest_unit_test(test_fat_dir),
			 ztest_unit_test(test_fat_fs),
			 ztest_unit_test(test_fat_rename),
			 ztest_unit_test(test_fs_open_flags),
			 ztest_unit_test(test_fat_unmount),
			 ztest_unit_test(test_fat_mount_rd_only));
	ztest_run_test_suite(fat_fs_basic_test);
}
