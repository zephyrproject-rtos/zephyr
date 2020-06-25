/*
 * Copyright (c) 2018 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"

void test_main(void)
{
	ztest_test_suite(posix_fs_test,
		ztest_unit_test(test_fs_mount),
		ztest_unit_test(test_fs_open),
		ztest_unit_test(test_fs_write),
		ztest_unit_test(test_fs_read),
		ztest_unit_test(test_fs_close),
		ztest_unit_test(test_fs_fd_leak),
		ztest_unit_test(test_fs_unlink),
		ztest_unit_test(test_fs_mkdir),
		ztest_unit_test(test_fs_readdir),
		ztest_unit_test(test_fs_open_flags));
	ztest_run_test_suite(posix_fs_test);
}
