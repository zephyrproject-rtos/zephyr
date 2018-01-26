/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb.h"
#include "fcb_priv.h"

/*
 * Call 'cb' for every element in flash circular buffer. If sector is specified,
 * only elements with that flash_sector are reported.
 */
int
fcb_walk(struct fcb *fcb, struct flash_sector *sector, fcb_walk_cb cb,
         void *cb_arg)
{
	struct fcb_entry loc;
	int rc;

	loc.fe_sector = sector;
	loc.fe_elem_off = 0;

	rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
	if (rc) {
		return FCB_ERR_ARGS;
	}
	while ((rc = fcb_getnext_nolock(fcb, &loc)) != FCB_ERR_NOVAR) {
		k_mutex_unlock(&fcb->f_mtx);
		if (sector && loc.fe_sector != sector) {
			return 0;
		}
		rc = cb(&loc, cb_arg);
		if (rc) {
			return rc;
		}
		k_mutex_lock(&fcb->f_mtx, K_FOREVER);
	}
	k_mutex_unlock(&fcb->f_mtx);
	return 0;
}
