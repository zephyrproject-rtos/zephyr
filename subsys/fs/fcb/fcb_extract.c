/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/fs/fcb.h>
#include "fcb_priv.h"

int
fcb_extract(struct fcb *fcb, struct fcb_entry *loc)
{
	int rc;

	rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
	if (rc) {
		return -EINVAL;
	}
	memset(loc, 0, sizeof(struct fcb_entry));
	rc = fcb_getnext_nolock(fcb, loc);
	k_mutex_unlock(&fcb->f_mtx);

	return rc;
}

int
fcb_extract_finish_nolock(struct fcb *fcb, struct fcb_entry *loc)
{
	int rc;
	uint8_t buffer[MAX(fcb->f_align, FCB_TMP_BUF_SZ)];
	uint8_t crc8[fcb->f_align];
	off_t off;

	(void)memset(buffer, 0, sizeof(buffer));
	uint16_t total_len = fcb_len_in_flash(fcb, loc->fe_data_len);
	uint16_t max_write_len = sizeof(buffer);
	while (fcb_len_in_flash(fcb, max_write_len) > sizeof(buffer)) {
		max_write_len /= 2;
	}

	while (total_len > 0) {
		rc = fcb_flash_write(fcb, loc->fe_sector, loc->fe_data_off,
							 buffer, MIN(total_len, max_write_len));
		if (rc) {
			return -EIO;
		}
		total_len -= MIN(total_len, max_write_len);
	}

	(void)memset(crc8, 0xFF, sizeof(crc8));

	rc = fcb_elem_crc8(fcb, loc, &crc8[0]);
	if (rc) {
		return rc;
	}

	off = loc->fe_data_off + fcb_len_in_flash(fcb, loc->fe_data_len);
	rc = fcb_flash_read(fcb, loc->fe_sector, off,
						buffer, fcb_len_in_flash(fcb, FCB_CRC_SZ));
	if (rc) {
		return -EIO;
	}

	if (crc8[0] == buffer[0]) {
		(void)memset(buffer, 0, sizeof(buffer));
		rc = fcb_flash_write(fcb, loc->fe_sector, off,
							 buffer, fcb_len_in_flash(fcb, FCB_CRC_SZ));
		if (rc) {
			return -EIO;
		}
	}

	return rc;
}

int
fcb_extract_finish(struct fcb *fcb, struct fcb_entry *extract_loc)
{
	int rc;

	rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
	if (rc) {
		return -EINVAL;
	}
	rc = fcb_extract_finish_nolock(fcb, extract_loc);
	k_mutex_unlock(&fcb->f_mtx);

	return rc;
}
