/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_clock_apply))

#define R_NDAC_NADC 0x02
#define R_PLL_A     0x03
#define R_PLL_B     0x04
#define R_PLL_C     0x05
#define R_PLL_D     0x06
#define R_DATAPATH  0x07
#define R_OVERFLOW  0x0B
#define R_CLOCK     0x65

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	tlv320aic3104_emul_reset();
}

ZTEST_SUITE(tlv320_clock_apply, NULL, NULL, before_each, NULL, NULL);

ZTEST(tlv320_clock_apply, test_divider_only_register_set)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
	};

	zassert_ok(audio_codec_configure(CODEC, &cfg));

	zassert_equal(tlv320aic3104_emul_last_val(0, R_NDAC_NADC), 0x00, "R2 NCODEC=1");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_A), 0x10, "R3 PLL off, Q=2");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_B), 0x00, "R4 unused, PLL off");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_C), 0x00, "R5 unused, PLL off");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_D), 0x00, "R6 unused, PLL off");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_OVERFLOW), 0x00,
		      "R11 PLL R unused, PLL off");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DATAPATH), 0x0A,
		      "R7 stereo datapath, 48 kHz family bit clear");

	zassert_equal(tlv320aic3104_emul_last_val(0, R_CLOCK), 0x01,
		      "R101 CODEC_CLKIN = CLKDIV_OUT when the PLL is off");
}

ZTEST(tlv320_clock_apply, test_pll_required_register_set)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12000000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 44100,
		.dai_cfg.i2s.word_size = 16,
	};

	zassert_ok(audio_codec_configure(CODEC, &cfg));

	zassert_equal(tlv320aic3104_emul_last_val(0, R_NDAC_NADC), 0x00, "R2 NCODEC=1");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_A), 0x81, "R3 PLL on, P=1");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_B), 0x1C, "R4 J=7");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_C), 0x52, "R5 D MSBs");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_D), 0x40, "R6 D LSBs");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_OVERFLOW), 0x01, "R11 PLL R=1");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DATAPATH), 0x8A,
		      "R7 stereo datapath, 44.1 kHz family bit set");

	zassert_equal(tlv320aic3104_emul_last_val(0, R_CLOCK), 0x00,
		      "R101 CODEC_CLKIN = PLLDIV_OUT when the PLL is on");
}

ZTEST(tlv320_clock_apply, test_unreachable_rate_writes_no_clock_register)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12000000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 44101,
		.dai_cfg.i2s.word_size = 16,
	};

	zassert_equal(audio_codec_configure(CODEC, &cfg), -ENOTSUP);

	zassert_equal(tlv320aic3104_emul_last_val(0, R_NDAC_NADC), 0x00, "R2 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_A), 0x00, "R3 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_B), 0x00, "R4 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_C), 0x00, "R5 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_PLL_D), 0x00, "R6 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_OVERFLOW), 0x00, "R11 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DATAPATH), 0x00,
		      "R7 untouched - configure() aborted before any write");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_CLOCK), 0x00, "R101 untouched");
}
