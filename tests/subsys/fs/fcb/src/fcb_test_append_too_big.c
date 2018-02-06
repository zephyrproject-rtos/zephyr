/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

void (fcb_test_append_too_big(void))
{
		struct fcb *fcb;
		int rc;
		int len;
		struct fcb_entry elem_loc;

		fcb = &test_fcb;

		/*
		 * Max element which fits inside sector is
		 * sector size - (disk header + crc + 1-2 bytes of length).
		 */
		len = fcb->f_active.fe_sector->fs_size;

		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc != 0,
			     "fcb_append call should fail for too big entry");

		len--;
		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc != 0,
			     "fcb_append call should fail for too big entry");

		len -= sizeof(struct fcb_disk_area);
		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc != 0,
			     "fcb_append call should fail for too big entry");

		len = fcb->f_active.fe_sector->fs_size -
			(sizeof(struct fcb_disk_area) + 1 + 2);
		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc == 0, "fcb_append call failure");

		rc = fcb_append_finish(fcb, &elem_loc);
		zassert_true(rc == 0, "fcb_append call failure");

		rc = fcb_elem_info(fcb, &elem_loc);
		zassert_true(rc == 0, "fcb_elem_info call failure");
		zassert_true(elem_loc.fe_data_len == len,
		"entry length fetched should match length of appended entry");
}
