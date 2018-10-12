/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_FCB_H_
#define ZEPHYR_INCLUDE_FCB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flash circular buffer.
 */
#include <inttypes.h>
#include <limits.h>

#include "flash_map.h"

#include <kernel.h>

#define FCB_MAX_LEN	(CHAR_MAX | CHAR_MAX << 7) /* Max length of element */

/*
 * Entry location is pointer to area (within fcb->f_sectors), and offset
 * within that area.
 */
struct fcb_entry {
	struct flash_sector *fe_sector;	/* ptr to sector */
					/* within fcb->f_sectors */
	u32_t fe_elem_off;		/* start of entry */
	u32_t fe_data_off;		/* start of data */
	u16_t fe_data_len;		/* size of data area */
};

/*
 * Helper macro for calculate the data offset related to
 * the fcb flash_area start offset.
 */
#define FCB_ENTRY_FA_DATA_OFF(entry) (entry.fe_sector->fs_off +\
				      entry.fe_data_off)

struct fcb_entry_ctx {
	struct fcb_entry loc;
	const struct flash_area *fap;
};

struct fcb {
	/* Caller of fcb_init fills this in */
	u32_t f_magic;		/* As placed on the disk */
	u8_t f_version;		/* Current version number of the data */
	u8_t f_sector_cnt;	/* Number of elements in sector array */
	u8_t f_scratch_cnt;	/* How many sectors should be kept empty */
	struct flash_sector *f_sectors; /* Array of sectors, */
					/* must be contiguous */

	/* Flash circular buffer internal state */
	struct k_mutex f_mtx;	/* Locking for accessing the FCB data */
	struct flash_sector *f_oldest;
	struct fcb_entry f_active;
	u16_t f_active_id;
	u8_t f_align;		/* writes to flash have to aligned to this */

	const struct flash_area *fap; /* Flash area used by the fcb instance */
				     /* This can be transfer to FCB user    */
};

/*
 * Error codes.
 */
#define FCB_OK		0
#define FCB_ERR_ARGS	-1
#define FCB_ERR_FLASH	-2
#define FCB_ERR_NOVAR   -3
#define FCB_ERR_NOSPACE	-4
#define FCB_ERR_NOMEM	-5
#define FCB_ERR_CRC	-6
#define FCB_ERR_MAGIC   -7

int fcb_init(int f_area_id, struct fcb *fcb);

/*
 * fcb_append() appends an entry to circular buffer. When writing the
 * contents for the entry, use loc->fl_area and loc->fl_data_off with
 * flash_area_write(). When you're finished, call fcb_append_finish() with
 * loc as argument.
 */
int fcb_append(struct fcb *fcb, u16_t len, struct fcb_entry *loc);
int fcb_append_finish(struct fcb *fcb, struct fcb_entry *append_loc);

/*
 * Walk over all log entries in FCB, or entries in a given flash_area.
 * cb gets called for every entry. If cb wants to stop the walk, it should
 * return non-zero value.
 *
 * Entry data can be read using flash_area_read(), using
 * loc->fe_area, loc->fe_data_off, and loc->fe_data_len as arguments.
 */
typedef int (*fcb_walk_cb)(struct fcb_entry_ctx *loc_ctx, void *arg);
int fcb_walk(struct fcb *fcb, struct flash_sector *sector, fcb_walk_cb cb,
	     void *cb_arg);
int fcb_getnext(struct fcb *fcb, struct fcb_entry *loc);

/*
 * Erases the data from oldest sector.
 */
int fcb_rotate(struct fcb *fcb);

/*
 * Start using the scratch block.
 */
int fcb_append_to_scratch(struct fcb *fcb);

/*
 * How many sectors are unused.
 */
int fcb_free_sector_cnt(struct fcb *fcb);

/*
 * Whether FCB has any data.
 */
int fcb_is_empty(struct fcb *fcb);

/*
 * Element at offset *entries* from last position (backwards).
 */
int fcb_offset_last_n(struct fcb *fcb, u8_t entries,
		      struct fcb_entry *last_n_entry);

/*
 * Clears FCB passed to it
 */
int fcb_clear(struct fcb *fcb);

int fcb_flash_read(const struct fcb *fcb, const struct flash_sector *sector,
		   off_t off, void *dst, size_t len);
int fcb_flash_write(const struct fcb *fcb, const struct flash_sector *sector,
		    off_t off, const void *src, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FCB_H_ */
