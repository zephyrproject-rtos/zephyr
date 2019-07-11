/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/crc.h>

#include <fs/fcb.h>
#include "fcb_priv.h"

/*
 * Given offset in flash sector, fill in rest of the fcb_entry, and crc8 over
 * the data.
 */
int
fcb_elem_crc8(struct fcb *fcb, struct fcb_entry *loc, u8_t *c8p)
{
	u8_t tmp_str[FCB_TMP_BUF_SZ];
	int cnt;
	int blk_sz;
	u8_t crc8;
	u16_t len;
	u32_t off;
	u32_t end;
	int rc;

	if (loc->fe_elem_off + 2 > loc->fe_sector->fs_size) {
		return -ENOTSUP;
	}
	rc = fcb_flash_read(fcb, loc->fe_sector, loc->fe_elem_off, tmp_str, 2);
	if (rc) {
		return -EIO;
	}

	cnt = fcb_get_len(tmp_str, &len);
	if (cnt < 0) {
		return cnt;
	}
	loc->fe_data_off = loc->fe_elem_off + fcb_len_in_flash(fcb, cnt);
	loc->fe_data_len = len;

	crc8 = CRC8_CCITT_INITIAL_VALUE;
	crc8 = crc8_ccitt(crc8, tmp_str, cnt);

	off = loc->fe_data_off;
	end = loc->fe_data_off + len;
	for (; off < end; off += blk_sz) {
		blk_sz = end - off;
		if (blk_sz > sizeof(tmp_str)) {
			blk_sz = sizeof(tmp_str);
		}

		rc = fcb_flash_read(fcb, loc->fe_sector, off, tmp_str, blk_sz);
		if (rc) {
			return -EIO;
		}
		crc8 = crc8_ccitt(crc8, tmp_str, blk_sz);
	}
	*c8p = crc8;

	return 0;
}

int fcb_elem_info(struct fcb *fcb, struct fcb_entry *loc)
{
	int rc;
	u8_t crc8;
	u8_t fl_crc8;
	off_t off;

	rc = fcb_elem_crc8(fcb, loc, &crc8);
	if (rc) {
		return rc;
	}
	off = loc->fe_data_off + fcb_len_in_flash(fcb, loc->fe_data_len);

	rc = fcb_flash_read(fcb, loc->fe_sector, off, &fl_crc8, sizeof(fl_crc8));
	if (rc) {
		return -EIO;
	}

	if (fl_crc8 != crc8) {
		return -EBADMSG;
	}
	return 0;
}
