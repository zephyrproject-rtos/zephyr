/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

void fcb_test_multi_scratch(void)
{
	struct fcb *fcb;
	int rc;
	struct fcb_entry loc;
	u8_t test_data[128];
	int elem_cnts[4];
	int idx;
	int cnts[4];
	struct append_arg aa_arg = {
		.elem_cnts = cnts
	};

	fcb = &test_fcb;
	fcb->f_scratch_cnt = 1U;

	/*
	 * Now fill up everything. We should be able to get 3 of the sectors
	 * full.
	 */
	(void)memset(elem_cnts, 0, sizeof(elem_cnts));
	while (1) {
		rc = fcb_append(fcb, sizeof(test_data), &loc);
		if (rc == -ENOSPC) {
			break;
		}
		idx = loc.fe_sector - &test_fcb_sector[0];
		elem_cnts[idx]++;

		rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc),
				      test_data, sizeof(test_data));
		zassert_true(rc == 0, "flash_area_write call failure");

		rc = fcb_append_finish(fcb, &loc);
		zassert_true(rc == 0, "fcb_append_finish call failure");
	}

	zassert_true(elem_cnts[0] > 0, "unexpected entry number was appended");
	zassert_true(elem_cnts[0] == elem_cnts[1] &&
		     elem_cnts[0] == elem_cnts[2],
		     "unexpected entry number was appended");
	zassert_true(elem_cnts[3] == 0, "unexpected entry number was appended");

	/*
	 * Ask to use scratch block, then fill it up.
	 */
	rc = fcb_append_to_scratch(fcb);
	zassert_true(rc == 0, "fcb_append_to_scratch call failure");

	while (1) {
		rc = fcb_append(fcb, sizeof(test_data), &loc);
		if (rc == -ENOSPC) {
			break;
		}
		idx = loc.fe_sector - &test_fcb_sector[0];
		elem_cnts[idx]++;

		rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc),
				      test_data, sizeof(test_data));
		zassert_true(rc == 0, "flash_area_write call failure");

		rc = fcb_append_finish(fcb, &loc);
		zassert_true(rc == 0, "fcb_append_finish call failure");
	}
	zassert_true(elem_cnts[3] == elem_cnts[0],
		     "unexpected entry number was appended");

	/*
	 * Rotate
	 */
	rc = fcb_rotate(fcb);
	zassert_true(rc == 0, "fcb_rotate call failure");

	(void)memset(&cnts, 0, sizeof(cnts));
	rc = fcb_walk(fcb, NULL, fcb_test_cnt_elems_cb, &aa_arg);
	zassert_true(rc == 0, "fcb_walk call failure");

	zassert_true(cnts[0] == 0, "unexpected entry count");
	zassert_true(cnts[1] > 0, "unexpected entry count");
	zassert_true(cnts[1] == cnts[2] && cnts[1] == cnts[3],
		     "unexpected entry count");

	rc = fcb_append_to_scratch(fcb);
	zassert_true(rc == 0, "fcb_append_to_scratch call failure");
	rc = fcb_append_to_scratch(fcb);
	zassert_true(rc != 0, "fcb_append_to_scratch call should fail");
}
