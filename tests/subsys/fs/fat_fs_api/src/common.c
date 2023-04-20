/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"

/* FatFs work area */
FATFS fat_fs;
struct fs_file_t filep;
const char test_str[] = "hello world!";

int check_file_dir_exists(const char *path)
{
	int res;
	struct fs_dirent entry;

	/* Verify fs_stat() */
	res = fs_stat(path, &entry);

	return !res;
}
