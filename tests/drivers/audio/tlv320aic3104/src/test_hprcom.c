/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_hprcom))

#define R_HP_OUTPUT_DRIVER_CTRL 0x26
#define R_HPRCOM_LEVEL          0x48

#define HP_OUTPUT_DRIVER_CTRL_EXPECT 0x0E

#define HPRCOM_LEVEL_EXPECT 0x09

static void configure_codec(void)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
	};
	zassert_ok(audio_codec_configure(CODEC, &cfg));
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	tlv320aic3104_emul_reset();
	configure_codec();
}

ZTEST_SUITE(tlv320_hprcom, NULL, NULL, before_each, NULL, NULL);

ZTEST(tlv320_hprcom, test_configure_drives_hprcom_as_vcm)
{
	zassert_equal(tlv320aic3104_emul_last_val(0, R_HP_OUTPUT_DRIVER_CTRL),
		      HP_OUTPUT_DRIVER_CTRL_EXPECT,
		      "R38 must carry the VCM mode and keep the protection bits");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_HPRCOM_LEVEL), HPRCOM_LEVEL_EXPECT,
		      "R72 must be powered and unmuted at 0 dB");
}

ZTEST(tlv320_hprcom, test_output_mute_leaves_hprcom_driven)
{
	audio_property_value_t on = {.mute = true};

	audio_codec_start_output(CODEC);
	zassert_ok(
		audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, on));

	zassert_equal(tlv320aic3104_emul_last_val(0, R_HPRCOM_LEVEL), HPRCOM_LEVEL_EXPECT,
		      "R72 must not follow the output mute");
}

ZTEST(tlv320_hprcom, test_reconfigure_is_idempotent)
{
	configure_codec();

	zassert_equal(tlv320aic3104_emul_last_val(0, R_HP_OUTPUT_DRIVER_CTRL),
		      HP_OUTPUT_DRIVER_CTRL_EXPECT, "R38 unchanged across a reconfigure");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_HPRCOM_LEVEL), HPRCOM_LEVEL_EXPECT,
		      "R72 unchanged across a reconfigure");
}
