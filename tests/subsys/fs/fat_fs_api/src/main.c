/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include "test_fat.h"
void test_fs_open_flags(void);
const char *test_fs_open_flags_file_path =  FATFS_MNTP"/the_file.txt";

/* Time integration for filesystem */
DWORD get_fattime(void)
{
	time_t unix_time = time(NULL);
	struct tm *cal;

	/* Convert to calendar time */
	cal = localtime(&unix_time);

	/* From http://elm-chan.org/fsw/ff/doc/fattime.html */
	return (DWORD)(cal->tm_year - 80) << 25 | (DWORD)(cal->tm_mon + 1) << 21 |
	       (DWORD)cal->tm_mday << 16 | (DWORD)cal->tm_hour << 11 | (DWORD)cal->tm_min << 5 |
	       (DWORD)cal->tm_sec >> 1;
}

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
