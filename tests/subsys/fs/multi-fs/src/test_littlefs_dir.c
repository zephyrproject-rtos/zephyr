/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/fs/fs.h>
#include "test_common.h"
#include "test_littlefs.h"
#include "test_littlefs_priv.h"


void test_littlefs_mkdir(void)
{
	zassert_true(test_mkdir(TEST_DIR_PATH, TEST_FILE) == TC_PASS);
}

void test_littlefs_readdir(void)
{
	zassert_true(test_lsdir(TEST_DIR_PATH) == TC_PASS);
}

void test_littlefs_rmdir(void)
{
	zassert_true(test_rmdir(TEST_DIR_PATH) == TC_PASS);
}
