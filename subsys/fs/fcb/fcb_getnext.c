/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <fs/fcb.h>
#include "fcb_priv.h"

int
fcb_getnext_in_sector(struct fcb *fcb, struct fcb_entry *loc)
{
	int rc;

	rc = fcb_elem_info(fcb, loc);
	if (rc == 0 || rc == -EBADMSG) {
		do {
			loc->fe_elem_off = loc->fe_data_off +
			  fcb_len_in_flash(fcb, loc->fe_data_len) +
			  fcb_len_in_flash(fcb, FCB_CRC_SZ);
			rc = fcb_elem_info(fcb, loc);
			if (rc != -EBADMSG) {
				break;
			}
		} while (rc == -EBADMSG);
	}
	return rc;
}

struct flash_sector *
fcb_getnext_sector(struct fcb *fcb, struct flash_sector *sector)
{
	sector++;
	if (sector >= &fcb->f_sectors[fcb->f_sector_cnt]) {
		sector = &fcb->f_sectors[0];
	}
	return sector;
}

int
fcb_getnext_nolock(struct fcb *fcb, struct fcb_entry *loc)
{
	int rc;

	if (loc->fe_sector == NULL) {
		/*
		 * Find the first one we have in flash.
		 */
		loc->fe_sector = fcb->f_oldest;
	}
	if (loc->fe_elem_off == 0U) {
		/*
		 * If offset is zero, we serve the first entry from the sector.
		 */
		loc->fe_elem_off = sizeof(struct fcb_disk_area);
		rc = fcb_elem_info(fcb, loc);
		switch (rc) {
		case 0:
			return 0;
		case -EBADMSG:
			break;
		default:
			goto next_sector;
		}
	} else {
		rc = fcb_getnext_in_sector(fcb, loc);
		if (rc == 0) {
			return 0;
		}
		if (rc == -ENOTSUP) {
			goto next_sector;
		}
	}
	while (rc == -EBADMSG) {
		rc = fcb_getnext_in_sector(fcb, loc);
		if (rc == 0) {
			return 0;
		}

		if (rc != -EBADMSG) {
			/*
			 * Moving to next sector.
			 */
next_sector:
			if (loc->fe_sector == fcb->f_active.fe_sector) {
				return -ENOTSUP;
			}
			loc->fe_sector = fcb_getnext_sector(fcb, loc->fe_sector);
			loc->fe_elem_off = sizeof(struct fcb_disk_area);
			rc = fcb_elem_info(fcb, loc);
			switch (rc) {
			case 0:
				return 0;
			case -EBADMSG:
				break;
			default:
				goto next_sector;
			}
		}
	}

	return 0;
}

int
fcb_getnext(struct fcb *fcb, struct fcb_entry *loc)
{
	int rc;

	rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
	if (rc) {
		return -EINVAL;
	}
	rc = fcb_getnext_nolock(fcb, loc);
	k_mutex_unlock(&fcb->f_mtx);

	return rc;
}
