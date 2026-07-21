/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

ZTEST(fcb_test_with_4sectors_set, test_fcb_last_of_n)
{
	const uint8_t ENTRIES = 5U;
	struct fcb *fcb;
	int rc;
	struct fcb_entry loc;
	struct fcb_entry areas[ENTRIES];
	uint8_t test_data[128] = {0};
	uint8_t i;

	fcb = &test_fcb;
	fcb->f_scratch_cnt = 1U;

	/* No fcbs available */
	rc = fcb_offset_last_n(fcb, 1, &loc);
	zassert_true(rc != 0, "No fcbs available");

	/*
	 * Add some fcbs.
	 */
	for (i = 0U; i < ENTRIES; i++) {
		rc = fcb_append(fcb, sizeof(test_data), &loc);
		if (rc == -ENOSPC) {
			break;
		}

		rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc),
				      test_data, sizeof(test_data));
		zassert_true(rc == 0, "flash_area_write call failure");

		rc = fcb_append_finish(fcb, &loc);
		zassert_true(rc == 0, "fcb_append_finish call failure");

		areas[i] = loc;
	}

	/* last entry */
	rc = fcb_offset_last_n(fcb, 1, &loc);
	zassert_true(rc == 0, "fcb_offset_last_n call failure");
	zassert_true(areas[4].fe_sector == loc.fe_sector &&
		     areas[4].fe_data_off == loc.fe_data_off &&
		     areas[4].fe_data_len == loc.fe_data_len,
		     "fcb_offset_last_n: fetched wrong n-th location");

	/* somewhere in the middle */
	rc = fcb_offset_last_n(fcb, 3, &loc);
	zassert_true(rc == 0, "fcb_offset_last_n call failure");
	zassert_true(areas[2].fe_sector == loc.fe_sector &&
		     areas[2].fe_data_off == loc.fe_data_off &&
		     areas[2].fe_data_len == loc.fe_data_len,
		     "fcb_offset_last_n: fetched wrong n-th location");

	/* first entry */
	rc = fcb_offset_last_n(fcb, 5, &loc);
	zassert_true(rc == 0, "fcb_offset_last_n call failure");
	zassert_true(areas[0].fe_sector == loc.fe_sector &&
		     areas[0].fe_data_off == loc.fe_data_off &&
		     areas[0].fe_data_len == loc.fe_data_len,
		     "fcb_offset_last_n: fetched wrong n-th location");

	/* after last valid entry, returns the first one like for 5 */
	rc = fcb_offset_last_n(fcb, 6, &loc);
	zassert_true(rc == 0, "fcb_offset_last_n call failure");
	zassert_true(areas[0].fe_sector == loc.fe_sector &&
		     areas[0].fe_data_off == loc.fe_data_off &&
		     areas[0].fe_data_len == loc.fe_data_len,
		     "fcb_offset_last_n: fetched wrong n-th location");
}
