/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_fcb.h"

void test_config_save_1_fcb(void)
{
	int rc;
	struct settings_fcb cf;

	config_wipe_srcs();

	cf.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC;
	cf.cf_fcb.f_sectors = fcb_sectors;
	cf.cf_fcb.f_sector_cnt = ARRAY_SIZE(fcb_sectors);

	rc = settings_fcb_src(&cf);
	ztest_true(rc == 0, "can't register FCB as configuration source");

	settings_mount_fcb_backend(&cf);

	rc = settings_fcb_dst(&cf);
	ztest_true(rc == 0,
		     "can't register FCB as configuration destination");

	val8 = 33U;
	rc = settings_save();
	ztest_true(rc == 0, "fcb write error");

	val8 = 0U;

	rc = settings_load();
	ztest_true(rc == 0, "fcb redout error");
	ztest_true(val8 == 33U, "bad value read");

	val8 = 15U;
	rc = settings_save();
	ztest_true(rc == 0, "fcb write error");
}
