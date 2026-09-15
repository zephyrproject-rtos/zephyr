/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_volume_restore))

#define R_LEFT_DAC_VOL            0x2B
#define R_RIGHT_DAC_VOL           0x2C
#define R_DAC_L1_TO_HPLOUT_VOL    0x2F
#define R_DAC_R1_TO_HPROUT_VOL    0x40

#define R_DAC_L1_TO_LEFT_LOP_VOL  0x52
#define R_DAC_R1_TO_RIGHT_LOP_VOL 0x5C

#define TEST_VOLUME_DB       (-25)
#define EXPECT_DAC_50PCT     0x19
#define EXPECT_ROUTING_50PCT 0x99

ZTEST(tlv320_volume_restore, test_start_output_restores_set_volume)
{
	audio_property_value_t val = {.vol = TEST_VOLUME_DB};

	zassert_true(device_is_ready(CODEC));
	tlv320aic3104_emul_reset();

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), EXPECT_DAC_50PCT,
		      "set 50%% dac");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), EXPECT_ROUTING_50PCT,
		      "set 50%% routing");

	audio_codec_stop_output(CODEC);
	zassert_not_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), EXPECT_DAC_50PCT,
			  "stop muted dac");
	zassert_not_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL),
			  EXPECT_ROUTING_50PCT, "stop muted routing");

	audio_codec_start_output(CODEC);

	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), EXPECT_DAC_50PCT,
		      "start_output kept left dac");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_RIGHT_DAC_VOL), EXPECT_DAC_50PCT,
		      "start_output kept right dac");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), EXPECT_ROUTING_50PCT,
		      "start_output kept L routing");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_R1_TO_HPROUT_VOL), EXPECT_ROUTING_50PCT,
		      "start_output kept R routing");
}

ZTEST(tlv320_volume_restore, test_configure_leaves_output_muted)
{
	zassert_true(device_is_ready(CODEC));
	tlv320aic3104_emul_reset();

	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
	};
	zassert_ok(audio_codec_configure(CODEC, &cfg));

	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), 0x80, "boot: left DAC muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_RIGHT_DAC_VOL), 0x80,
		      "boot: right DAC muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), 0xF6,
		      "boot: L routing held in the mute band");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_R1_TO_HPROUT_VOL), 0xF6,
		      "boot: R routing held in the mute band");

	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_LEFT_LOP_VOL), 0xF6,
		      "boot: L line-out held in the mute band");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_R1_TO_RIGHT_LOP_VOL), 0xF6,
		      "boot: R line-out held in the mute band");
}

ZTEST(tlv320_volume_restore, test_start_output_after_configure_uses_defaults)
{
	zassert_true(device_is_ready(CODEC));
	tlv320aic3104_emul_reset();

	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
	};
	zassert_ok(audio_codec_configure(CODEC, &cfg));

	audio_codec_start_output(CODEC);

	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), 0x00, "default dac 0 dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_RIGHT_DAC_VOL), 0x00, "default dac 0 dB");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), 0x80,
		      "routing enabled");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_R1_TO_HPROUT_VOL), 0x80,
		      "routing enabled");
}

ZTEST(tlv320_volume_restore, test_mute_unmute_restores_set_volume)
{
	audio_property_value_t vol = {.vol = TEST_VOLUME_DB};
	audio_property_value_t on = {.mute = true};
	audio_property_value_t off = {.mute = false};
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
	};

	zassert_true(device_is_ready(CODEC));
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	audio_codec_start_output(CODEC);

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), EXPECT_DAC_50PCT);
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), EXPECT_ROUTING_50PCT);

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    on));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), 0x80, "mute: left DAC muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_RIGHT_DAC_VOL), 0x80,
		      "mute: right DAC muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), 0xF6,
		      "mute: L routing in the mute band");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_R1_TO_HPROUT_VOL), 0xF6,
		      "mute: R routing in the mute band");

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    off));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), EXPECT_DAC_50PCT,
		      "unmute: left DAC back at the set level, not full scale");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_RIGHT_DAC_VOL), EXPECT_DAC_50PCT,
		      "unmute: right DAC back at the set level, not full scale");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), EXPECT_ROUTING_50PCT,
		      "unmute: L routing back at the set level, not full scale");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_R1_TO_HPROUT_VOL), EXPECT_ROUTING_50PCT,
		      "unmute: R routing back at the set level, not full scale");
}

ZTEST(tlv320_volume_restore, test_set_volume_while_muted_stays_silent)
{
	audio_property_value_t vol = {.vol = TEST_VOLUME_DB};
	audio_property_value_t on = {.mute = true};
	audio_property_value_t off = {.mute = false};
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
		.dai_route = AUDIO_ROUTE_PLAYBACK,
	};

	zassert_true(device_is_ready(CODEC));
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_configure(CODEC, &cfg));
	audio_codec_start_output(CODEC);

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    on));
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));

	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), 0x80, "still muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), 0xF6, "still muted");

	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    off));
	zassert_equal(tlv320aic3104_emul_last_val(0, R_LEFT_DAC_VOL), EXPECT_DAC_50PCT,
		      "unmute lands on the level set while muted");
	zassert_equal(tlv320aic3104_emul_last_val(0, R_DAC_L1_TO_HPLOUT_VOL), EXPECT_ROUTING_50PCT,
		      "unmute lands on the level set while muted");
}

ZTEST_SUITE(tlv320_volume_restore, NULL, NULL, NULL, NULL, NULL);
