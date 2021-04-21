/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fs/fs.h>
#include "test_common.h"
#include "test_littlefs.h"
#include "test_littlefs_priv.h"

static struct fs_file_t test_file;
static const char *test_str = "Hello world LITTLEFS";

void test_littlefs_open(void)
{
	fs_file_t_init(&test_file);
	zassert_true(test_file_open(&test_file, TEST_FILE_PATH) == TC_PASS,
		NULL);
}

void test_littlefs_write(void)
{
	TC_PRINT("Write to file %s\n", TEST_FILE_PATH);
	zassert_true(test_file_write(&test_file, test_str) == TC_PASS,
		NULL);
}

void test_littlefs_read(void)
{
	zassert_true(test_file_read(&test_file, test_str) == TC_PASS, NULL);
}

void test_littlefs_close(void)
{
	zassert_true(test_file_close(&test_file) == TC_PASS, NULL);
}
void test_littlefs_unlink(void)
{
	zassert_true(test_file_delete(TEST_FILE_PATH) == TC_PASS, NULL);
}
