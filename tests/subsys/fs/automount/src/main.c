/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <ztest.h>
#include <fs/fs.h>

int test_open_lfs0(void)
{
	struct fs_file_t f =  { 0 };

	if (fs_open(&f, "/lfs0/file", FS_O_CREATE) != 0) {
		return TC_FAIL;
	}

	fs_close(&f);

	return TC_PASS;
}

static int test_open_lfs1(void)
{
	struct fs_file_t f =  { 0 };

	if (fs_open(&f, "/lfs1/file", FS_O_CREATE) != 0) {
		return TC_FAIL;
	}

	fs_close(&f);

	return TC_PASS;
}

void test_all(void)
{
	zassert_equal(test_open_lfs0(),
		      TC_PASS,
		      "Failed to create file on /lfs0");
	zassert_equal(test_open_lfs1(),
		      TC_PASS,
		      "Failed to create file on /lfs1");
}

void test_main(void)
{
	ztest_test_suite(automount_test,
			 ztest_unit_test(test_all)
			 );
	ztest_run_test_suite(automount_test);
}
