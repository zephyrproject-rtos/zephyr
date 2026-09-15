/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_CLOCK_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_CLOCK_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

	bool pll_enabled;

	uint8_t r2_ndac_nadc;

	uint8_t r3_pll_enable_q_p;

	uint8_t r4_pll_j;

	uint8_t r5_pll_d_msb;

	uint8_t r6_pll_d_lsb;

	uint8_t r7_fsref_family;

	uint8_t r11_pll_r;

	uint8_t r101_codec_clkin_src;
} tlv320aic3104_clock_solution;

int tlv320aic3104_clock_solve(uint32_t mclk_hz, uint32_t sample_rate_hz,
			      tlv320aic3104_clock_solution *out);

#ifdef __cplusplus
}
#endif

#endif
