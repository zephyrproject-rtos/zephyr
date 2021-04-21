/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

void test_fcb_reset(void)
{
	struct fcb *fcb;
	int rc;
	int i;
	struct fcb_entry loc;
	uint8_t test_data[128];
	int var_cnt;

	fcb = &test_fcb;

	var_cnt = 0;
	rc = fcb_walk(fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == 0,
		     "fcb_walk: elements count read different than expected");

	rc = fcb_append(fcb, 32, &loc);
	zassert_true(rc == 0, "fcb_append call failure");

	/*
	 * No ready ones yet. CRC should not match.
	 */
	var_cnt = 0;
	rc = fcb_walk(fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == 0,
		     "fcb_walk: elements count read different than expected");

	for (i = 0; i < sizeof(test_data); i++) {
		test_data[i] = fcb_test_append_data(32, i);
	}
	rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc), test_data,
			      32);
	zassert_true(rc == 0, "flash_area_write call failure");

	rc = fcb_append_finish(fcb, &loc);
	zassert_true(rc == 0, "fcb_append_finish call failure");

	/*
	 * one entry
	 */
	var_cnt = 32;
	rc = fcb_walk(fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == 33,
		     "fcb_walk: elements count read different than expected");

	/*
	 * Pretend reset
	 */
	(void)memset(fcb, 0, sizeof(*fcb));
	fcb->f_sector_cnt = 2U;
	fcb->f_sectors = test_fcb_sector;

	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, fcb);
	zassert_true(rc == 0, "fcb_init call failure");

	var_cnt = 32;
	rc = fcb_walk(fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == 33,
		     "fcb_walk: elements count read different than expected");

	rc = fcb_append(fcb, 33, &loc);
	zassert_true(rc == 0, "fcb_append call failure");

	for (i = 0; i < sizeof(test_data); i++) {
		test_data[i] = fcb_test_append_data(33, i);
	}
	rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc), test_data,
			      33);
	zassert_true(rc == 0, "flash_area_write call failure");

	rc = fcb_append_finish(fcb, &loc);
	zassert_true(rc == 0, "fcb_append_finish call failure");

	var_cnt = 32;
	rc = fcb_walk(fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == 34,
		     "fcb_walk: elements count read different than expected");

	/*
	 * Add partial one, make sure that we survive reset then.
	 */
	rc = fcb_append(fcb, 34, &loc);
	zassert_true(rc == 0, "fcb_append call failure");

	(void)memset(fcb, 0, sizeof(*fcb));
	fcb->f_sector_cnt = 2U;
	fcb->f_sectors = test_fcb_sector;

	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, fcb);
	zassert_true(rc == 0, "fcb_init call failure");

	/*
	 * Walk should skip that.
	 */
	var_cnt = 32;
	rc = fcb_walk(fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == 34,
		     "fcb_walk: elements count read different than expected");

	/* Add a 3rd one, should go behind corrupt entry */
	rc = fcb_append(fcb, 34, &loc);
	zassert_true(rc == 0, "fcb_append call failure");

	for (i = 0; i < sizeof(test_data); i++) {
		test_data[i] = fcb_test_append_data(34, i);
	}
	rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc), test_data,
			      34);
	zassert_true(rc == 0, "flash_area_write call failure");

	rc = fcb_append_finish(fcb, &loc);
	zassert_true(rc == 0, "fcb_append_finish call failure");

	/*
	 * Walk should skip corrupt entry, but report the next one.
	 */
	var_cnt = 32;
	rc = fcb_walk(fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == 35,
		     "fcb_walk: elements count read different than expected");
}
