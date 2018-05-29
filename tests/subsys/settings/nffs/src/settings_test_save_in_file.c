/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_file.h"

void test_config_save_in_file(void)
{
	int rc;
	struct settings_file cf;

	config_wipe_srcs();

	rc = fs_mkdir(TEST_CONFIG_DIR);
	zassert_true(rc == 0 || rc == -EEXIST, "can't create directory");

	cf.cf_name = TEST_CONFIG_DIR "/blah";
	rc = settings_file_src(&cf);
	zassert_true(rc == 0, "can't register FS as configuration source");

	rc = settings_file_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FS as configuration destination");

	val8 = 8;
	rc = settings_save();
	zassert_true(rc == 0, "fs write error");

	rc = settings_test_file_strstr(cf.cf_name, "myfoo/mybar=8\n");
	zassert_true(rc == 0, "bad value read");

	val8 = 43;
	rc = settings_save();
	zassert_true(rc == 0, "fs write error");

	rc = settings_test_file_strstr(cf.cf_name, "myfoo/mybar=43\n");
	zassert_true(rc == 0, "bad value read");
}
