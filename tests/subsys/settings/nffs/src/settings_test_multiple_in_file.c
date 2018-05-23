/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_file.h"

void test_config_multiple_in_file(void)
{
	int rc;
	struct settings_file cf_mfg;
	const char cf_mfg_test1[] =
		"myfoo/mybar=1\n"
		"myfoo/mybar=14";
	const char cf_mfg_test2[] =
		"myfoo/mybar=1\n"
		"myfoo/mybar=15\n"
		"\n";

	config_wipe_srcs();

	cf_mfg.cf_name = TEST_CONFIG_DIR "/mfg";
	rc = settings_file_src(&cf_mfg);
	zassert_true(rc == 0, "can't register FS as configuration source");

	rc = fsutil_write_file(TEST_CONFIG_DIR "/mfg", cf_mfg_test1,
			       sizeof(cf_mfg_test1));
	zassert_true(rc == 0, "can't write to file");

	settings_load();
	zassert_true(test_set_called, "the SET handler wasn't called");
	zassert_true(val8 == 14,
		     "SET handler: was called with wrong parameters");

	rc = fsutil_write_file(TEST_CONFIG_DIR "/mfg", cf_mfg_test2,
			       sizeof(cf_mfg_test2));
	zassert_true(rc == 0, "can't write to file");

	settings_load();
	zassert_true(test_set_called, "the SET handler wasn't called");
	zassert_true(val8 == 15,
		     "SET handler: was called with wrong parameters");
}
