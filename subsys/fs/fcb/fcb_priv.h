/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FCB_PRIV_H_
#define __FCB_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FCB_CRC_SZ	sizeof(uint8_t)
#define FCB_TMP_BUF_SZ	32

#define FCB_ID_GT(a, b) (((int16_t)(a) - (int16_t)(b)) > 0)

#define MK32(val) ((((uint32_t)(val)) << 24) |			\
		   (((uint32_t)((val) & 0xff)) << 16) |		\
		   (((uint32_t)((val) & 0xff)) << 8) |		\
		   (((uint32_t)((val) & 0xff))))


/* @brief Gets magic value in flash formatted version
 *
 * Magic, the fcb->f_magic, prior to being written to flash, is xored with
 * binary inversion of fcb->f_erase_value to avoid it matching a 4 consecutive
 * bytes of flash erase value, which is used to recognize end of records,
 * by accident. Only the value of 0xFFFFFFFF will be always written as
 * 4 bytes of erase value.
 *
 * @param fcb pointer to initialized fcb structure
 *
 * @return uin32_t formatted magic value
 */
static inline uint32_t fcb_flash_magic(const struct fcb *fcb)
{
	const uint8_t ev = fcb->f_erase_value;

	return (fcb->f_magic ^ ~MK32(ev));
}

struct fcb_disk_area {
	uint32_t fd_magic;
	uint8_t fd_ver;
	uint8_t _pad;
	uint16_t fd_id;
};

int fcb_put_len(const struct fcb *fcb, uint8_t *buf, uint16_t len);
int fcb_get_len(const struct fcb *fcb, uint8_t *buf, uint16_t *len);

static inline int fcb_len_in_flash(struct fcb *fcb, uint16_t len)
{
	if (fcb->f_align <= 1U) {
		return len;
	}
	return (len + (fcb->f_align - 1U)) & ~(fcb->f_align - 1U);
}

const struct flash_area *fcb_open_flash(const struct fcb *fcb);
uint8_t fcb_get_align(const struct fcb *fcb);
int fcb_erase_sector(const struct fcb *fcb, const struct flash_sector *sector);

int fcb_getnext_in_sector(struct fcb *fcb, struct fcb_entry *loc);
struct flash_sector *fcb_getnext_sector(struct fcb *fcb,
					struct flash_sector *sector);
int fcb_getnext_nolock(struct fcb *fcb, struct fcb_entry *loc);

int fcb_elem_info(struct fcb *fcb, struct fcb_entry *loc);
int fcb_elem_endmarker(struct fcb *fcb, struct fcb_entry *loc, uint8_t *crc8p);

int fcb_sector_hdr_init(struct fcb *fcb, struct flash_sector *sector, uint16_t id);
int fcb_sector_hdr_read(struct fcb *fcb, struct flash_sector *sector,
			struct fcb_disk_area *fdap);

#ifdef __cplusplus
}
#endif

#endif /* __FCB_PRIV_H_ */
