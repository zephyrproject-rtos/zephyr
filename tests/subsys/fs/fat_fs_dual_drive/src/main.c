/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_fat_fs
 * @{
 * @defgroup t_fat_fs_basic test_fat_fs_basic_operations
 * @}
 */

#include "test_fat.h"

void test_main(void)
{
	fs_file_t_init(&filep);

	ztest_test_suite(fat_fs_basic_test,
			 ztest_unit_test(test_fat_mount),
			 ztest_unit_test(test_fat_file),
			 ztest_unit_test(test_fat_dir),
			 ztest_unit_test(test_fat_fs));
	ztest_run_test_suite(fat_fs_basic_test);
}
