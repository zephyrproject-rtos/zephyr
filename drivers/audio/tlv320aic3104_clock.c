/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tlv320aic3104_clock.h"
#include "tlv320aic3104_regs.h"

#include <errno.h>

#define MCLK_MIN_HZ 512000u
#define MCLK_MAX_HZ 50000000u

#define FSREF_MAX_HZ 53000u

#define FSREF_FRACTIONAL_NCODEC_MIN_HZ 39000u

#define NCODEC_X2_MIN 2
#define NCODEC_X2_MAX 12

#define DIVIDER_Q_MIN 2
#define DIVIDER_Q_MAX 17

#define PLL_P_MIN      1
#define PLL_P_MAX      8
#define PLL_R_MIN      1
#define PLL_R_MAX      16
#define PLL_J_MIN      1
#define PLL_J_MAX      63

#define PLL_J_PERF_MIN 4
#define PLL_D_MAX      9999
#define PLL_K10000_MIN (PLL_J_MIN * 10000)
#define PLL_K10000_MAX (PLL_J_MAX * 10000 + PLL_D_MAX)

#define PLL_INPUT_OVER_P_MIN_D0_HZ  512000u
#define PLL_INPUT_OVER_P_MIN_DNZ_HZ 10000000u
#define PLL_INPUT_OVER_P_MAX_HZ     20000000u
#define PLL_VCO_MIN_HZ              80000000u
#define PLL_VCO_MAX_HZ              110000000u
#define PLL_J_MAX_D0                55
#define PLL_J_MAX_DNZ               11

static uint8_t encode_ncodec(int ncodec_x2)
{
	return (uint8_t)(ncodec_x2 - NCODEC_X2_MIN);
}

static uint8_t encode_q(int q)
{
	if (q == 16) {
		return 0;
	}
	if (q == 17) {
		return 1;
	}
	return (uint8_t)q;
}

static uint8_t encode_r(int r)
{
	return (r == 16) ? 0 : (uint8_t)r;
}

static uint8_t encode_p(int p)
{
	return (p == 8) ? 0 : (uint8_t)p;
}

static void fill_ncodec_and_fsref_bit(tlv320aic3104_clock_solution *out, int ncodec_x2,
				      int64_t fsref_x2)
{
	uint8_t code = encode_ncodec(ncodec_x2);

	out->r2_ndac_nadc = (uint8_t)((code << 4) | code);

	int64_t d_44100 = fsref_x2 - (int64_t)44100 * 2;
	int64_t d_48000 = fsref_x2 - (int64_t)48000 * 2;

	if (d_44100 < 0) {
		d_44100 = -d_44100;
	}
	if (d_48000 < 0) {
		d_48000 = -d_48000;
	}
	out->r7_fsref_family = (d_44100 <= d_48000) ? CODEC_DATAPATH_FSREF_FAMILY_BIT : 0;
}

static bool solve_divider(uint32_t mclk_hz, int ncodec_x2, int64_t fsref_x2,
			  tlv320aic3104_clock_solution *out)
{
	int64_t denom = (int64_t)128 * fsref_x2;

	if (denom == 0 || ((int64_t)mclk_hz * 2) % denom != 0) {
		return false;
	}

	int64_t q = ((int64_t)mclk_hz * 2) / denom;

	if (q < DIVIDER_Q_MIN || q > DIVIDER_Q_MAX) {
		return false;
	}
	if ((ncodec_x2 % 2) != 0) {
		if ((q % 2) != 0) {
			return false;
		}
		if (fsref_x2 < (int64_t)FSREF_FRACTIONAL_NCODEC_MIN_HZ * 2) {
			return false;
		}
	}

	fill_ncodec_and_fsref_bit(out, ncodec_x2, fsref_x2);
	out->pll_enabled = false;
	out->r3_pll_enable_q_p = (uint8_t)(encode_q((int)q) << 3);
	out->r4_pll_j = 0;
	out->r5_pll_d_msb = 0;
	out->r6_pll_d_lsb = 0;
	out->r11_pll_r = 0;
	out->r101_codec_clkin_src = R101_CODEC_CLKIN_CLKDIV_OUT;
	return true;
}

static bool solve_pll(uint32_t mclk_hz, int ncodec_x2, int64_t fsref_x2,
		      tlv320aic3104_clock_solution *out)
{
	for (int p = PLL_P_MIN; p <= PLL_P_MAX; p++) {
		for (int r = PLL_R_MIN; r <= PLL_R_MAX; r++) {
			int64_t num = fsref_x2 * 1024 * p * 10000;
			int64_t den = (int64_t)mclk_hz * r;

			if (den == 0 || num % den != 0) {
				continue;
			}

			int64_t k10000 = num / den;

			if (k10000 < PLL_K10000_MIN || k10000 > PLL_K10000_MAX) {
				continue;
			}

			int64_t j = k10000 / 10000;
			int64_t d = k10000 % 10000;

			if (d == 0) {
				if (mclk_hz < PLL_INPUT_OVER_P_MIN_D0_HZ * (uint32_t)p ||
				    mclk_hz > PLL_INPUT_OVER_P_MAX_HZ * (uint32_t)p) {
					continue;
				}
				if (j < PLL_J_PERF_MIN || j > PLL_J_MAX_D0) {
					continue;
				}
			} else {
				if (mclk_hz < PLL_INPUT_OVER_P_MIN_DNZ_HZ * (uint32_t)p ||
				    mclk_hz > PLL_INPUT_OVER_P_MAX_HZ * (uint32_t)p) {
					continue;
				}
				if (j < PLL_J_PERF_MIN || j > PLL_J_MAX_DNZ || r != 1) {
					continue;
				}
			}

			int64_t vco_x10000_over_p = (int64_t)mclk_hz * k10000 * r;
			int64_t vco_min = (int64_t)PLL_VCO_MIN_HZ * 10000 * p;
			int64_t vco_max = (int64_t)PLL_VCO_MAX_HZ * 10000 * p;

			if (vco_x10000_over_p < vco_min || vco_x10000_over_p > vco_max) {
				continue;
			}

			fill_ncodec_and_fsref_bit(out, ncodec_x2, fsref_x2);
			out->pll_enabled = true;
			out->r3_pll_enable_q_p = (uint8_t)(R3_PLL_ENABLE_BIT | (encode_p(p)));
			out->r4_pll_j = (uint8_t)((j & 0x3F) << 2);
			out->r5_pll_d_msb = (uint8_t)((d >> 6) & 0xFF);
			out->r6_pll_d_lsb = (uint8_t)((d & 0x3F) << 2);
			out->r11_pll_r = encode_r(r);
			out->r101_codec_clkin_src = R101_CODEC_CLKIN_PLLDIV_OUT;
			return true;
		}
	}
	return false;
}

int tlv320aic3104_clock_solve(uint32_t mclk_hz, uint32_t sample_rate_hz,
			      tlv320aic3104_clock_solution *out)
{
	if (mclk_hz == 0 || sample_rate_hz == 0) {
		return -EINVAL;
	}
	if (mclk_hz < MCLK_MIN_HZ || mclk_hz > MCLK_MAX_HZ) {
		return -ENOTSUP;
	}

	for (int ncodec_x2 = NCODEC_X2_MIN; ncodec_x2 <= NCODEC_X2_MAX; ncodec_x2++) {
		int64_t fsref_x2 = (int64_t)sample_rate_hz * ncodec_x2;

		if (fsref_x2 > (int64_t)FSREF_MAX_HZ * 2) {
			continue;
		}
		if (solve_divider(mclk_hz, ncodec_x2, fsref_x2, out)) {
			return 0;
		}
	}

	for (int ncodec_x2 = NCODEC_X2_MIN; ncodec_x2 <= NCODEC_X2_MAX; ncodec_x2++) {
		int64_t fsref_x2 = (int64_t)sample_rate_hz * ncodec_x2;

		if (fsref_x2 > (int64_t)FSREF_MAX_HZ * 2) {
			continue;
		}
		if (solve_pll(mclk_hz, ncodec_x2, fsref_x2, out)) {
			return 0;
		}
	}

	return -ENOTSUP;
}
