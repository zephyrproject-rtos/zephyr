/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/ztest.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_read_path))

#define CHIP_DB_MIN (-117)
#define CHIP_DB_MAX 0

static void before_each(void *f)
{
	ARG_UNUSED(f);
	tlv320aic3104_emul_reset();

	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
	};
	zassert_ok(audio_codec_configure(CODEC, &cfg));
}

ZTEST(tlv320_read_path, test_set_property_unsupported_channel)
{
	audio_property_value_t val = {.vol = 0};

	zassert_equal(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       (audio_channel_t)0, val),
		      -EINVAL, "non-ALL channel must return -EINVAL");
}

ZTEST(tlv320_read_path, test_set_property_unknown_property)
{
	audio_property_value_t val = {.vol = 0};

	zassert_equal(
		audio_codec_set_property(CODEC, (audio_property_t)0xFF, AUDIO_CHANNEL_ALL, val),
		-EINVAL, "unknown property must return -EINVAL");
}

ZTEST(tlv320_read_path, test_set_output_volume_out_of_range)
{
	audio_property_value_t val_above = {.vol = CHIP_DB_MAX + 1};
	audio_property_value_t val_below = {.vol = CHIP_DB_MIN - 1};

	zassert_equal(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_ALL, val_above),
		      -EINVAL, "vol above max must return -EINVAL");
	zassert_equal(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_ALL, val_below),
		      -EINVAL, "vol below min must return -EINVAL");
}

ZTEST_SUITE(tlv320_read_path, NULL, NULL, before_each, NULL, NULL);
