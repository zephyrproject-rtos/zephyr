/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/fs/fcb.h>
#include "fcb_priv.h"

static struct flash_sector *
fcb_new_sector(struct fcb *fcb, int cnt)
{
	struct flash_sector *prev;
	struct flash_sector *cur;
	int i;

	prev = NULL;
	i = 0;
	cur = fcb->f_active.fe_sector;
	do {
		cur = fcb_getnext_sector(fcb, cur);
		if (!prev) {
			prev = cur;
		}
		if (cur == fcb->f_oldest) {
			return NULL;
		}
	} while (i++ < cnt);
	return prev;
}

/*
 * Take one of the scratch blocks into use, if at all possible.
 */
int
fcb_append_to_scratch(struct fcb *fcb)
{
	struct flash_sector *sector;
	int rc;

	sector = fcb_new_sector(fcb, 0);
	if (!sector) {
		return -ENOSPC;
	}
	rc = fcb_sector_hdr_init(fcb, sector, fcb->f_active_id + 1);
	if (rc) {
		return rc;
	}
	fcb->f_active.fe_sector = sector;
	fcb->f_active.fe_elem_off = fcb_len_in_flash(fcb, sizeof(struct fcb_disk_area));
	fcb->f_active_id++;
	return 0;
}

int
fcb_append(struct fcb *fcb, uint16_t len, struct fcb_entry *append_loc)
{
	struct flash_sector *sector;
	struct fcb_entry *active;
	int cnt;
	int rc;
	uint8_t tmp_str[MAX(8, fcb->f_align)];

	/* Ensure defined value of padding bytes */
	memset(tmp_str, fcb->f_erase_value, sizeof(tmp_str));

	cnt = fcb_put_len(fcb, tmp_str, len);
	if (cnt < 0) {
		return cnt;
	}
	cnt = fcb_len_in_flash(fcb, cnt);
	len = fcb_len_in_flash(fcb, len) + fcb_len_in_flash(fcb, FCB_CRC_SZ);

	__ASSERT_NO_MSG(cnt <= sizeof(tmp_str));

	rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
	if (rc) {
		return -EINVAL;
	}
	active = &fcb->f_active;
	if (active->fe_elem_off + len + cnt > active->fe_sector->fs_size) {
		sector = fcb_new_sector(fcb, fcb->f_scratch_cnt);
		if (!sector || (sector->fs_size <
			fcb_len_in_flash(fcb, sizeof(struct fcb_disk_area)) + len + cnt)) {
			rc = -ENOSPC;
			goto err;
		}
		rc = fcb_sector_hdr_init(fcb, sector, fcb->f_active_id + 1);
		if (rc) {
			goto err;
		}
		fcb->f_active.fe_sector = sector;
		fcb->f_active.fe_elem_off = fcb_len_in_flash(fcb, sizeof(struct fcb_disk_area));
		fcb->f_active_id++;
	}

	rc = fcb_flash_write(fcb, active->fe_sector, active->fe_elem_off, tmp_str, cnt);
	if (rc) {
		rc = -EIO;
		goto err;
	}
	append_loc->fe_sector = active->fe_sector;
	append_loc->fe_elem_off = active->fe_elem_off;
	append_loc->fe_data_off = active->fe_elem_off + cnt;

	active->fe_elem_off = append_loc->fe_data_off + len;

	k_mutex_unlock(&fcb->f_mtx);

	return 0;
err:
	k_mutex_unlock(&fcb->f_mtx);
	return rc;
}

int
fcb_append_finish(struct fcb *fcb, struct fcb_entry *loc)
{
	int rc;
	uint8_t em[fcb->f_align];
	off_t off;

	(void)memset(em, 0xFF, sizeof(em));

	rc = fcb_elem_endmarker(fcb, loc, &em[0]);
	if (rc) {
		return rc;
	}
	off = loc->fe_data_off + fcb_len_in_flash(fcb, loc->fe_data_len);

	rc = fcb_flash_write(fcb, loc->fe_sector, off, em, fcb->f_align);
	if (rc) {
		return -EIO;
	}
	return 0;
}
