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

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_pga_gain))

#define LEFT_ADC_PGA_GAIN  0x0F
#define RIGHT_ADC_PGA_GAIN 0x10

static int set_input_volume(audio_channel_t channel, int vol)
{
	audio_property_value_t val = {.vol = vol};

	return audio_codec_set_property(CODEC, AUDIO_PROPERTY_INPUT_VOLUME, channel, val);
}

ZTEST(tlv320_pga_gain, test_clamps_at_low_end)
{
	tlv320aic3104_emul_reset();
	zassert_ok(set_input_volume(AUDIO_CHANNEL_ALL, -50));
	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), 0x00, "clamped to 0 dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), 0x00, "clamped to 0 dB");
}

ZTEST(tlv320_pga_gain, test_clamps_at_high_end)
{
	tlv320aic3104_emul_reset();

	zassert_ok(set_input_volume(AUDIO_CHANNEL_ALL, 900));
	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), 0x77,
		      "clamped to 59.5 dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), 0x77,
		      "clamped to 59.5 dB");
}

ZTEST(tlv320_pga_gain, test_mid_value_per_channel)
{
	tlv320aic3104_emul_reset();

	zassert_ok(set_input_volume(AUDIO_CHANNEL_FRONT_LEFT, 300));
	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), 0x3C, "left 30 dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), 0x00, "right untouched");

	tlv320aic3104_emul_reset();
	zassert_ok(set_input_volume(AUDIO_CHANNEL_FRONT_RIGHT, 300));
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), 0x3C, "right 30 dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), 0x00, "left untouched");
}

ZTEST(tlv320_pga_gain, test_unsupported_channel_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(set_input_volume(AUDIO_CHANNEL_LFE, 300), -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, LEFT_ADC_PGA_GAIN), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, RIGHT_ADC_PGA_GAIN), 0x00,
		      "no write on rejection");
}

ZTEST_SUITE(tlv320_pga_gain, NULL, NULL, NULL, NULL, NULL);
