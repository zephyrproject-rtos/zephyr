/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb.h"
#include "fcb_priv.h"

int
fcb_rotate(struct fcb *fcb)
{
	struct flash_area *fap;
	int rc = 0;

	rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
	if (rc) {
		return FCB_ERR_ARGS;
	}

	rc = flash_area_erase(fcb->f_oldest, 0, fcb->f_oldest->fa_size);
	if (rc) {
		rc = FCB_ERR_FLASH;
		goto out;
	}
	if (fcb->f_oldest == fcb->f_active.fe_area) {
		/*
		 * Need to create a new active area, as we're wiping
		 * the current.
		 */
		fap = fcb_getnext_area(fcb, fcb->f_oldest);
		rc = fcb_sector_hdr_init(fcb, fap, fcb->f_active_id + 1);
		if (rc) {
			goto out;
		}
		fcb->f_active.fe_area = fap;
		fcb->f_active.fe_elem_off = sizeof(struct fcb_disk_area);
		fcb->f_active_id++;
	}
	fcb->f_oldest = fcb_getnext_area(fcb, fcb->f_oldest);
out:
	k_mutex_unlock(&fcb->f_mtx);
	return rc;
}
