/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/sys/byteorder.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(audio_codec_test_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_freq_to_freq_hz)
{
	const struct freq_test_input {
		enum bt_audio_codec_config_freq freq;
		uint32_t freq_hz;
	} freq_test_inputs[] = {
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_8KHZ, .freq_hz = 8000U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_11KHZ, .freq_hz = 11025U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_16KHZ, .freq_hz = 16000U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_22KHZ, .freq_hz = 22050U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_24KHZ, .freq_hz = 24000U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_32KHZ, .freq_hz = 32000U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_44KHZ, .freq_hz = 44100U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_48KHZ, .freq_hz = 48000U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_88KHZ, .freq_hz = 88200U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_96KHZ, .freq_hz = 96000U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_176KHZ, .freq_hz = 176400U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_192KHZ, .freq_hz = 192000U},
		{.freq = BT_AUDIO_CODEC_CONFIG_LC3_FREQ_384KHZ, .freq_hz = 384000U},
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

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_set_freq)
{
	struct bt_bap_lc3_preset preset = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
		BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_freq(&preset.codec_cfg);
	zassert_equal(ret, 0x03, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_set_freq(&preset.codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FREQ_32KHZ);
	zassert_true(ret > 0, "Unexpected return value %d", ret);

	ret = bt_audio_codec_cfg_get_freq(&preset.codec_cfg);
	zassert_equal(ret, 0x06, "Unexpected return value %d", ret);
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
	zassert_equal(chan_allocation, BT_AUDIO_LOCATION_FRONT_LEFT, "unexpected return value %d",
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

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cfg_get_frame_blocks_per_sdu)
{
	const struct bt_bap_lc3_preset preset =
		BT_BAP_LC3_UNICAST_PRESET_48_5_1(BT_AUDIO_LOCATION_FRONT_LEFT,
						 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
	int ret;

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(&preset.codec_cfg, true);
	zassert_equal(ret, 1u, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_pref_context)
{
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED |
						       BT_AUDIO_CONTEXT_TYPE_MEDIA)),
	};
	int ret;

	ret = bt_audio_codec_meta_get_pref_context(metadata, ARRAY_SIZE(metadata));
	zassert_equal(ret, 0x0005, "unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_stream_context)
{
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
				    BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED |
						       BT_AUDIO_CONTEXT_TYPE_MEDIA)),
	};
	int ret;

	ret = bt_audio_codec_meta_get_stream_context(metadata, ARRAY_SIZE(metadata));
	zassert_equal(ret, 0x0005, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_program_info)
{
	const uint8_t expected_data[] = {'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ',
					 'I', 'n', 'f', 'o'};
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO,
				    'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ', 'I', 'n', 'f', 'o'),
	};
	const uint8_t *program_data;
	int ret;

	ret = bt_audio_codec_meta_get_program_info(metadata, ARRAY_SIZE(metadata), &program_data);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_data, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_stream_lang)
{
	const uint32_t expected_data = sys_get_le24((uint8_t[]){'e', 'n', 'g'});
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_LANG, 'e', 'n', 'g'),
	};
	int ret;

	ret = bt_audio_codec_meta_get_stream_lang(metadata, ARRAY_SIZE(metadata));
	zassert_equal(ret, expected_data, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_ccid_list)
{
	const uint8_t expected_data[] = {0x05, 0x10, 0x15};
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST, 0x05, 0x10, 0x15),
	};
	const uint8_t *ccid_list;
	int ret;

	ret = bt_audio_codec_meta_get_ccid_list(metadata, ARRAY_SIZE(metadata), &ccid_list);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, ccid_list, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_parental_rating)
{
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
				    BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE),
	};
	int ret;

	ret = bt_audio_codec_meta_get_parental_rating(metadata, ARRAY_SIZE(metadata));
	zassert_equal(ret, 0x07, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_program_info_uri)
{
	const uint8_t expected_data[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'};
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI,
				    'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm'),
	};
	const uint8_t *program_info_uri;
	int ret;

	ret = bt_audio_codec_meta_get_program_info_uri(metadata, ARRAY_SIZE(metadata),
						       &program_info_uri);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, program_info_uri, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_audio_active_state)
{
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_AUDIO_STATE,
				    BT_AUDIO_ACTIVE_STATE_ENABLED),
	};
	int ret;

	ret = bt_audio_codec_meta_get_audio_active_state(metadata, ARRAY_SIZE(metadata));
	zassert_equal(ret, 0x01, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_broadcast_audio_immediate_rendering_flag)
{
	const uint8_t metadata[] = {0x01, BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE};
	int ret;

	ret = bt_audio_codec_meta_get_broadcast_audio_immediate_rendering_flag(
		metadata, ARRAY_SIZE(metadata));
	zassert_equal(ret, 0, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_extended)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_EXTENDED, 0x00, 0x01, 0x02, 0x03),
	};
	const uint8_t *extended_meta;
	int ret;

	ret = bt_audio_codec_meta_get_extended(metadata, ARRAY_SIZE(metadata), &extended_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, extended_meta, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_meta_get_vendor)
{
	const uint8_t expected_data[] = {0x00, 0x01, 0x02, 0x03};
	const uint8_t metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, 0x00, 0x01, 0x02, 0x03),
	};
	const uint8_t *vendor_meta;
	int ret;

	ret = bt_audio_codec_meta_get_vendor(metadata, ARRAY_SIZE(metadata), &vendor_meta);
	zassert_equal(ret, ARRAY_SIZE(expected_data), "Unexpected return value %d", ret);
	zassert_mem_equal(expected_data, vendor_meta, ARRAY_SIZE(expected_data));
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_freq)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_LC3_FREQ_16KHZ, BT_AUDIO_CODEC_LC3_DURATION_10,
		BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_freq(&codec_cap);
	zassert_equal(ret, 4, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_frame_duration)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_LC3_FREQ_16KHZ, BT_AUDIO_CODEC_LC3_DURATION_10,
		BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_frame_duration(&codec_cap);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_supported_audio_chan_counts)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_LC3_FREQ_16KHZ, BT_AUDIO_CODEC_LC3_DURATION_10,
		BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(2), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_supported_audio_chan_counts(&codec_cap);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_octets_per_frame)
{
	struct bt_audio_codec_octets_per_codec_frame expected = {
		.min = 40U,
		.max = 120U,
	};
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_LC3_FREQ_16KHZ, BT_AUDIO_CODEC_LC3_DURATION_10,
		BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
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

ZTEST(audio_codec_test_suite, test_bt_audio_codec_cap_get_max_codec_frames_per_sdu)
{
	const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_LC3_FREQ_16KHZ, BT_AUDIO_CODEC_LC3_DURATION_10,
		BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40U, 120U, 2U,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

	int ret;

	ret = bt_audio_codec_cap_get_max_codec_frames_per_sdu(&codec_cap);
	zassert_equal(ret, 2, "Unexpected return value %d", ret);
}
