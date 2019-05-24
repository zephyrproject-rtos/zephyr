/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FCB_PRIV_H_
#define __FCB_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FCB_CRC_SZ	sizeof(u8_t)
#define FCB_TMP_BUF_SZ	32

#define FCB_ID_GT(a, b) (((int16_t)(a) - (int16_t)(b)) > 0)

struct fcb_disk_area {
	u32_t fd_magic;
	u8_t fd_ver;
	u8_t _pad;
	u16_t fd_id;
};

int fcb_put_len(u8_t *buf, u16_t len);
int fcb_get_len(u8_t *buf, u16_t *len);

static inline int fcb_len_in_flash(struct fcb *fcb, u16_t len)
{
	if (fcb->f_align <= 1U) {
		return len;
	}
	return (len + (fcb->f_align - 1U)) & ~(fcb->f_align - 1U);
}

const struct flash_area *fcb_open_flash(const struct fcb *fcb);
u8_t fcb_get_align(const struct fcb *fcb);
int fcb_erase_sector(const struct fcb *fcb, const struct flash_sector *sector);

int fcb_getnext_in_sector(struct fcb *fcb, struct fcb_entry *loc);
struct flash_sector *fcb_getnext_sector(struct fcb *fcb,
					struct flash_sector *sector);
int fcb_getnext_nolock(struct fcb *fcb, struct fcb_entry *loc);

int fcb_elem_info(struct fcb *fcb, struct fcb_entry *loc);
int fcb_elem_crc8(struct fcb *fcb, struct fcb_entry *loc, u8_t *crc8p);

int fcb_sector_hdr_init(struct fcb *fcb, struct flash_sector *sector, u16_t id);
int fcb_sector_hdr_read(struct fcb *fcb, struct flash_sector *sector,
			struct fcb_disk_area *fdap);

#ifdef __cplusplus
}
#endif

#endif /* __FCB_PRIV_H_ */
