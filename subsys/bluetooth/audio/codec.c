/*
 * Copyright (c) 2022 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/** @file
 *  @brief Bluetooth LE-Audio codec LTV parsing
 *
 *  Helper functions to parse codec config data as specified in the Bluetooth assigned numbers for
 *  Generic Audio.
 */

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_audio_codec, CONFIG_BT_AUDIO_CODEC_LOG_LEVEL);

bool bt_audio_codec_cfg_get_val(const struct bt_audio_codec_cfg *codec_cfg, uint8_t type,
				const struct bt_audio_codec_data **data)
{
	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return false;
	}

	for (size_t i = 0; i < codec_cfg->data_count; i++) {
		if (codec_cfg->data[i].data.type == type) {
			*data = &codec_cfg->data[i];

			return true;
		}
	}

	return false;
}

int bt_audio_codec_cfg_get_freq(const struct bt_audio_codec_cfg *codec_cfg)
{
	const struct bt_audio_codec_data *element;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	if (bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FREQ, &element)) {

		switch (element->data.data[0]) {
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_8KHZ:
			return 8000;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_11KHZ:
			return 11025;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_16KHZ:
			return 16000;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_22KHZ:
			return 22050;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_24KHZ:
			return 24000;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_32KHZ:
			return 32000;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_44KHZ:
			return 44100;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_48KHZ:
			return 48000;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_88KHZ:
			return 88200;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_96KHZ:
			return 96000;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_176KHZ:
			return 176400;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_192KHZ:
			return 192000;
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_384KHZ:
			return 384000;
		default:
			return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
		}
	}

	return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
}

int bt_audio_codec_cfg_get_frame_duration_us(const struct bt_audio_codec_cfg *codec_cfg)
{
	const struct bt_audio_codec_data *element;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	if (bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_DURATION, &element)) {
		switch (element->data.data[0]) {
		case BT_AUDIO_CODEC_CONFIG_LC3_DURATION_7_5:
			return 7500;
		case BT_AUDIO_CODEC_CONFIG_LC3_DURATION_10:
			return 10000;
		default:
			return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
		}
	}

	return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
}

int bt_audio_codec_cfg_get_chan_allocation_val(const struct bt_audio_codec_cfg *codec_cfg,
					 enum bt_audio_location *chan_allocation)
{
	const struct bt_audio_codec_data *element;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	CHECKIF(chan_allocation == NULL) {
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	*chan_allocation = 0;
	if (bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_CHAN_ALLOC, &element)) {

		*chan_allocation = sys_le32_to_cpu(*((uint32_t *)&element->data.data[0]));

		return BT_AUDIO_CODEC_PARSE_ERR_SUCCESS;
	}

	return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
}

int bt_audio_codec_cfg_get_octets_per_frame(const struct bt_audio_codec_cfg *codec_cfg)
{
	const struct bt_audio_codec_data *element;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	if (bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FRAME_LEN, &element)) {

		return sys_le16_to_cpu(*((uint16_t *)&element->data.data[0]));
	}

	return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
}

int bt_audio_codec_cfg_get_frame_blocks_per_sdu(const struct bt_audio_codec_cfg *codec_cfg,
					  bool fallback_to_default)
{
	const struct bt_audio_codec_data *element;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	if (bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FRAME_BLKS_PER_SDU,
				       &element)) {

		return element->data.data[0];
	}

	if (fallback_to_default) {

		return 1;
	}

	return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
}
