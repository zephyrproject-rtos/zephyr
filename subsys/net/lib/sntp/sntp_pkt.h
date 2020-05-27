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
	uint8_t lvm;		/* li, vn, and mode in big endian fashion */
	uint8_t stratum;
	uint8_t poll;
	uint8_t precision;
	uint32_t root_delay;
	uint32_t root_dispersion;
	uint32_t ref_id;
	uint32_t ref_tm_s;
	uint32_t ref_tm_f;
	uint32_t orig_tm_s;	/* Originate timestamp seconds */
	uint32_t orig_tm_f;	/* Originate timsstamp seconds fraction */
	uint32_t rx_tm_s;		/* Receive timestamp seconds */
	uint32_t rx_tm_f;		/* Receive timestamp seconds fraction */
	uint32_t tx_tm_s;		/* Transimit timestamp seconds */
	uint32_t tx_tm_f;		/* Transimit timestamp seconds fraction */
} __packed;

#endif
