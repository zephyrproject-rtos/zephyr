/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "tlv320aic3104_in.h"

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_route_input))

#define MIC2_LINE2_TO_LADC  0x11
#define MIC2_LINE2_TO_RADC  0x12
#define LINE1L_TO_LADC_CTRL 0x13
#define LINE1R_TO_RADC_CTRL 0x16

ZTEST(tlv320_route_input, test_line2_front_left)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_LEFT,
					   TLV320AIC3104_INPUT_LINE2));
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_LADC), 0x0F, "LINE2L->LADC 0dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x78,
		      "LINE1L disconnected");
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_RADC), 0x00, "right untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x00, "right untouched");
}

ZTEST(tlv320_route_input, test_line1_front_right)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_RIGHT,
					   TLV320AIC3104_INPUT_LINE1));
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_RADC), 0xFF,
		      "LINE2 fully disconnected");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x00,
		      "LINE1R connected 0dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_LADC), 0x00, "left untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x00, "left untouched");
}

ZTEST(tlv320_route_input, test_line2_all)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_INPUT_LINE2));
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_LADC), 0x0F, "LINE2L->LADC 0dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_RADC), 0xF0, "LINE2R->RADC 0dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x78,
		      "LINE1L disconnected");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x78,
		      "LINE1R disconnected");
}

ZTEST(tlv320_route_input, test_line1_all)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_INPUT_LINE1));
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_LADC), 0xFF,
		      "LINE2 disconnected");
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_RADC), 0xFF,
		      "LINE2 disconnected");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x00,
		      "LINE1L connected 0dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x00,
		      "LINE1R connected 0dB");
}

ZTEST(tlv320_route_input, test_unsupported_channel_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(audio_codec_route_input(CODEC, AUDIO_CHANNEL_LFE, TLV320AIC3104_INPUT_LINE2),
		      -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_LADC), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_RADC), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x00,
		      "no write on rejection");
}

ZTEST(tlv320_route_input, test_unsupported_input_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(audio_codec_route_input(CODEC, AUDIO_CHANNEL_FRONT_LEFT, 99), -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, MIC2_LINE2_TO_LADC), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x00,
		      "no write on rejection");
}

ZTEST(tlv320_route_input, test_start_after_route_keeps_line1_connected)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
	};

	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_ok(audio_codec_route_input(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_INPUT_LINE1));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x04,
		      "LINE1L still connected after start");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x04,
		      "LINE1R still connected after start");
}

ZTEST(tlv320_route_input, test_start_without_route_leaves_line1_disconnected)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
	};

	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1L_TO_LADC_CTRL), 0x7C,
		      "LINE1L disconnected, channel powered");
	zassert_equal(tlv320aic3104_emul_last_val(0, LINE1R_TO_RADC_CTRL), 0x7C,
		      "LINE1R disconnected, channel powered");
}

ZTEST_SUITE(tlv320_route_input, NULL, NULL, NULL, NULL, NULL);
