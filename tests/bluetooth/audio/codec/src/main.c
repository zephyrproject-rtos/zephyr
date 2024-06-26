/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <ztest_test.h>
#include <ztest_assert.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(audio_codec_test_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_val)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000,
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_FREQ,
							BT_AUDIO_CODEC_CFG_FREQ_16KHZ)},
				   {});
	const uint8_t expected_data = BT_AUDIO_CODEC_CFG_FREQ_16KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_get_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_val)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000,
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_FREQ,
							BT_AUDIO_CODEC_CFG_FREQ_16KHZ)},
				   {});
	const uint8_t new_expected_data = BT_AUDIO_CODEC_CFG_FREQ_48KHZ;
	const uint8_t expected_data = BT_AUDIO_CODEC_CFG_FREQ_16KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_get_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ,
					 &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected data value %u", data[0]);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_val_new_value)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, {});
	const uint8_t new_expected_data = BT_AUDIO_CODEC_CFG_FREQ_48KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_get_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ,
					 &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_unset_val)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000,
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_FREQ,
							BT_AUDIO_CODEC_CFG_FREQ_16KHZ)},
				   {});
	const uint8_t expected_data = BT_AUDIO_CODEC_CFG_FREQ_16KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_get_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_unset_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ);
	zassert_true(ret >= 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_val(&codec_cfg, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_freq_to_freq_hz)
{
	const struct freq_test_input {
		enum bt_audio_codec_cfg_freq freq;
		uint32_t freq_hz;
	} freq_test_inputs[] = {
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_8KHZ, .freq_hz = 8000U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_11KHZ, .freq_hz = 11025U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_16KHZ, .freq_hz = 16000U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_22KHZ, .freq_hz = 22050U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_24KHZ, .freq_hz = 24000U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_32KHZ, .freq_hz = 32000U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_44KHZ, .freq_hz = 44100U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_48KHZ, .freq_hz = 48000U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_88KHZ, .freq_hz = 88200U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_96KHZ, .freq_hz = 96000U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_176KHZ, .freq_hz = 176400U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_192KHZ, .freq_hz = 192000U},
		{.freq = BT_AUDIO_CODEC_CFG_FREQ_384KHZ, .freq_hz = 384000U},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(freq_test_inputs); i++) {
		const struct freq_test_input *fti = &freq_test_inputs[i];

		zassert_equal(bt_audio_codec_cfg_freq_to_freq_hz(fti->freq), fti->freq_hz,
			      "freq %d was not coverted to %u", fti->freq, fti->freq_hz);
		zassert_equal(bt_audio_codec_cfg_freq_hz_to_freq(fti->freq_hz), fti->freq,
			      "freq_hz %u was not coverted to %d", fti->freq_hz, fti->freq);
	}
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_freq)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
						 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_freq(&preset.codec_cfg);
	zassert_equal(ret, 0x03, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_val_new)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	const uint8_t frame_blocks = 0x02;
	int ret;

	/* Frame blocks are not part of the preset, so we can use that to test adding a new type to
	 * the config
	 */
	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&preset.codec_cfg, false);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_frame_blocks_per_sdu(&preset.codec_cfg, frame_blocks);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&preset.codec_cfg, false);
	zassert_equal(ret, frame_blocks, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_freq)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_freq(&preset.codec_cfg);
	zassert_equal(ret, 0x03, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_freq(&preset.codec_cfg, BT_AUDIO_CODEC_CFG_FREQ_32KHZ);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_freq(&preset.codec_cfg);
	zassert_equal(ret, 0x06, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_frame_dur_to_frame_dur_us)
{
	const struct frame_dur_test_input {
		enum bt_audio_codec_cfg_frame_dur frame_dur;
		uint32_t frame_dur_us;
	} frame_dur_test_inputs[] = {
		{.frame_dur = BT_AUDIO_CODEC_CFG_DURATION_7_5, .frame_dur_us = 7500U},
		{.frame_dur = BT_AUDIO_CODEC_CFG_DURATION_10, .frame_dur_us = 10000U},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(frame_dur_test_inputs); i++) {
		const struct frame_dur_test_input *fdti = &frame_dur_test_inputs[i];

		zassert_equal(bt_audio_codec_cfg_frame_dur_to_frame_dur_us(fdti->frame_dur),
			      fdti->frame_dur_us, "frame_dur %d was not coverted to %u",
			      fdti->frame_dur, fdti->frame_dur_us);
		zassert_equal(bt_audio_codec_cfg_frame_dur_us_to_frame_dur(fdti->frame_dur_us),
			      fdti->frame_dur, "frame_dur_us %u was not coverted to %d",
			      fdti->frame_dur_us, fdti->frame_dur);
	}
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_dur)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_48_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,
						 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_frame_dur(&preset.codec_cfg);
	zassert_equal(ret, 0x01, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_frame_dur)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_frame_dur(&preset.codec_cfg);
	zassert_equal(ret, 0x01, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_frame_dur(&preset.codec_cfg,
					       BT_AUDIO_CODEC_CFG_DURATION_7_5);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_frame_dur(&preset.codec_cfg);
	zassert_equal(ret, 0x00, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_chan_allocation)
{
	const struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_8_1_1(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	enum bt_audio_location chan_allocation = BT_AUDIO_LOCATION_FRONT_RIGHT;
	int err;

	err = bt_audio_codec_cfg_get_chan_allocation(&preset.codec_cfg, &chan_allocation, false);
	zassert_false(err, "unexpected error %d", err);
	zassert_equal(chan_allocation, BT_AUDIO_LOCATION_FRONT_LEFT, "unexpected return value %d",
		      chan_allocation);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_chan_allocation_lc3_fallback_true)
{
	struct bt_audio_codec_cfg codec_cfg = {.id = BT_HCI_CODING_FORMAT_LC3};
	enum bt_audio_location chan_allocation;
	int err;

	err = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &chan_allocation, true);
	zassert_equal(err, 0, "unexpected error %d", err);
	zassert_equal(chan_allocation, BT_AUDIO_LOCATION_MONO_AUDIO, "unexpected return value %d",
		      chan_allocation);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_chan_allocation_lc3_fallback_false)
{
	struct bt_audio_codec_cfg codec_cfg = {.id = BT_HCI_CODING_FORMAT_LC3};
	enum bt_audio_location chan_allocation;
	int err;

	err = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &chan_allocation, false);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_chan_allocation_fallback_true)
{
	struct bt_audio_codec_cfg codec_cfg = {0};
	enum bt_audio_location chan_allocation;
	int err;

	err = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &chan_allocation, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_chan_allocation_fallback_false)
{
	struct bt_audio_codec_cfg codec_cfg = {0};
	enum bt_audio_location chan_allocation;
	int err;

	err = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &chan_allocation, false);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_chan_allocation)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	enum bt_audio_location chan_allocation;
	int err;

	err = bt_audio_codec_cfg_get_chan_allocation(&preset.codec_cfg, &chan_allocation, false);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zassert_equal(chan_allocation, 0x00000001, "Unexpected chan_allocation value %d",
		      chan_allocation);

	chan_allocation = BT_AUDIO_LOCATION_FRONT_RIGHT | BT_AUDIO_LOCATION_SIDE_RIGHT |
			  BT_AUDIO_LOCATION_TOP_SIDE_RIGHT | BT_AUDIO_LOCATION_RIGHT_SURROUND;
	err = bt_audio_codec_cfg_set_chan_allocation(&preset.codec_cfg, chan_allocation);
	zassert_true(err > 0, "Unexpected return value %d", err);

	err = bt_audio_codec_cfg_get_chan_allocation(&preset.codec_cfg, &chan_allocation, false);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zassert_equal(chan_allocation, 0x8080802, "Unexpected chan_allocation value %d",
		      chan_allocation);
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

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_octets_per_frame)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_32_2_2(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_octets_per_frame(&preset.codec_cfg);
	zassert_equal(ret, 80, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_octets_per_frame(&preset.codec_cfg, 120);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_octets_per_frame(&preset.codec_cfg);
	zassert_equal(ret, 120, "Unexpected return value %d", ret);
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

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_blocks_per_sdu_lc3_fallback_true)
{
	struct bt_audio_codec_cfg codec_cfg = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&codec_cfg, true);
	zassert_equal(err, 1, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_blocks_per_sdu_lc3_fallback_false)
{
	struct bt_audio_codec_cfg codec_cfg = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&codec_cfg, false);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_blocks_per_sdu_fallback_true)
{
	struct bt_audio_codec_cfg codec_cfg = {0};
	int err;

	err = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&codec_cfg, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_blocks_per_sdu_fallback_false)
{
	struct bt_audio_codec_cfg codec_cfg = {0};
	int err;

	err = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&codec_cfg, false);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_frame_blocks_per_sdu)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_32_2_2(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&preset.codec_cfg, true);
	zassert_equal(ret, 1, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_frame_blocks_per_sdu(&preset.codec_cfg, 2U);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&preset.codec_cfg, true);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_val)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	const uint8_t expected_data = BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_val)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	const uint8_t new_expected_data = BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE;
	const uint8_t expected_data = BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected data value %u", data[0]);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_val_new)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, {});
	const uint8_t new_expected_data = BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_unset_val)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	const uint8_t expected_data = BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_unset_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING);
	zassert_true(ret >= 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_val(&codec_cfg, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_pref_context)
{
	const enum bt_audio_context ctx =
		BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	const struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_pref_context(&codec_cfg, false);
	zassert_equal(ret, 0x0005, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_pref_context_lc3_fallback_true)
{
	struct bt_audio_codec_cfg codec_cfg = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cfg_meta_get_pref_context(&codec_cfg, true);
	zassert_equal(err, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_pref_context_lc3_fallback_false)
{
	struct bt_audio_codec_cfg codec_cfg = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cfg_meta_get_pref_context(&codec_cfg, false);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_pref_context_fallback_true)
{
	struct bt_audio_codec_cfg codec_cfg = {0};
	int err;

	err = bt_audio_codec_cfg_meta_get_pref_context(&codec_cfg, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_pref_context_fallback_false)
{
	struct bt_audio_codec_cfg codec_cfg = {0};
	int err;

	err = bt_audio_codec_cfg_meta_get_pref_context(&codec_cfg, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_pref_context)
{
	const enum bt_audio_context ctx =
		BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	const enum bt_audio_context new_ctx = BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS;
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_pref_context(&codec_cfg, false);
	zassert_equal(ret, 0x0005, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_pref_context(&codec_cfg, new_ctx);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_pref_context(&codec_cfg, false);
	zassert_equal(ret, 0x0100, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_stream_context)
{
	const enum bt_audio_context ctx =
		BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	const struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_stream_context(&codec_cfg);
	zassert_equal(ret, 0x0005, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_stream_context)
{
	enum bt_audio_context ctx = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_stream_context(&codec_cfg);
	zassert_equal(ret, 0x0005, "Unexpected return value %d", ret);

	ctx = BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS;
	ret = bt_audio_codec_cfg_meta_set_stream_context(&codec_cfg, ctx);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_stream_context(&codec_cfg);
	zassert_equal(ret, 0x0100, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_program_info)
{
	const uint8_t expected_data[] = {'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ',
					 'I', 'n', 'f', 'o'};
	const struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO,
				     'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ',
				     'I', 'n', 'f', 'o')});
	const uint8_t *program_data;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_program_info(&codec_cfg, &program_data);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_data, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_program_info)
{
	const uint8_t expected_data[] = {'P', 'r', 'o', 'g', 'r', 'a',
					 'm', ' ', 'I', 'n', 'f', 'o'};
	const uint8_t new_expected_data[] = {'N', 'e', 'w', ' ', 'i', 'n', 'f', 'o'};
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO, 'P', 'r', 'o', 'g', 'r',
				     'a', 'm', ' ', 'I', 'n', 'f', 'o')});
	const uint8_t *program_data;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_program_info(&codec_cfg, &program_data);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_data, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_set_program_info(&codec_cfg, new_expected_data,
						       ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_program_info(&codec_cfg, &program_data);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, program_data, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_lang)
{
	const struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_LANG, 'e', 'n', 'g')});
	char expected_data[] = "eng";
	const uint8_t *lang;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_lang(&codec_cfg, &lang);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, lang, BT_AUDIO_LANG_SIZE);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_lang)
{
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_LANG, 'e', 'n', 'g')});
	char new_expected_data[] = "deu";
	char expected_data[] = "eng";
	const uint8_t *lang;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_lang(&codec_cfg, &lang);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, lang, BT_AUDIO_LANG_SIZE);

	ret = bt_audio_codec_cfg_meta_set_lang(&codec_cfg, new_expected_data);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_lang(&codec_cfg, &lang);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, lang, BT_AUDIO_LANG_SIZE);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_ccid_list)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST, 0x05, 0x10, 0x15)});
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_ccid_list_shorter)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const uint8_t new_expected_data[] = {0x25, 0x30};
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST, 0x05, 0x10, 0x15)});
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_set_ccid_list(&codec_cfg, new_expected_data,
						    ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, ccid_list, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_ccid_list_longer)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const uint8_t new_expected_data[] = {0x25, 0x30, 0x35, 0x40};
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST, 0x05, 0x10, 0x15)});
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_set_ccid_list(&codec_cfg, new_expected_data,
						    ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, ccid_list, ARRAY_SIZE(new_expected_data));
}

/* Providing multiple BT_AUDIO_CODEC_DATA to BT_AUDIO_CODEC_CFG without packing it in a macro
 * cause compile issue, so define a macro to denote 2 types of data for the ccid_list_first tests
 */
#define DOUBLE_CFG_DATA                                                                            \
	{                                                                                          \
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST, 0x05, 0x10, 0x15),           \
			BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,                \
					    BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)              \
	}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_ccid_list_first_shorter)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const uint8_t new_expected_data[] = {0x25, 0x30};
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, DOUBLE_CFG_DATA);
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_ccid_list(&codec_cfg, new_expected_data,
						    ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, ccid_list, ARRAY_SIZE(new_expected_data));

	ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_ccid_list_first_longer)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const uint8_t new_expected_data[] = {0x25, 0x30, 0x35, 0x40};
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, DOUBLE_CFG_DATA);
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_ccid_list(&codec_cfg, new_expected_data,
						    ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_ccid_list(&codec_cfg, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, ccid_list, ARRAY_SIZE(new_expected_data));

	ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_parental_rating)
{
	const struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_parental_rating)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_parental_rating(&codec_cfg,
							  BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
	zassert_equal(ret, 0x0a, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_program_info_uri)
{
	const uint8_t expected_data[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'};
	const struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI,
				     'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm')});
	const uint8_t *program_info_uri;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_program_info_uri(&codec_cfg, &program_info_uri);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_info_uri, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_program_info_uri)
{
	const uint8_t expected_data[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'};
	const uint8_t new_expected_data[] = {'n', 'e', 'w', '.', 'c', 'o', 'm'};
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI, 'e', 'x', 'a', 'm',
				     'p', 'l', 'e', '.', 'c', 'o', 'm')});
	const uint8_t *program_info_uri;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_program_info_uri(&codec_cfg, &program_info_uri);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_info_uri, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_set_program_info_uri(&codec_cfg, new_expected_data,
							   ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_program_info_uri(&codec_cfg, &program_info_uri);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, program_info_uri, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_audio_active_state)
{
	const struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_AUDIO_STATE,
							BT_AUDIO_ACTIVE_STATE_ENABLED)});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_audio_active_state(&codec_cfg);
	zassert_equal(ret, 0x01, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_audio_active_state)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_AUDIO_STATE,
							BT_AUDIO_ACTIVE_STATE_ENABLED)});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_audio_active_state(&codec_cfg);
	zassert_equal(ret, 0x01, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_audio_active_state(&codec_cfg,
							     BT_AUDIO_ACTIVE_STATE_DISABLED);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_audio_active_state(&codec_cfg);
	zassert_equal(ret, 0x00, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag)
{
	const struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE)});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(&codec_cfg);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_bcast_audio_immediate_rend_flag)
{
	struct bt_audio_codec_cfg codec_cfg =
		BT_AUDIO_CODEC_CFG(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, {});
	int ret;

	ret = bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(&codec_cfg);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_set_bcast_audio_immediate_rend_flag(&codec_cfg);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(&codec_cfg);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_extended)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_EXTENDED, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *extended_meta;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_extended(&codec_cfg, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, extended_meta, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_extended)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const uint8_t new_expected_data[] = {0x04, 0x05};
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_EXTENDED, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *extended_meta;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_extended(&codec_cfg, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, extended_meta, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_set_extended(&codec_cfg, new_expected_data,
						   ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_extended(&codec_cfg, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, extended_meta, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_get_vendor)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *vendor_meta;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_vendor(&codec_cfg, &vendor_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, vendor_meta, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_meta_set_vendor)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const uint8_t new_expected_data[] = {0x04, 0x05, 0x06, 0x07, 0x08};
	struct bt_audio_codec_cfg codec_cfg = BT_AUDIO_CODEC_CFG(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *extended_meta;
	int ret;

	ret = bt_audio_codec_cfg_meta_get_vendor(&codec_cfg, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, extended_meta, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cfg_meta_set_vendor(&codec_cfg, new_expected_data,
						 ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_meta_get_vendor(&codec_cfg, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, extended_meta, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_val)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000,
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_FREQ,
							BT_AUDIO_CODEC_CFG_FREQ_16KHZ)},
				   {});
	const uint8_t expected_data = BT_AUDIO_CODEC_CFG_FREQ_16KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_get_val(&codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_set_val)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000,
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_FREQ,
							BT_AUDIO_CODEC_CFG_FREQ_16KHZ)},
				   {});
	const uint8_t new_expected_data = BT_AUDIO_CODEC_CFG_FREQ_48KHZ;
	const uint8_t expected_data = BT_AUDIO_CODEC_CFG_FREQ_16KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_get_val(&codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_set_val(&codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ,
					 &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_val(&codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ, &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected data value %u", data[0]);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_set_val_new)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, {});
	const uint8_t new_expected_data = BT_AUDIO_CODEC_CFG_FREQ_48KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_get_val(&codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ, &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_set_val(&codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ,
					 &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_val(&codec_cap, BT_AUDIO_CODEC_CAP_TYPE_FREQ, &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_unset_val)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000,
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_FREQ,
							BT_AUDIO_CODEC_CFG_FREQ_16KHZ)},
				   {});
	const uint8_t expected_data = BT_AUDIO_CODEC_CFG_FREQ_16KHZ;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_get_val(&codec_cap, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_unset_val(&codec_cap, BT_AUDIO_CODEC_CFG_FREQ);
	zassert_true(ret >= 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_val(&codec_cap, BT_AUDIO_CODEC_CFG_FREQ, &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_freq)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_freq(&codec_cap);
	zassert_equal(ret, 4, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_set_freq)
{
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_freq(&codec_cap);
	zassert_equal(ret, 4, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_set_freq(&codec_cap, BT_AUDIO_CODEC_CAP_FREQ_22KHZ);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_freq(&codec_cap);
	zassert_equal(ret, 8, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_frame_dur)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_frame_dur(&codec_cap);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_set_frame_dur)
{
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_frame_dur(&codec_cap);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_set_frame_dur(&codec_cap, BT_AUDIO_CODEC_CAP_DURATION_7_5);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_frame_dur(&codec_cap);
	zassert_equal(ret, 1, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_supported_audio_chan_counts)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(2), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap, false);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite,
	test_bt_audio_codec_cap_get_supported_audio_chan_counts_lc3_fallback_true)
{
	struct bt_audio_codec_cap codec_cap = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap, true);
	zassert_equal(err, 1, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite,
	test_bt_audio_codec_cap_get_supported_audio_chan_counts_lc3_fallback_false)
{
	struct bt_audio_codec_cap codec_cap = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap, false);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_supported_audio_chan_counts_fallback_true)
{
	struct bt_audio_codec_cap codec_cap = {0};
	int err;

	err = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite,
	test_bt_audio_codec_cap_get_supported_audio_chan_counts_fallback_false)
{
	struct bt_audio_codec_cap codec_cap = {0};
	int err;

	err = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_set_supported_audio_chan_counts)
{
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap, false);
	zassert_equal(ret, 1, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_set_supported_audio_chan_counts(
		&codec_cap, BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(2));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap, false);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_octets_per_frame)
{
	struct bt_audio_codec_octets_per_codec_frame expected = {
		.min = 40U,
		.max = 120U,
	};
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));
	struct bt_audio_codec_octets_per_codec_frame codec_frame;

	int ret;

	ret = bt_audio_codec_cap_get_octets_per_frame(&codec_cap, &codec_frame);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_equal(codec_frame.min, expected.min, "Unexpected minimum value %d",
		      codec_frame.min);
	zassert_equal(codec_frame.max, expected.max, "Unexpected maximum value %d",
		      codec_frame.max);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_set_octets_per_frame)
{
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));
	struct bt_audio_codec_octets_per_codec_frame codec_frame;
	int ret;

	ret = bt_audio_codec_cap_get_octets_per_frame(&codec_cap, &codec_frame);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_equal(codec_frame.min, 40U, "Unexpected minimum value %d", codec_frame.min);
	zassert_equal(codec_frame.max, 120U, "Unexpected maximum value %d", codec_frame.max);

	codec_frame.min = 50U;
	codec_frame.max = 100U;
	ret = bt_audio_codec_cap_set_octets_per_frame(&codec_cap, &codec_frame);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_octets_per_frame(&codec_cap, &codec_frame);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_equal(codec_frame.min, 50U, "Unexpected minimum value %d", codec_frame.min);
	zassert_equal(codec_frame.max, 100U, "Unexpected maximum value %d", codec_frame.max);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_max_codec_frames_per_sdu)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap, false);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite,
	test_bt_audio_codec_cap_get_max_codec_frames_per_sdu_lc3_fallback_true)
{
	struct bt_audio_codec_cap codec_cap = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap, true);
	zassert_equal(err, 1, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite,
	test_bt_audio_codec_cap_get_max_codec_frames_per_sdu_lc3_fallback_false)
{
	struct bt_audio_codec_cap codec_cap = {.id = BT_HCI_CODING_FORMAT_LC3};
	int err;

	err = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap, false);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_max_codec_frames_per_sdu_fallback_true)
{
	struct bt_audio_codec_cap codec_cap = {0};
	int err;

	err = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_max_codec_frames_per_sdu_fallback_false)
{
	struct bt_audio_codec_cap codec_cap = {0};
	int err;

	err = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap, true);
	zassert_equal(err, -ENODATA, "unexpected error %d", err);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_set_max_codec_frames_per_sdu)
{
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ, BT_AUDIO_CODEC_CAP_DURATION_10,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap, false);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_set_max_codec_frames_per_sdu(&codec_cap, 4U);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap, false);
	zassert_equal(ret, 4, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_val)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	const uint8_t expected_data = BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_val)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	const uint8_t new_expected_data = BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE;
	const uint8_t expected_data = BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_set_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected data value %u", data[0]);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_val_new)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, {});
	const uint8_t new_expected_data = BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_set_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &new_expected_data, sizeof(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(new_expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], new_expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_unset_val_only)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	const uint8_t expected_data = BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_unset_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING);
	zassert_true(ret >= 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);
}

/* Providing multiple BT_AUDIO_CODEC_DATA to BT_AUDIO_CODEC_CAP without packing it in a macro
 * cause compile issue, so define a macro to denote 3 types of metadata for the meta_unset tests
 */
#define TRIPLE_META_DATA                                                                           \
	{                                                                                          \
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,                        \
				    BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE),                     \
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,                           \
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)),        \
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,                         \
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED))         \
	}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_unset_val_first)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, TRIPLE_META_DATA);
	const uint8_t expected_data = BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(data[0], expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_unset_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING);
	zassert_true(ret >= 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					      &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_unset_val_middle)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, TRIPLE_META_DATA);
	const uint16_t expected_data = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(sys_get_le16(data), expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_unset_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PREF_CONTEXT);
	zassert_true(ret >= 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
					      &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_unset_val_last)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, TRIPLE_META_DATA);
	const uint16_t expected_data = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
	const uint8_t *data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
					      &data);
	zassert_equal(ret, sizeof(expected_data), "Unexpected return value %d", ret);
	zassert_equal(sys_get_le16(data), expected_data, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_unset_val(&codec_cap, BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT);
	zassert_true(ret >= 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_val(&codec_cap, BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
					      &data);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_pref_context)
{
	const enum bt_audio_context ctx =
		BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	const struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cap_meta_get_pref_context(&codec_cap);
	zassert_equal(ret, 0x0005, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_pref_context)
{
	const enum bt_audio_context ctx =
		BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	const enum bt_audio_context new_ctx = BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS;
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cap_meta_get_pref_context(&codec_cap);
	zassert_equal(ret, 0x0005, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_set_pref_context(&codec_cap, new_ctx);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_pref_context(&codec_cap);
	zassert_equal(ret, 0x0100, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_stream_context)
{
	const enum bt_audio_context ctx =
		BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	const struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cap_meta_get_stream_context(&codec_cap);
	zassert_equal(ret, 0x0005, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_stream_context)
{
	enum bt_audio_context ctx = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA;
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
							BT_BYTES_LIST_LE16(ctx))});
	int ret;

	ret = bt_audio_codec_cap_meta_get_stream_context(&codec_cap);
	zassert_equal(ret, 0x0005, "Unexpected return value %d", ret);

	ctx = BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS;
	ret = bt_audio_codec_cap_meta_set_stream_context(&codec_cap, ctx);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_stream_context(&codec_cap);
	zassert_equal(ret, 0x0100, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_program_info)
{
	const uint8_t expected_data[] = {'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ',
					 'I', 'n', 'f', 'o'};
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO,
				     'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ',
				     'I', 'n', 'f', 'o')});
	const uint8_t *program_data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_program_info(&codec_cap, &program_data);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_data, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_program_info)
{
	const uint8_t expected_data[] = {'P', 'r', 'o', 'g', 'r', 'a',
					 'm', ' ', 'I', 'n', 'f', 'o'};
	const uint8_t new_expected_data[] = {'N', 'e', 'w', ' ', 'i', 'n', 'f', 'o'};
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO, 'P', 'r', 'o', 'g', 'r',
				     'a', 'm', ' ', 'I', 'n', 'f', 'o')});
	const uint8_t *program_data;
	int ret;

	ret = bt_audio_codec_cap_meta_get_program_info(&codec_cap, &program_data);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_data, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cap_meta_set_program_info(&codec_cap, new_expected_data,
						       ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_program_info(&codec_cap, &program_data);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, program_data, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_lang)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_LANG, 'e', 'n', 'g')});
	char expected_data[] = "eng";
	const uint8_t *lang;
	int ret;

	ret = bt_audio_codec_cap_meta_get_lang(&codec_cap, &lang);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, lang, BT_AUDIO_LANG_SIZE);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_lang)
{
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_LANG, 'e', 'n', 'g')});
	char new_expected_data[] = "deu";
	char expected_data[] = "eng";
	const uint8_t *lang;
	int ret;

	ret = bt_audio_codec_cap_meta_get_lang(&codec_cap, &lang);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, lang, BT_AUDIO_LANG_SIZE);

	ret = bt_audio_codec_cap_meta_set_lang(&codec_cap, new_expected_data);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_lang(&codec_cap, &lang);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, lang, BT_AUDIO_LANG_SIZE);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_ccid_list)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST, 0x05, 0x10, 0x15)});
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_cap_meta_get_ccid_list(&codec_cap, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_ccid_list)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const uint8_t new_expected_data[] = {0x25, 0x30};
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST, 0x05, 0x10, 0x15)});
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_cap_meta_get_ccid_list(&codec_cap, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cap_meta_set_ccid_list(&codec_cap, new_expected_data,
						    ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_ccid_list(&codec_cap, &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, ccid_list, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_parental_rating)
{
	const struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	int ret;

	ret = bt_audio_codec_cap_meta_get_parental_rating(&codec_cap);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_parental_rating)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
							BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE)});
	int ret;

	ret = bt_audio_codec_cap_meta_get_parental_rating(&codec_cap);
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_set_parental_rating(&codec_cap,
							  BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_parental_rating(&codec_cap);
	zassert_equal(ret, 0x0a, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_program_info_uri)
{
	const uint8_t expected_data[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'};
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI,
				     'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm')});
	const uint8_t *program_info_uri;
	int ret;

	ret = bt_audio_codec_cap_meta_get_program_info_uri(&codec_cap, &program_info_uri);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_info_uri, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_program_info_uri)
{
	const uint8_t expected_data[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'};
	const uint8_t new_expected_data[] = {'n', 'e', 'w', '.', 'c', 'o', 'm'};
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI, 'e', 'x', 'a', 'm',
				     'p', 'l', 'e', '.', 'c', 'o', 'm')});
	const uint8_t *program_info_uri;
	int ret;

	ret = bt_audio_codec_cap_meta_get_program_info_uri(&codec_cap, &program_info_uri);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_info_uri, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cap_meta_set_program_info_uri(&codec_cap, new_expected_data,
							   ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_program_info_uri(&codec_cap, &program_info_uri);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, program_info_uri, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_audio_active_state)
{
	const struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_AUDIO_STATE,
							BT_AUDIO_ACTIVE_STATE_ENABLED)});
	int ret;

	ret = bt_audio_codec_cap_meta_get_audio_active_state(&codec_cap);
	zassert_equal(ret, 0x01, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_audio_active_state)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
				   {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_AUDIO_STATE,
							BT_AUDIO_ACTIVE_STATE_ENABLED)});
	int ret;

	ret = bt_audio_codec_cap_meta_get_audio_active_state(&codec_cap);
	zassert_equal(ret, 0x01, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_set_audio_active_state(&codec_cap,
							     BT_AUDIO_ACTIVE_STATE_DISABLED);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_audio_active_state(&codec_cap);
	zassert_equal(ret, 0x00, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_bcast_audio_immediate_rend_flag)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE)});
	int ret;

	ret = bt_audio_codec_cap_meta_get_bcast_audio_immediate_rend_flag(&codec_cap);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_bcast_audio_immediate_rend_flag)
{
	struct bt_audio_codec_cap codec_cap =
		BT_AUDIO_CODEC_CAP(BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {}, {});
	int ret;

	ret = bt_audio_codec_cap_meta_get_bcast_audio_immediate_rend_flag(&codec_cap);
	zassert_equal(ret, -ENODATA, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_set_bcast_audio_immediate_rend_flag(&codec_cap);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_bcast_audio_immediate_rend_flag(&codec_cap);
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_extended)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_EXTENDED, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *extended_meta;
	int ret;

	ret = bt_audio_codec_cap_meta_get_extended(&codec_cap, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, extended_meta, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_extended)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const uint8_t new_expected_data[] = {0x04, 0x05};
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_EXTENDED, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *extended_meta;
	int ret;

	ret = bt_audio_codec_cap_meta_get_extended(&codec_cap, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, extended_meta, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cap_meta_set_extended(&codec_cap, new_expected_data,
						   ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_extended(&codec_cap, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, extended_meta, ARRAY_SIZE(new_expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_get_vendor)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *vendor_meta;
	int ret;

	ret = bt_audio_codec_cap_meta_get_vendor(&codec_cap, &vendor_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, vendor_meta, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_meta_set_vendor)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const uint8_t new_expected_data[] = {0x04, 0x05, 0x06, 0x07, 0x08};
	struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP(
		BT_HCI_CODING_FORMAT_LC3, 0x0000, 0x0000, {},
		{BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, 0x00, 0x01, 0x02, 0x03)});
	const uint8_t *extended_meta;
	int ret;

	ret = bt_audio_codec_cap_meta_get_vendor(&codec_cap, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, extended_meta, ARRAY_SIZE(expected_data));

	ret = bt_audio_codec_cap_meta_set_vendor(&codec_cap, new_expected_data,
						 ARRAY_SIZE(new_expected_data));
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cap_meta_get_vendor(&codec_cap, &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(new_expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(new_expected_data, extended_meta, ARRAY_SIZE(new_expected_data));
}
