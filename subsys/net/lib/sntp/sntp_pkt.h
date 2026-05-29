/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SNTP_PKT_H
#define __SNTP_PKT_H

#include <zephyr/types.h>

struct sntp_pkt {
#if defined(CONFIG_LITTLE_ENDIAN)
	uint8_t mode: 3;
	uint8_t vn: 3;
	uint8_t li: 2;
#else
	uint8_t li: 2;
	uint8_t vn: 3;
	uint8_t mode: 3;
#endif /* CONFIG_LITTLE_ENDIAN */
	uint8_t stratum;
	uint8_t poll;
	int8_t precision;
	uint32_t root_delay;
	uint32_t root_dispersion;
	uint32_t ref_id;
	uint32_t ref_tm_s;
	uint32_t ref_tm_f;
	uint32_t orig_tm_s;	/* Originate timestamp seconds */
	uint32_t orig_tm_f;	/* Originate timestamp seconds fraction */
	uint32_t rx_tm_s;		/* Receive timestamp seconds */
	uint32_t rx_tm_f;		/* Receive timestamp seconds fraction */
	uint32_t tx_tm_s;		/* Transmit timestamp seconds */
	uint32_t tx_tm_f;		/* Transmit timestamp seconds fraction */
} __packed;

#endif
