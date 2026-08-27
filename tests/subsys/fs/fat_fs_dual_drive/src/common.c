/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"

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
