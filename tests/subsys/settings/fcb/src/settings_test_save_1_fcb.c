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
	zassert_true(rc == 0, "can't register FCB as configuration source\n");

	rc = settings_fcb_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FCB as configuration destination\n");

	val8 = 33;
	rc = settings_save();
	zassert_true(rc == 0, "fcb write error\n");

	val8 = 0;

	rc = settings_load();
	zassert_true(rc == 0, "fcb redout error\n");
	zassert_true(val8 == 33, "bad value read\n");
}
