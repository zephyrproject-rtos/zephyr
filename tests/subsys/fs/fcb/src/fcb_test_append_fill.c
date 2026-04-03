/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

ZTEST(fcb_test_with_2sectors_set, test_fcb_append_fill)
{
	struct fcb *fcb;
	int rc;
	int i;
	struct fcb_entry loc;
	uint8_t test_data[128];
	int elem_cnts[2] = {0, 0};
	int aa_together_cnts[2];
	struct append_arg aa_together = {
		.elem_cnts = aa_together_cnts
	};
	int aa_separate_cnts[2];
	struct append_arg aa_separate = {
		.elem_cnts = aa_separate_cnts
	};

	fcb = &test_fcb;

	for (i = 0; i < sizeof(test_data); i++) {
		test_data[i] = fcb_test_append_data(sizeof(test_data), i);
	}

	while (1) {
		rc = fcb_append(fcb, sizeof(test_data), &loc);
		if (rc == -ENOSPC) {
			break;
		}
		if (loc.fe_sector == &test_fcb_sector[0]) {
			elem_cnts[0]++;
		} else if (loc.fe_sector == &test_fcb_sector[1]) {
			elem_cnts[1]++;
		} else {
			zassert_true(0,
				     "unexpected flash area of appended loc");
		}

		rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc),
				      test_data, sizeof(test_data));
		zassert_true(rc == 0, "flash_area_write call failure");

		rc = fcb_append_finish(fcb, &loc);
		zassert_true(rc == 0, "fcb_append_finish call failure");
	}
	zassert_true(elem_cnts[0] > 0,
		     "appendend count should be greater than zero");
	zassert_true(elem_cnts[0] == elem_cnts[1],
		     "appendend counts should equal to each other");

	(void)memset(&aa_together_cnts, 0, sizeof(aa_together_cnts));
	rc = fcb_walk(fcb, NULL, fcb_test_cnt_elems_cb, &aa_together);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(aa_together.elem_cnts[0] == elem_cnts[0],
		     "fcb_walk: elements count read different than expected");
	zassert_true(aa_together.elem_cnts[1] == elem_cnts[1],
		     "fcb_walk: elements count read different than expected");

	(void)memset(&aa_separate_cnts, 0, sizeof(aa_separate_cnts));
	rc = fcb_walk(fcb, &test_fcb_sector[0], fcb_test_cnt_elems_cb,
	  &aa_separate);
	zassert_true(rc == 0, "fcb_walk call failure");
	rc = fcb_walk(fcb, &test_fcb_sector[1], fcb_test_cnt_elems_cb,
	  &aa_separate);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(aa_separate.elem_cnts[0] == elem_cnts[0],
		     "fcb_walk: elements count read different than expected");
	zassert_true(aa_separate.elem_cnts[1] == elem_cnts[1],
		     "fcb_walk: elements count read different than expected");
}
