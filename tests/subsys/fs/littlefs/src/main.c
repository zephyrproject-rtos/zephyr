/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests of functions in testfs_util */

#include <string.h>
#include <ztest.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"

void test_main(void)
{
	ztest_test_suite(littlefs_test,
			 ztest_unit_test(test_util_path_init_base),
			 ztest_unit_test(test_util_path_init_overrun),
			 ztest_unit_test(test_util_path_init_extended),
			 ztest_unit_test(test_util_path_extend),
			 ztest_unit_test(test_util_path_extend_up),
			 ztest_unit_test(test_util_path_extend_overrun),
			 ztest_unit_test(test_lfs_basic),
			 ztest_unit_test(test_lfs_dirops),
			 ztest_unit_test(test_lfs_perf),
			 ztest_unit_test(test_fs_open_flags_lfs)
			 );
	ztest_run_test_suite(littlefs_test);
}
