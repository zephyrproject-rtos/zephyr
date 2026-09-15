/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_dai_configure))

#define R_ASI_CTRL_A 0x08
#define R_ASI_CTRL_B 0x09

#define ASI_CTRL_A_HIZ_DOUT    0x20
#define ASI_CTRL_A_BCLK_MASTER 0x80
#define ASI_CTRL_A_WCLK_MASTER 0x40

static struct audio_codec_cfg base_cfg(void)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
	};

	return cfg;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	tlv320aic3104_emul_reset();
}

ZTEST_SUITE(tlv320_dai_configure, NULL, NULL, before_each, NULL, NULL);

ZTEST(tlv320_dai_configure, test_mode_i2s)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x00, "I2S, 16-bit -> 00_00");
}

ZTEST(tlv320_dai_configure, test_mode_left_justified)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_type = AUDIO_DAI_TYPE_LEFT_JUSTIFIED;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0xC0,
		      "left justified -> 11_00");
}

ZTEST(tlv320_dai_configure, test_mode_right_justified)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_type = AUDIO_DAI_TYPE_RIGHT_JUSTIFIED;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x80,
		      "right justified -> 10_00");
}

ZTEST(tlv320_dai_configure, test_mode_dsp)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_type = AUDIO_DAI_TYPE_PCM;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x40, "DSP mode -> 01_00");
}

ZTEST(tlv320_dai_configure, test_unsupported_mode_rejected)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_type = AUDIO_DAI_TYPE_PCMA;
	zassert_equal(audio_codec_configure(CODEC, &cfg), -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_A), 0x00, "R8 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x00, "R9 untouched");
}

ZTEST(tlv320_dai_configure, test_word_length_16)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_cfg.i2s.word_size = 16;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x00, "16-bit -> 00_00_00");
}

ZTEST(tlv320_dai_configure, test_word_length_20)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_cfg.i2s.word_size = 20;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x10, "20-bit -> 00_01_00");
}

ZTEST(tlv320_dai_configure, test_word_length_24)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_cfg.i2s.word_size = 24;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x20, "24-bit -> 00_10_00");
}

ZTEST(tlv320_dai_configure, test_word_length_32)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_cfg.i2s.word_size = 32;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x30, "32-bit -> 00_11_00");
}

ZTEST(tlv320_dai_configure, test_unsupported_word_length_rejected)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_cfg.i2s.word_size = 18;
	zassert_equal(audio_codec_configure(CODEC, &cfg), -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_A), 0x00, "R8 untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_B), 0x00, "R9 untouched");
}

ZTEST(tlv320_dai_configure, test_clock_direction_slave)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_cfg.i2s.options = 0;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_A), ASI_CTRL_A_HIZ_DOUT,
		      "slave BCLK+WCLK -> D7=0, D6=0, explicit");
}

ZTEST(tlv320_dai_configure, test_clock_direction_master)
{
	struct audio_codec_cfg cfg = base_cfg();

	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_ASI_CTRL_A),
		      ASI_CTRL_A_HIZ_DOUT | ASI_CTRL_A_BCLK_MASTER | ASI_CTRL_A_WCLK_MASTER,
		      "master BCLK+WCLK -> D7=1, D6=1, explicit");
}
