/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <stdlib.h>

#include <fs/fcb.h>
#include "fcb_priv.h"
#include "string.h"
#include <errno.h>

u8_t
fcb_get_align(const struct fcb *fcb)
{
	u8_t align;

	if (fcb->fap == NULL) {
		return 0;
	}

	align = flash_area_align(fcb->fap);

	return align;
}

int fcb_flash_read(const struct fcb *fcb, const struct flash_sector *sector,
		   off_t off, void *dst, size_t len)
{
	int rc;

	if (off + len > sector->fs_size) {
		return -EINVAL;
	}

	if (fcb->fap == NULL) {
		return -EIO;
	}

	rc = flash_area_read(fcb->fap, sector->fs_off + off, dst, len);

	if (rc != 0) {
		return -EIO;
	}

	return 0;
}

int fcb_flash_write(const struct fcb *fcb, const struct flash_sector *sector,
		    off_t off, const void *src, size_t len)
{
	int rc;

	if (off + len > sector->fs_size) {
		return -EINVAL;
	}

	if (fcb->fap == NULL) {
		return -EIO;
	}

	rc = flash_area_write(fcb->fap, sector->fs_off + off, src, len);

	if (rc != 0) {
		return -EIO;
	}

	return 0;
}

int
fcb_erase_sector(const struct fcb *fcb, const struct flash_sector *sector)
{
	int rc;

	if (fcb->fap == NULL) {
		return -EIO;
	}

	rc = flash_area_erase(fcb->fap, sector->fs_off, sector->fs_size);

	if (rc != 0) {
		return -EIO;
	}

	return 0;
}

int
fcb_init(int f_area_id, struct fcb *fcb)
{
	struct flash_sector *sector;
	int rc;
	int i;
	u8_t align;
	int oldest = -1, newest = -1;
	struct flash_sector *oldest_sector = NULL, *newest_sector = NULL;
	struct fcb_disk_area fda;

	if (!fcb->f_sectors || fcb->f_sector_cnt - fcb->f_scratch_cnt < 1) {
		return -EINVAL;
	}

	rc = flash_area_open(f_area_id, &fcb->fap);
	if (rc != 0) {
		return -EINVAL;
	}

	align = fcb_get_align(fcb);
	if (align == 0U) {
		return -EINVAL;
	}

	/* Fill last used, first used */
	for (i = 0; i < fcb->f_sector_cnt; i++) {
		sector = &fcb->f_sectors[i];
		rc = fcb_sector_hdr_read(fcb, sector, &fda);
		if (rc < 0) {
			return rc;
		}
		if (rc == 0) {
			continue;
		}
		if (oldest < 0) {
			oldest = newest = fda.fd_id;
			oldest_sector = newest_sector = sector;
			continue;
		}
		if (FCB_ID_GT(fda.fd_id, newest)) {
			newest = fda.fd_id;
			newest_sector = sector;
		} else if (FCB_ID_GT(oldest, fda.fd_id)) {
			oldest = fda.fd_id;
			oldest_sector = sector;
		}
	}
	if (oldest < 0) {
		/*
		 * No initialized areas.
		 */
		oldest_sector = newest_sector = &fcb->f_sectors[0];
		rc = fcb_sector_hdr_init(fcb, oldest_sector, 0);
		if (rc) {
			return rc;
		}
		newest = oldest = 0;
	}
	fcb->f_align = align;
	fcb->f_oldest = oldest_sector;
	fcb->f_active.fe_sector = newest_sector;
	fcb->f_active.fe_elem_off = sizeof(struct fcb_disk_area);
	fcb->f_active_id = newest;

	while (1) {
		rc = fcb_getnext_in_sector(fcb, &fcb->f_active);
		if (rc == -ENOTSUP) {
			rc = 0;
			break;
		}
		if (rc != 0) {
			break;
		}
	}
	k_mutex_init(&fcb->f_mtx);
	return rc;
}

int
fcb_free_sector_cnt(struct fcb *fcb)
{
	int i;
	struct flash_sector *fa;

	fa = fcb->f_active.fe_sector;
	for (i = 0; i < fcb->f_sector_cnt; i++) {
		fa = fcb_getnext_sector(fcb, fa);
		if (fa == fcb->f_oldest) {
			break;
		}
	}
	return i;
}

int
fcb_is_empty(struct fcb *fcb)
{
	return (fcb->f_active.fe_sector == fcb->f_oldest &&
	  fcb->f_active.fe_elem_off == sizeof(struct fcb_disk_area));
}

/**
 * Length of an element is encoded in 1 or 2 bytes.
 * 1 byte for lengths < 128 bytes, and 2 bytes for < 16384.
 */
int
fcb_put_len(u8_t *buf, u16_t len)
{
	if (len < 0x80) {
		buf[0] = len;
		return 1;
	} else if (len < FCB_MAX_LEN) {
		buf[0] = (len & 0x7f) | 0x80;
		buf[1] = len >> 7;
		return 2;
	} else {
		return -EINVAL;
	}
}

int
fcb_get_len(u8_t *buf, u16_t *len)
{
	int rc;

	if (buf[0] & 0x80) {
		if (buf[0] == 0xff && buf[1] == 0xff) {
			return -ENOTSUP;
		}
		*len = (buf[0] & 0x7f) | (buf[1] << 7);
		rc = 2;
	} else {
		*len = buf[0];
		rc = 1;
	}
	return rc;
}

/**
 * Initialize erased sector for use.
 */
int
fcb_sector_hdr_init(struct fcb *fcb, struct flash_sector *sector, u16_t id)
{
	struct fcb_disk_area fda;
	int rc;

	fda.fd_magic = fcb->f_magic;
	fda.fd_ver = fcb->f_version;
	fda._pad = 0xff;
	fda.fd_id = id;

	rc = fcb_flash_write(fcb, sector, 0, &fda, sizeof(fda));
	if (rc != 0) {
		return -EIO;
	}
	return 0;
}

/**
 * Checks whether FCB sector contains data or not.
 * Returns <0 in error.
 * Returns 0 if sector is unused;
 * Returns 1 if sector has data.
 */
int fcb_sector_hdr_read(struct fcb *fcb, struct flash_sector *sector,
			struct fcb_disk_area *fdap)
{
	struct fcb_disk_area fda;
	int rc;

	if (!fdap) {
		fdap = &fda;
	}
	rc = fcb_flash_read(fcb, sector, 0, fdap, sizeof(*fdap));
	if (rc) {
		return -EIO;
	}
	if (fdap->fd_magic == 0xffffffff) {
		return 0;
	}
	if (fdap->fd_magic != fcb->f_magic) {
		return -ENOMSG;
	}
	return 1;
}

/**
 * Finds the fcb entry that gives back upto n entries at the end.
 * @param0 ptr to fcb
 * @param1 n number of fcb entries the user wants to get
 * @param2 ptr to the fcb_entry to be returned
 * @return 0 on there are any fcbs aviable; -ENOENT otherwise
 */
int
fcb_offset_last_n(struct fcb *fcb, u8_t entries,
		struct fcb_entry *last_n_entry)
{
	struct fcb_entry loc;
	int i;

	/* assure a minimum amount of entries */
	if (!entries) {
		entries = 1U;
	}

	i = 0;
	(void)memset(&loc, 0, sizeof(loc));
	while (!fcb_getnext(fcb, &loc)) {
		if (i == 0) {
			/* Start from the beginning of fcb entries */
			*last_n_entry = loc;
		}
		/* Update last_n_entry after n entries and keep updating */
		else if (i > (entries - 1)) {
			fcb_getnext(fcb, last_n_entry);
		}
		i++;
	}

	return (i == 0) ? -ENOENT : 0;
}

/**
 * Clear fcb
 * @param fcb
 * @return 0 on success; non-zero on failure
 */
int
fcb_clear(struct fcb *fcb)
{
	int rc;

	rc = 0;
	while (!fcb_is_empty(fcb)) {
		rc = fcb_rotate(fcb);
		if (rc) {
			break;
		}
	}
	return rc;
}
