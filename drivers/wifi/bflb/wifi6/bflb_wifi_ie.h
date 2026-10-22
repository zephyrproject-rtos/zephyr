/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_IE_H_
#define BFLB_WIFI_IE_H_

#include <stdint.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#define WLAN_EID_RSN    48
#define WLAN_EID_VENDOR 221

#define IEEE80211_TLV_HDR_LEN 2U
#define MAX_CACHED_RSN_IE_LEN 48
#define MAX_CACHED_RSNXE_LEN  8
#define WLAN_EID_RSNXE        244

static const uint8_t wpa_oui_header[] = {0x00, 0x50, 0xF2, 0x01};

static inline uint8_t *emit_le16(uint8_t *pos, uint16_t val)
{
	sys_put_le16(val, pos);
	return pos + 2;
}

static inline uint8_t *emit_be32(uint8_t *pos, uint32_t val)
{
	sys_put_be32(val, pos);
	return pos + 4;
}

static inline uint8_t *emit_buf(uint8_t *pos, const void *src, size_t len)
{
	memcpy(pos, src, len);
	return pos + len;
}

static inline uint8_t *put_cipher_suite_list(uint8_t *pos, uint32_t group, uint32_t pairwise,
					     uint32_t akm)
{
	pos = emit_le16(pos, 1); /* version */
	pos = emit_be32(pos, group);
	pos = emit_le16(pos, 1); /* pairwise count */
	pos = emit_be32(pos, pairwise);
	pos = emit_le16(pos, 1); /* AKM count */
	pos = emit_be32(pos, akm);
	return pos;
}

#endif /* BFLB_WIFI_IE_H_ */
