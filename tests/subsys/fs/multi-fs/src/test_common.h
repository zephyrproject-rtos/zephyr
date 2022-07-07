/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/fs/fs.h>
#include <limits.h>

void test_clear_flash(void);
int check_file_dir_exists(const char *fpath);

int test_file_open(struct fs_file_t *filep, const char *fpath);
int test_file_write(struct fs_file_t *filep, const char *str);
int test_file_read(struct fs_file_t *filep, const char *test_str);
int test_file_close(struct fs_file_t *filep);
int test_file_delete(const char *fpath);

int test_rmdir(const char *dir_path);
int test_mkdir(const char *dir_path, const char *file);
int test_lsdir(const char *path);
