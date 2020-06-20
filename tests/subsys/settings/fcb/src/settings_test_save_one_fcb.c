/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_fcb.h"

static int test_config_save_one_byte_value(const char *name, uint8_t val)
{
	return settings_save_one(name, &val, 1);
}

void test_config_save_one_fcb(void)
{
	int rc;
	struct settings_fcb cf;

	config_wipe_srcs();
	config_wipe_fcb(fcb_sectors, ARRAY_SIZE(fcb_sectors));

	cf.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC;
	cf.cf_fcb.f_sectors = fcb_sectors;
	cf.cf_fcb.f_sector_cnt = ARRAY_SIZE(fcb_sectors);

	rc = settings_fcb_src(&cf);
	zassert_true(rc == 0, "can't register FCB as configuration source");

	rc = settings_fcb_dst(&cf);
	zassert_true(rc == 0,
			 "can't register FCB as configuration destination");

	val8 = 33U;
	rc = settings_save();
	zassert_true(rc == 0, "fcb write error");

	rc = test_config_save_one_byte_value("myfoo/mybar", 42);
	zassert_true(rc == 0, "fcb one item write error");

	rc = settings_load();
	zassert_true(rc == 0, "fcb read error");
	zassert_true(val8 == 42U, "bad value read");

	rc = test_config_save_one_byte_value("myfoo/mybar", 44);
	zassert_true(rc == 0, "fcb one item write error");

	rc = settings_load();
	zassert_true(rc == 0, "fcb read error");
	zassert_true(val8 == 44U, "bad value read");
}
