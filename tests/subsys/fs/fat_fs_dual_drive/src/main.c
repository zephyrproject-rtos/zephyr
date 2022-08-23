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

static void *fat_fs_dual_drive_setup(void)
{
	fs_file_t_init(&filep);

	test_fat_mount();
	return NULL;
}
ZTEST_SUITE(fat_fs_dual_drive, NULL, fat_fs_dual_drive_setup, NULL, NULL, NULL);
