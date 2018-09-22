/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

void fcb_test_rotate(void)
{
	struct fcb *fcb;
	int rc;
	int old_id;
	struct fcb_entry loc;
	u8_t test_data[128];
	int elem_cnts[2] = {0, 0};
	int cnts[2];
	struct append_arg aa_arg = {
		.elem_cnts = cnts
	};

	fcb = &test_fcb;

	old_id = fcb->f_active_id;
	rc = fcb_rotate(fcb);
	zassert_true(rc == 0, "fcb_rotate call failure");
	zassert_true(fcb->f_active_id == old_id + 1,
		     "flash location id should increased");

	/*
	 * Now fill up the
	 */
	while (1) {
		rc = fcb_append(fcb, sizeof(test_data), &loc);
		if (rc == FCB_ERR_NOSPACE) {
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
	zassert_true(elem_cnts[0] > 0 && elem_cnts[0] == elem_cnts[1],
		     "unexpected entry number was appended");

	old_id = fcb->f_active_id;
	rc = fcb_rotate(fcb);
	zassert_true(rc == 0, "fcb_rotate call failure");
	zassert_true(fcb->f_active_id == old_id,
		     "flash location should be kept");

	(void)memset(cnts, 0, sizeof(cnts));
	rc = fcb_walk(fcb, NULL, fcb_test_cnt_elems_cb, &aa_arg);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(aa_arg.elem_cnts[0] == elem_cnts[0] ||
		     aa_arg.elem_cnts[1] == elem_cnts[1],
		     "fcb_walk: entry count got different than expected");
	zassert_true(aa_arg.elem_cnts[0] == 0 || aa_arg.elem_cnts[1] == 0,
		     "fcb_walk: entry count got different than expected");

	/*
	 * One sector is full. The other one should have one entry in it.
	 */
	rc = fcb_append(fcb, sizeof(test_data), &loc);
	zassert_true(rc == 0, "fcb_append call failure");

	rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc), test_data,
			      sizeof(test_data));
	zassert_true(rc == 0, "flash_area_write call failure");

	rc = fcb_append_finish(fcb, &loc);
	zassert_true(rc == 0, "fcb_append_finish call failure");

	old_id = fcb->f_active_id;
	rc = fcb_rotate(fcb);
	zassert_true(rc == 0, "fcb_rotate call failure");
	zassert_true(fcb->f_active_id == old_id,
		     "flash location should be kept");

	(void)memset(cnts, 0, sizeof(cnts));
	rc = fcb_walk(fcb, NULL, fcb_test_cnt_elems_cb, &aa_arg);
	zassert_true(rc == 0, "fcb_walk call failure");
	zassert_true(aa_arg.elem_cnts[0] == 1 || aa_arg.elem_cnts[1] == 1,
		     "fcb_walk: entry count got different than expected");
	zassert_true(aa_arg.elem_cnts[0] == 0 || aa_arg.elem_cnts[1] == 0,
		     "fcb_walk: entry count got different than expected");
}
