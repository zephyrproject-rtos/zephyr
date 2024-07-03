/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/crc.h>

#include <zephyr/fs/fcb.h>
#include "fcb_priv.h"

#define FCB_FIXED_ENDMARKER 0xab

/*
 * Given offset in flash sector, fill in rest of the fcb_entry, and crc8 over
 * the data.
 */
static int
fcb_elem_crc8(struct fcb *_fcb, struct fcb_entry *loc, uint8_t *c8p)
{
	uint8_t tmp_str[FCB_TMP_BUF_SZ];
	int cnt;
	int blk_sz;
	uint8_t crc8;
	uint16_t len;
	uint32_t off;
	uint32_t end;
	int rc;

	if (loc->fe_elem_off + 2 > loc->fe_sector->fs_size) {
		return -ENOTSUP;
	}

	rc = fcb_flash_read(_fcb, loc->fe_sector, loc->fe_elem_off, tmp_str, 2);
	if (rc) {
		return -EIO;
	}

	cnt = fcb_get_len(_fcb, tmp_str, &len);
	if (cnt < 0) {
		return cnt;
	}
	loc->fe_data_off = loc->fe_elem_off + fcb_len_in_flash(_fcb, cnt);
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

		rc = fcb_flash_read(_fcb, loc->fe_sector, off, tmp_str, blk_sz);
		if (rc) {
			return -EIO;
		}
		crc8 = crc8_ccitt(crc8, tmp_str, blk_sz);
	}
	*c8p = crc8;

	return 0;
}

#if defined(CONFIG_FCB_ALLOW_FIXED_ENDMARKER)
/* Given the offset in flash sector, calculate the FCB entry data offset and size, and set
 * the fixed endmarker.
 */
static int
fcb_elem_endmarker_fixed(struct fcb *_fcb, struct fcb_entry *loc, uint8_t *em)
{
	uint8_t tmp_str[2];
	int cnt;
	uint16_t len;
	int rc;

	if (loc->fe_elem_off + 2 > loc->fe_sector->fs_size) {
		return -ENOTSUP;
	}

	rc = fcb_flash_read(_fcb, loc->fe_sector, loc->fe_elem_off, tmp_str, 2);
	if (rc) {
		return -EIO;
	}

	cnt = fcb_get_len(_fcb, tmp_str, &len);
	if (cnt < 0) {
		return cnt;
	}
	loc->fe_data_off = loc->fe_elem_off + fcb_len_in_flash(_fcb, cnt);
	loc->fe_data_len = len;

	*em = FCB_FIXED_ENDMARKER;
	return 0;
}
#endif /* defined(CONFIG_FCB_ALLOW_FIXED_ENDMARKER) */

/* Given the offset in flash sector, calculate the FCB entry data offset and size, and calculate
 * the expected endmarker.
 */
int
fcb_elem_endmarker(struct fcb *_fcb, struct fcb_entry *loc, uint8_t *em)
{
#if defined(CONFIG_FCB_ALLOW_FIXED_ENDMARKER)
	if (_fcb->f_flags & FCB_FLAGS_CRC_DISABLED) {
		return fcb_elem_endmarker_fixed(_fcb, loc, em);
	}
#endif /* defined(CONFIG_FCB_ALLOW_FIXED_ENDMARKER) */

	return fcb_elem_crc8(_fcb, loc, em);
}

/* Given the offset in flash sector, calculate the FCB entry data offset and size, and verify that
 * the FCB entry endmarker is correct.
 */
int fcb_elem_info(struct fcb *_fcb, struct fcb_entry *loc)
{
	int rc;
	uint8_t em;
	uint8_t fl_em;
	off_t off;

	rc = fcb_elem_endmarker(_fcb, loc, &em);
	if (rc) {
		return rc;
	}
	off = loc->fe_data_off + fcb_len_in_flash(_fcb, loc->fe_data_len);

	rc = fcb_flash_read(_fcb, loc->fe_sector, off, &fl_em, sizeof(fl_em));
	if (rc) {
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_FCB_ALLOW_FIXED_ENDMARKER) && (fl_em != em)) {
		rc = fcb_elem_crc8(_fcb, loc, &em);
		if (rc) {
			return rc;
		}
	}

	if (fl_em != em) {
		return -EBADMSG;
	}
	return 0;
}
