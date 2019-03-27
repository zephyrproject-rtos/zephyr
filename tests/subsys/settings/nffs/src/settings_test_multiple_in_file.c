/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_file.h"

#ifdef CONFIG_SETTINGS_USE_BASE64
#define CF_MFG_TEST1 "\x10\x00myfoo/mybar=AQ=="\
		     "\x10\x00myfoo/mybar=Dg=="
#define CF_MFG_TEST2 "\x10\x00myfoo/mybar=AQ=="\
		     "\x10\x00myfoo/mybar=Dw=="
#else
#define CF_MFG_TEST1 "\x0d\x00myfoo/mybar=\x01"\
		     "\x0d\x00myfoo/mybar=\x0e"
#define CF_MFG_TEST2 "\x0d\x00myfoo/mybar=\x01"\
		     "\x0d\x00myfoo/mybar=\x0f"
#endif

void test_config_multiple_in_file(void)
{
	int rc;
	struct settings_file cf_mfg;
	const char cf_mfg_test1[] = CF_MFG_TEST1;
	const char cf_mfg_test2[] = CF_MFG_TEST2;

	config_wipe_srcs();

	cf_mfg.cf_name = TEST_CONFIG_DIR "/mfg";
	rc = settings_file_src(&cf_mfg);
	zassert_true(rc == 0, "can't register FS as configuration source");

	rc = fsutil_write_file(TEST_CONFIG_DIR "/mfg", cf_mfg_test1,
			       sizeof(cf_mfg_test1));
	zassert_true(rc == 0, "can't write to file");

	settings_load();
	zassert_true(test_set_called, "the SET handler wasn't called");
	zassert_true(val8 == 14U,
		     "SET handler: was called with wrong parameters");

	rc = fsutil_write_file(TEST_CONFIG_DIR "/mfg", cf_mfg_test2,
			       sizeof(cf_mfg_test2));
	zassert_true(rc == 0, "can't write to file");

	settings_load();
	zassert_true(test_set_called, "the SET handler wasn't called");
	zassert_true(val8 == 15U,
		     "SET handler: was called with wrong parameters");
}
