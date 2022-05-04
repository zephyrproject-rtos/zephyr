/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

void (test_fcb_append_too_big(void))
{
		struct fcb *fcb;
		int rc;
		int overhead;
		uint32_t len;
		struct fcb_entry elem_loc;
		const uint8_t max_length_field_len = 2;

		fcb = &test_fcb;

		/*
		 * An entry in the FCB has the following structure:
		 * | fcb_disk_area | length | data | CRC |
		 *
		 * The length is encoded in 1-2 bytes. All these entries
		 * have to abide flash alignment constraints.
		 *
		 * Thus, the max element which fits inside sector is
		 * (M / fcb->f_align) * fcb->f_align, where / is integer
		 * division and M = sector size - O, with O being the sum
		 * of all overhead elements' lengths in flash, i.e.
		 *
		 * O = fcb_len_in_flash(fcb, FCB_CRC_SZ)
		 *    + fcb_len_in_flash(fcb, max_length_field_len)
		 *    + fcb_len_in_flash(fcb, sizeof(struct fcb_disk_area))
		 */
		len = MIN(FCB_MAX_LEN
			     + fcb_len_in_flash(fcb, sizeof(struct fcb_disk_area))
			     + fcb_len_in_flash(fcb, FCB_CRC_SZ)
			     + fcb_len_in_flash(fcb, max_length_field_len)
			, fcb->f_active.fe_sector->fs_size);


		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc != 0,
			     "fcb_append call should fail for too big entry");

		len--;
		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc != 0,
			     "fcb_append call should fail for too big entry");

		len -=  fcb_len_in_flash(fcb, sizeof(struct fcb_disk_area));
		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc != 0,
			     "fcb_append call should fail for too big entry");

		overhead = fcb_len_in_flash(fcb, sizeof(struct fcb_disk_area))
			   + fcb_len_in_flash(fcb, max_length_field_len)
			   + fcb_len_in_flash(fcb, FCB_CRC_SZ);

		len = MIN(FCB_MAX_LEN, fcb->f_active.fe_sector->fs_size - overhead);
		len = fcb->f_align * (len / fcb->f_align);

		rc = fcb_append(fcb, len, &elem_loc);
		zassert_true(rc == 0, "fcb_append call failure");

		rc = fcb_append_finish(fcb, &elem_loc);
		zassert_true(rc == 0, "fcb_append call failure");

		rc = fcb_elem_info(fcb, &elem_loc);
		zassert_true(rc == 0, "fcb_elem_info call failure");
		zassert_true(elem_loc.fe_data_len == len,
		"entry length fetched should match length of appended entry");
}
