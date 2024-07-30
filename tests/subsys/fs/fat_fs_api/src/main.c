/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_fat.h"
void test_fs_open_flags(void);
const char *test_fs_open_flags_file_path =  FATFS_MNTP"/the_file.txt";

static void *fat_fs_basic_setup(void)
{
	fs_file_t_init(&filep);
	test_fat_mount();
	test_fat_file();
	test_fat_dir();
	test_fat_fs();
	test_fat_rename();
	test_fs_open_flags();
#ifdef CONFIG_FS_FATFS_REENTRANT
	test_fat_file_reentrant();
#endif /* CONFIG_FS_FATFS_REENTRANT */
	test_fat_unmount();

	return NULL;
}
ZTEST_SUITE(fat_fs_basic, NULL, fat_fs_basic_setup, NULL, NULL, NULL);
