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

struct search_type_param {
	bool found;
	uint8_t type;
	uint8_t data_len;
	const uint8_t **data;
};

static bool parse_cb(struct bt_data *data, void *user_data)
{
	struct search_type_param *param = (struct search_type_param *)user_data;

	if (param->type == data->type) {
		param->found = true;
		param->data_len = data->data_len;
		*param->data = data->data;

		return false;
	}

	return true;
}

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0

uint8_t bt_audio_codec_cfg_get_val(const struct bt_audio_codec_cfg *codec_cfg, uint8_t type,
				   const uint8_t **data)
{
	struct search_type_param param = {
		.found = false,
		.type = type,
		.data_len = 0,
		.data = data,
	};
	int err;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return 0;
	}

	CHECKIF(data == NULL) {
		LOG_DBG("data is NULL");
		return 0;
	}

	*data = NULL;

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
		return -EINVAL;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FREQ, &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (data_len != sizeof(uint8_t)) {
		return -EBADMSG;
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
		return -EBADMSG;
	}
}

int bt_audio_codec_cfg_get_frame_duration_us(const struct bt_audio_codec_cfg *codec_cfg)
{
	const uint8_t *data;
	uint8_t data_len;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return -EINVAL;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_DURATION, &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (data_len != sizeof(uint8_t)) {
		return -EBADMSG;
	}

	switch (data[0]) {
	case BT_AUDIO_CODEC_CONFIG_LC3_DURATION_7_5:
		return 7500;
	case BT_AUDIO_CODEC_CONFIG_LC3_DURATION_10:
		return 10000;
	default:
		return -EBADMSG;
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
		return -EINVAL;
	}

	CHECKIF(chan_allocation == NULL) {
		return -EINVAL;
	}

	data_len =
		bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_CHAN_ALLOC, &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (data_len != sizeof(uint32_t)) {
		return -EBADMSG;
	}

	*chan_allocation = sys_get_le32(data);

	return 0;
}

int bt_audio_codec_cfg_get_octets_per_frame(const struct bt_audio_codec_cfg *codec_cfg)
{
	const uint8_t *data;
	uint8_t data_len;

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec is NULL");
		return -EINVAL;
	}

	data_len =
		bt_audio_codec_cfg_get_val(codec_cfg, BT_AUDIO_CODEC_CONFIG_LC3_FRAME_LEN, &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (data_len != sizeof(uint16_t)) {
		return -EBADMSG;
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
		return -EINVAL;
	}

	data_len = bt_audio_codec_cfg_get_val(codec_cfg,
					      BT_AUDIO_CODEC_CONFIG_LC3_FRAME_BLKS_PER_SDU, &data);
	if (data == NULL) {
		if (fallback_to_default) {
			return 1;
		}

		return -ENODATA;
	}

	if (data_len != sizeof(uint8_t)) {
		return -EBADMSG;
	}

	return data[0];
}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 ||                                             \
	CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0

int bt_audio_codec_meta_get_val(const uint8_t meta[], size_t meta_len, uint8_t type,
				const uint8_t **data)
{
	struct search_type_param param = {
		.type = type,
		.data_len = 0,
		.data = data,
	};
	int err;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	CHECKIF(data == NULL) {
		LOG_DBG("data is NULL");
		return -EINVAL;
	}

	*data = NULL;

	err = bt_audio_data_parse(meta, meta_len, parse_cb, &param);
	if (err != 0 && err != -ECANCELED) {
		LOG_DBG("Could not parse the meta data: %d", err);
		return err;
	}

	if (!param.found) {
		LOG_DBG("Could not find the type %u", type);
		return -ENODATA;
	}

	return param.data_len;
}

int bt_audio_codec_meta_get_pref_context(const uint8_t meta[], size_t meta_len)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
					  &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (ret != sizeof(uint16_t)) {
		return -EBADMSG;
	}

	return sys_get_le16(data);
}

int bt_audio_codec_meta_get_stream_context(const uint8_t meta[], size_t meta_len)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
					  &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (ret != sizeof(uint16_t)) {
		return -EBADMSG;
	}

	return sys_get_le16(data);
}

int bt_audio_codec_meta_get_program_info(const uint8_t meta[], size_t meta_len,
					 const uint8_t **program_info)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	CHECKIF(program_info == NULL) {
		LOG_DBG("program_info is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_PROGRAM_INFO,
					  &data);
	if (data == NULL) {
		return -ENODATA;
	}

	*program_info = data;

	return ret;
}

int bt_audio_codec_meta_get_stream_lang(const uint8_t meta[], size_t meta_len)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_STREAM_LANG,
					  &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (ret != 3) { /* Stream language is 3 octets */
		return -EBADMSG;
	}

	return sys_get_le24(data);
}

int bt_audio_codec_meta_get_ccid_list(const uint8_t meta[], size_t meta_len,
				      const uint8_t **ccid_list)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	CHECKIF(ccid_list == NULL) {
		LOG_DBG("ccid_list is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_CCID_LIST, &data);
	if (data == NULL) {
		return -ENODATA;
	}

	*ccid_list = data;

	return ret;
}

int bt_audio_codec_meta_get_parental_rating(const uint8_t meta[], size_t meta_len)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
					  &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (ret != sizeof(uint8_t)) {
		return -EBADMSG;
	}

	return data[0];
}

int bt_audio_codec_meta_get_program_info_uri(const uint8_t meta[], size_t meta_len,
					     const uint8_t **program_info_uri)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	CHECKIF(program_info_uri == NULL) {
		LOG_DBG("program_info_uri is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI,
					  &data);
	if (data == NULL) {
		return -ENODATA;
	}

	*program_info_uri = data;

	return ret;
}

int bt_audio_codec_meta_get_audio_active_state(const uint8_t meta[], size_t meta_len)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_AUDIO_STATE,
					  &data);
	if (data == NULL) {
		return -ENODATA;
	}

	if (ret != sizeof(uint8_t)) {
		return -EBADMSG;
	}

	return data[0];
}

int bt_audio_codec_meta_get_broadcast_audio_immediate_rendering_flag(const uint8_t meta[],
								     size_t meta_len)
{
	const uint8_t *data;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	return bt_audio_codec_meta_get_val(meta, meta_len,
					   BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE, &data);
}

int bt_audio_codec_meta_get_extended(const uint8_t meta[], size_t meta_len,
				     const uint8_t **extended_meta)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	CHECKIF(extended_meta == NULL) {
		LOG_DBG("extended_meta is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_EXTENDED, &data);
	if (data == NULL) {
		return -ENODATA;
	}

	*extended_meta = data;

	return ret;
}

int bt_audio_codec_meta_get_vendor(const uint8_t meta[], size_t meta_len,
				   const uint8_t **vendor_meta)
{
	const uint8_t *data;
	int ret;

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");
		return -EINVAL;
	}

	CHECKIF(vendor_meta == NULL) {
		LOG_DBG("vendor_meta is NULL");
		return -EINVAL;
	}

	ret = bt_audio_codec_meta_get_val(meta, meta_len, BT_AUDIO_METADATA_TYPE_VENDOR, &data);
	if (data == NULL) {
		return -ENODATA;
	}

	*vendor_meta = data;

	return ret;
}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 ||                                       \
	* CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0                                          \
	*/
