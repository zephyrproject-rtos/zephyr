/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_output_fail))

#define REG_LEFT_DAC_VOL 0x2BU

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
}

ZTEST_SUITE(tlv320_output_fail, NULL, NULL, before_each, NULL, NULL);

ZTEST(tlv320_output_fail, test_start_write_failure_propagates)
{
	tlv320aic3104_emul_fail_write_at(0, REG_LEFT_DAC_VOL);

	zassert_equal(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX), -EIO,
		      "playback start must report the failed write");
}

ZTEST(tlv320_output_fail, test_failed_start_leaves_output_stopped)
{
	audio_property_value_t vol = {.vol = -25};

	tlv320aic3104_emul_fail_write_at(0, REG_LEFT_DAC_VOL);
	zassert_equal(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX), -EIO);

	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));
	zassert_equal(tlv320aic3104_emul_last_val(0, REG_LEFT_DAC_VOL), 0x00U,
		      "volume must not reach an output that failed to start");
}

ZTEST(tlv320_output_fail, test_stop_write_failure_propagates)
{
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));
	tlv320aic3104_emul_fail_write_at(0, REG_LEFT_DAC_VOL);

	zassert_equal(audio_codec_stop(CODEC, AUDIO_DAI_DIR_TX), -EIO,
		      "playback stop must report the failed write");
}

ZTEST(tlv320_output_fail, test_mute_write_failure_propagates)
{
	audio_property_value_t mute = {.mute = true};

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));
	tlv320aic3104_emul_fail_write_at(0, REG_LEFT_DAC_VOL);

	zassert_equal(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					       mute),
		      -EIO, "mute must report the failed write");
}

ZTEST(tlv320_output_fail, test_volume_write_failure_propagates)
{
	audio_property_value_t vol = {.vol = -25};

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_TX));
	tlv320aic3104_emul_fail_write_at(0, REG_LEFT_DAC_VOL);

	zassert_equal(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_ALL, vol),
		      -EIO, "volume must report the failed write");
}
