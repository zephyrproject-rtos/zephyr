/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

static void test_fcb_append(struct fcb *_fcb)
{
	int rc;
	struct fcb_entry loc;
	uint8_t test_data[128];
	int i;
	int j;
	int var_cnt;

	for (i = 0; i < sizeof(test_data); i++) {
		for (j = 0; j < i; j++) {
			test_data[j] = fcb_test_append_data(i, j);
		}
		rc = fcb_append(_fcb, i, &loc);
		zassert_true(rc == 0, "fcb_append call failure");
		rc = flash_area_write(_fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc),
				      test_data, i);
		zassert_true(rc == 0, "flash_area_write call failure");
		rc = fcb_append_finish(_fcb, &loc);
		zassert_true(rc == 0, "fcb_append_finish call failure");
	}

	var_cnt = 0;
	rc = fcb_walk(_fcb, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(var_cnt == sizeof(test_data),
		     "fetched data size not match to wrote data size");
}

ZTEST(fcb_test_with_2sectors_set, test_fcb_append_2sectors)
{
	test_fcb_append(&test_fcb);
}

ZTEST(fcb_test_crc_disabled, test_fcb_append_crc_disabled)
{
	test_fcb_append(&test_fcb_crc_disabled);
}
