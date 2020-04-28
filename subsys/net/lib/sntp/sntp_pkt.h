/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SNTP_PKT_H
#define __SNTP_PKT_H

#include <zephyr/types.h>

#define SNTP_LI_MASK   0xC0
#define SNTP_VN_MASK   0x38
#define SNTP_MODE_MASK 0x07

#define SNTP_LI_SHIFT   6
#define SNTP_VN_SHIFT   3
#define SNTP_MODE_SHIFT 0

#define SNTP_GET_LI(x)    ((x & SNTP_LI_MASK) >> SNTP_LI_SHIFT)
#define SNTP_GET_VN(x)    ((x & SNTP_VN_MASK) >> SNTP_VN_SHIFT)
#define SNTP_GET_MODE(x)  ((x & SNTP_MODE_MASK) >> SNTP_MODE_SHIFT)

#define SNTP_SET_LI(x, v)   (x = x | (v << SNTP_LI_SHIFT))
#define SNTP_SET_VN(x, v)   (x = x | (v << SNTP_VN_SHIFT))
#define SNTP_SET_MODE(x, v) (x = x | (v << SNTP_MODE_SHIFT))

struct sntp_pkt {
	u8_t lvm;		/* li, vn, and mode in big endian fashion */
	u8_t stratum;
	u8_t poll;
	u8_t precision;
	u32_t root_delay;
	u32_t root_dispersion;
	u32_t ref_id;
	u32_t ref_tm_s;
	u32_t ref_tm_f;
	u32_t orig_tm_s;	/* Originate timestamp seconds */
	u32_t orig_tm_f;	/* Originate timsstamp seconds fraction */
	u32_t rx_tm_s;		/* Receive timestamp seconds */
	u32_t rx_tm_f;		/* Receive timestamp seconds fraction */
	u32_t tx_tm_s;		/* Transimit timestamp seconds */
	u32_t tx_tm_f;		/* Transimit timestamp seconds fraction */
} __packed;

#endif
