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

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0

struct search_type_param {
	uint8_t type;
	uint8_t data_len;
	const uint8_t *data;
};

static bool parse_cb(struct bt_data *data, void *user_data)
{
	struct search_type_param *param = (struct search_type_param *)user_data;

	if (param->type == data->type) {
		param->data_len = data->data_len;
		param->data = data->data;

		return false;
	}

	return true;
}

uint8_t bt_audio_codec_cfg_get_val(const struct bt_audio_codec_cfg *codec_cfg, uint8_t type,
				   const uint8_t **data)
{
	struct search_type_param param = {
		.type = type,
		.data_len = 0,
		.data = *data,
	};
	int err;

	*data = NULL;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return 0;
	}

	err = bt_audio_data_parse(codec_cfg->data, codec_cfg->data_len, parse_cb, &param);
	if (err != 0 && err != -ECANCELED) {
		LOG_DBG("Could not parse the data: %d", err);
		return 0;
	}

	if (param.data == NULL) {
		LOG_DBG("Could not find the type %u", type);
		return 0;
	}

	return param.data_len;
}

int bt_audio_codec_cfg_get_freq(const struct bt_audio_codec_cfg *codec_cfg)
{
	const uint8_t *data;
	uint8_t data_len;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FREQ, &data);
	if (data == NULL) {
		return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
	}

	if (data_len != sizeof(uint8_t)) {
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
	}

	switch (data[0]) {
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

int bt_audio_codec_cfg_get_frame_duration_us(const struct bt_audio_codec_cfg *codec_cfg)
{
	const uint8_t *data;
	uint8_t data_len;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_DURATION, &data);
	if (data == NULL) {
		return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
	}

	if (data_len != sizeof(uint8_t)) {
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
	}

	switch (data[0]) {
	case BT_AUDIO_CODEC_CONFIG_LC3_DURATION_7_5:
		return 7500;
	case BT_AUDIO_CODEC_CONFIG_LC3_DURATION_10:
		return 10000;
	default:
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
	}
}

int bt_audio_codec_cfg_get_chan_allocation_val(const struct bt_audio_codec_cfg *codec_cfg,
					       enum bt_audio_location *chan_allocation)
{
	const uint8_t *data;
	uint8_t data_len;

	*chan_allocation = 0;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	CHECKIF(chan_allocation == NULL) {
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_CHAN_ALLOC,
					      &data);
	if (data == NULL) {
		return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
	}

	if (data_len != sizeof(uint32_t)) {
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
	}

	*chan_allocation = sys_get_le32(data);

	return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
}

int bt_audio_codec_cfg_get_octets_per_frame(const struct bt_audio_codec_cfg *codec_cfg)
{
	const uint8_t *data;
	uint8_t data_len;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FRAME_LEN,
					      &data);
	if (data == NULL) {
		return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
	}

	if (data_len != sizeof(uint16_t)) {
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
	}

	return sys_get_le16(data);
}

int bt_audio_codec_cfg_get_frame_blocks_per_sdu(const struct bt_audio_codec_cfg *codec_cfg,
						bool fallback_to_default)
{
	const uint8_t *data;
	uint8_t data_len;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg,
					      BT_AUDIO_CODEC_CONFIG_LC3_FRAME_BLKS_PER_SDU, &data);
	if (data == NULL) {
		if (fallback_to_default) {
			return 1;
		}

		return BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND;
	}

	if (data_len != sizeof(uint8_t)) {
		return BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND;
	}

	return data[0];
}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
