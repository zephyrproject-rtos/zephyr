/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "tlv320aic3104_priv.h"

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_route_output))

#define DAC_L1_TO_HPLOUT_VOL    0x2FU
#define DAC_R1_TO_HPROUT_VOL    0x40U
#define DAC_L1_TO_LEFT_LOP_VOL  0x52U
#define DAC_R1_TO_RIGHT_LOP_VOL 0x5CU

static struct audio_codec_cfg s_cfg = {
	.mclk_freq = 12288000,
	.dai_type = AUDIO_DAI_TYPE_I2S,
	.dai_cfg.i2s.frame_clk_freq = 48000,
	.dai_cfg.i2s.word_size = 16,
	.dai_route = AUDIO_ROUTE_PLAYBACK,
};

static void before_each(void *fixture)
{
	audio_property_value_t unmute = {.mute = false};

	ARG_UNUSED(fixture);

	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute));
	zassert_ok(audio_codec_configure(CODEC, &s_cfg));
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
}

ZTEST(tlv320_route_output, test_hp_front_left)
{
	tlv320aic3104_emul_reset();
	zassert_ok(
		audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT, TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x80, "HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "LOP not routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x00,
		      "right untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x00,
		      "right untouched");
}

ZTEST(tlv320_route_output, test_lop_front_right)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_RIGHT,
					    TLV320AIC3104_OUTPUT_LOP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x80, "LOP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x00, "HP not routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x00, "left untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "left untouched");
}

ZTEST(tlv320_route_output, test_hp_all)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x80, "left HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x80,
		      "right HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "left LOP not routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x00,
		      "right LOP not routed");
}

ZTEST(tlv320_route_output, test_hp_front_center_routes_like_all)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_CENTER,
					    TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x80, "left HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x80,
		      "right HP routed");
}

ZTEST(tlv320_route_output, test_unsupported_output_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT, 99), -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "no write on rejection");
}

ZTEST(tlv320_route_output, test_unsupported_channel_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(audio_codec_route_output(CODEC, AUDIO_CHANNEL_LFE, TLV320AIC3104_OUTPUT_HP),
		      -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x00,
		      "no write on rejection");
}

ZTEST(tlv320_route_output, test_volume_follows_the_routed_terminal)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
	};
	audio_property_value_t vol = {.vol = -25};

	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_LOP));
	audio_codec_start_output(CODEC);
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));

	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x99,
		      "left line-out carries the set level");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x99,
		      "right line-out carries the set level");
}

ZTEST(tlv320_route_output, test_stop_leaves_deselected_terminal_unrouted)
{
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_LOP));
	audio_codec_start_output(CODEC);
	audio_codec_stop_output(CODEC);

	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x76,
		      "left HP stays unrouted");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x76,
		      "right HP stays unrouted");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0xF6,
		      "left line-out routed, muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0xF6,
		      "right line-out routed, muted");
}

ZTEST(tlv320_route_output, test_route_while_running_applies_the_set_level)
{
	audio_property_value_t vol = {.vol = -25};

	audio_codec_start_output(CODEC);
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_LOP));

	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x99,
		      "left line-out carries the set level");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x99,
		      "right line-out carries the set level");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x19,
		      "left HP unrouted");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x19,
		      "right HP unrouted");
}

ZTEST(tlv320_route_output, test_route_while_running_and_muted_stays_muted)
{
	audio_property_value_t vol = {.vol = -25};
	audio_property_value_t mute = {.mute = true};

	audio_codec_start_output(CODEC);
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute));
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_LOP));

	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0xF6,
		      "left line-out routed, muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0xF6,
		      "right line-out routed, muted");
}

ZTEST_SUITE(tlv320_route_output, NULL, NULL, before_each, NULL, NULL);
