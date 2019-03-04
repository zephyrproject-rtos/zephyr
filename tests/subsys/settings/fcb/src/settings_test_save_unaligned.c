/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_fcb.h"

void test_config_save_fcb_unaligned(void)
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

	if (cf.cf_fcb.f_align == 1) {
		/* override flash driver alignment */
		cf.cf_fcb.f_align = 4;
	}

	settings_mount_fcb_backend(&cf);

	rc = settings_fcb_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FCB as configuration destination");

	val8_un = 33U;
	rc = settings_save();
	zassert_true(rc == 0, "fcb write error");

	val8_un = 0U;

	rc = settings_load();
	zassert_true(rc == 0, "fcb redout error");
	zassert_true(val8_un == 33U, "bad value read");

	val8_un = 15U;
	rc = settings_save();
	zassert_true(rc == 0, "fcb write error");
}
