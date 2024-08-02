/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_file.h"

#define CF_FILE_CONTENT_1 "\x0d\x00myfoo/mybar=\x08"
#define CF_FILE_CONTENT_2 "\x0d\x00myfoo/mybar=\x2b"

ZTEST(settings_config_fs, test_config_save_in_file)
{
	int rc;
	struct settings_file cf;
	const char cf_pattern_1[] = CF_FILE_CONTENT_1;
	const char cf_pattern_2[] = CF_FILE_CONTENT_2;

	config_wipe_srcs();

	rc = fs_mkdir(TEST_CONFIG_DIR);
	zassert_true(rc == 0 || rc == -EEXIST, "can't create directory");

	cf.cf_name = TEST_CONFIG_DIR "/blah";
	cf.cf_maxlines = 1000;
	cf.cf_lines = 0; /* normally fetched while loading, but this is test */
	rc = settings_file_src(&cf);
	zassert_true(rc == 0, "can't register FS as configuration source");

	rc = settings_file_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FS as configuration destination");

	val8 = 8U;
	rc = settings_save();
	zassert_true(rc == 0, "fs write error");

	rc = settings_test_file_strstr(cf.cf_name, cf_pattern_1,
				       sizeof(cf_pattern_1)-1);
	zassert_true(rc == 0, "bad value read");

	val8 = 43U;
	rc = settings_save();
	zassert_true(rc == 0, "fs write error");

	rc = settings_test_file_strstr(cf.cf_name, cf_pattern_2,
				       sizeof(cf_pattern_2)-1);
	zassert_true(rc == 0, "bad value read");
}
