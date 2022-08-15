/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_fcb.h"

#ifdef TEST_LONG
#define TESTS_S3_FCB_ITERATIONS 4096
#else
#define TESTS_S3_FCB_ITERATIONS 100
#endif

ZTEST(settings_config_fcb, test_config_save_3_fcb)
{
	int rc;
	struct settings_fcb cf;
	int i;

	rc = settings_register(&c_test_handlers[2]);
	zassert_true(rc == 0 || rc == -EEXIST, "settings_register fail");
	config_wipe_srcs();
	config_wipe_fcb(fcb_sectors, ARRAY_SIZE(fcb_sectors));

	cf.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC;
	cf.cf_fcb.f_sectors = fcb_sectors;
	cf.cf_fcb.f_sector_cnt = 4;

	rc = settings_fcb_src(&cf);
	zassert_true(rc == 0, "can't register FCB as configuration source");

	rc = settings_fcb_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FCB as configuration destination");

	for (i = 0; i < TESTS_S3_FCB_ITERATIONS; i++) {
		val32 = i;

		rc = settings_save();
		zassert_true(rc == 0, "fcb write error");

		val32 = 0U;

		rc = settings_load();
		zassert_true(rc == 0, "fcb read error");
		zassert_true(val32 == i, "bad value read");
	}
	settings_unregister(&c_test_handlers[2]);
}
