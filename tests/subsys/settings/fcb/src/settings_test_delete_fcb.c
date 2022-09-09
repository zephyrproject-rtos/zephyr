/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_fcb.h"

ZTEST(settings_config_fcb, test_config_delete_fcb)
{
	int rc;
	struct settings_fcb cf;

	rc = settings_register(&c_test_handlers[0]);
	zassert_true(rc == 0 || rc == -EEXIST, "settings_register fail");
	config_wipe_srcs();
	config_wipe_fcb(fcb_sectors, ARRAY_SIZE(fcb_sectors));

	cf.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC;
	cf.cf_fcb.f_sectors = fcb_sectors;
	cf.cf_fcb.f_sector_cnt = ARRAY_SIZE(fcb_sectors);

	rc = settings_fcb_src(&cf);
	zassert_true(rc == 0, "can't register FCB as configuration source");

	settings_mount_fcb_backend(&cf);

	rc = settings_fcb_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FCB as configuration destination");

	val8 = 153U;
	rc = settings_save();
	zassert_true(rc == 0, "fcb write error");

	val8 = 0U;

	rc = settings_load();
	zassert_true(rc == 0, "fcb redout error");
	zassert_true(val8 == 153U, "bad value read");

	val8 = 0x55;

	settings_delete("myfoo/mybar");
	rc = settings_load();
	zassert_true(rc == 0, "fcb redout error");
	zassert_true(val8 == 0x55, "bad value read");
	settings_unregister(&c_test_handlers[0]);
}
