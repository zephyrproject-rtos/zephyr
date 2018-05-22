/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nffs/nffs.h>
#include "test_nffs.h"

void test_main(void)
{
	ztest_test_suite(nffs_fs_basic_test,
		ztest_unit_test_setup_teardown(test_fs_mount,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_unlink,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_mkdir,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_rename,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_append,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_read,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_open,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_one,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_two,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_three,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_many,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_long_filename,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_large_write,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_many_children,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_gc,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_wear_level,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_corrupt_scratch,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_incomplete_block,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_corrupt_block,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_lost_found,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_readdir,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_split_file,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_gc_on_oom,
					       test_setup, test_teardown));
	ztest_run_test_suite(nffs_fs_basic_test);
}
