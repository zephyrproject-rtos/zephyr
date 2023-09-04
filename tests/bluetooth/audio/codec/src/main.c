/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(audio_codec_test_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_freq)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
						 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_freq(&preset.codec_cfg);
	zassert_equal(ret, 16000u, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_duration_us)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_48_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,
						 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_frame_duration_us(&preset.codec_cfg);
	zassert_equal(ret, 10000u, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_chan_allocation_val)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_8_1_1(BT_AUDIO_LOCATION_FRONT_LEFT,
						BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	enum bt_audio_location chan_allocation = BT_AUDIO_LOCATION_FRONT_RIGHT;
	int err;

	err = bt_audio_codec_cfg_get_chan_allocation_val(&preset.codec_cfg, &chan_allocation);
	zassert_false(err, "unexpected error %d", err);
	zassert_equal(chan_allocation, BT_AUDIO_LOCATION_FRONT_LEFT,
		      "unexpected return value %d", chan_allocation);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_octets_per_frame)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_32_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,
						 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_octets_per_frame(&preset.codec_cfg);
	zassert_equal(ret, 80u, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_blocks_per_sdu)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_48_5_1(BT_AUDIO_LOCATION_FRONT_LEFT,
						 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&preset.codec_cfg, true);
	zassert_equal(ret, 1u, "unexpected return value %d", ret);
}
