/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nffs/nffs.h>
#include "test_nffs.h"

void test_main(void)
{
	ztest_test_suite(nffs_fs_performance_test,
		ztest_unit_test_setup_teardown(test_fs_mount,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_performance,
					       test_setup, test_teardown));
	ztest_run_test_suite(nffs_fs_performance_test);
}
