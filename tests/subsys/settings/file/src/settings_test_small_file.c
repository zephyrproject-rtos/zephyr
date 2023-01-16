/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_file.h"

#define CF_MFG_TEST_STR "\x0D\x00myfoo/mybar=\x01"
#define CF_RUNNING_TEST_STR "\x0D\x00myfoo/mybar=\x08"

ZTEST(settings_config_fs, test_config_small_file)
{
	int rc;
	struct settings_file cf_mfg;
	struct settings_file cf_running;
	const char cf_mfg_test[] = CF_MFG_TEST_STR;
	const char cf_running_test[] = CF_RUNNING_TEST_STR;

	config_wipe_srcs();

	cf_mfg.cf_name = TEST_CONFIG_DIR "/mfg";
	cf_running.cf_name = TEST_CONFIG_DIR "/running";

	rc = settings_file_src(&cf_mfg);
	zassert_true(rc == 0, "can't register FS as configuration source");
	rc = settings_file_src(&cf_running);
	zassert_true(rc == 0, "can't register FS as configuration source");

	rc = fsutil_write_file(TEST_CONFIG_DIR "/mfg", cf_mfg_test,
			       sizeof(cf_mfg_test));
	zassert_true(rc == 0, "can't write to file");

	settings_load();
	zassert_true(test_set_called, "the SET handler wasn't called");
	zassert_true(val8 == 1U,
		     "SET handler: was called with wrong parameters");

	ctest_clear_call_state();

	rc = fsutil_write_file(TEST_CONFIG_DIR "/running", cf_running_test,
			       sizeof(cf_running_test));
	zassert_true(rc == 0, "can't write to file");

	settings_load();
	zassert_true(test_set_called, "the SET handler wasn't called");
	zassert_true(val8 == 8U,
		     "SET handler: was called with wrong parameters");

	ctest_clear_call_state();
}
