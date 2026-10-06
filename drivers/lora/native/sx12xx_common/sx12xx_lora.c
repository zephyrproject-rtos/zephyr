/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>

#include "sx12xx_lora.h"

uint32_t sx12xx_lora_airtime_ms(uint32_t bw_hz, uint8_t sf, uint8_t cr, uint32_t preamble_len,
				bool ldro, bool crc, uint32_t payload_len)
{
	uint64_t t_sym_us, t_preamble_us, t_payload_us;
	uint32_t n_payload;
	int32_t tmp;
	int32_t de = ldro ? 1 : 0;

	/* Symbol time = 2^SF / BW */
	t_sym_us = ((uint64_t)BIT(sf) * USEC_PER_SEC) / bw_hz;

	/* Preamble time (4.25 extra symbols) */
	t_preamble_us = (uint64_t)(preamble_len + 4U) * t_sym_us + (t_sym_us / 4U);

	/* Payload symbol count */
	tmp = 8 * (int32_t)payload_len - 4 * sf + 28 + (crc ? 16 : 0);
	if (tmp < 0) {
		tmp = 0;
	}

	n_payload = 8U + (uint32_t)(((tmp + 4 * (sf - 2 * de) - 1) / (4 * (sf - 2 * de))) *
				    (cr + 4));
	t_payload_us = n_payload * t_sym_us;

	return (uint32_t)((t_preamble_us + t_payload_us + 500U) / USEC_PER_MSEC);
}

uint32_t sx12xx_gfsk_airtime_ms(const struct lora_modem_config_gfsk *cfg, uint32_t data_len,
				uint32_t header_bits)
{
	uint32_t payload = cfg->fixed_len ? cfg->payload_len : data_len;
	uint64_t bits;

	bits = (uint64_t)(cfg->preamble_len + cfg->sync_word_len + payload +
			  (cfg->packet_crc_disable ? 0U : 2U)) *
	       BITS_PER_BYTE;
	if (!cfg->fixed_len) {
		bits += header_bits;
	}

	return (uint32_t)DIV_ROUND_UP(bits * MSEC_PER_SEC, cfg->bitrate);
}
