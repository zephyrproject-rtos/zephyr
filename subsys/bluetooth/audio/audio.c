/* Common functions for LE Audio services */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

#include "audio_internal.h"

LOG_MODULE_REGISTER(bt_audio, CONFIG_BT_AUDIO_LOG_LEVEL);

int bt_audio_data_parse(const uint8_t ltv[], size_t size,
			bool (*func)(struct bt_data *data, void *user_data), void *user_data)
{
	CHECKIF(ltv == NULL) {
		LOG_DBG("ltv is NULL");

		return -EINVAL;
	}

	CHECKIF(func == NULL) {
		LOG_DBG("func is NULL");

		return -EINVAL;
	}

	for (size_t i = 0; i < size;) {
		const uint8_t len = ltv[i];
		struct bt_data data;

		if (i + len > size || len < sizeof(data.type)) {
			LOG_DBG("Invalid len %u at i = %zu", len, i);

			return -EINVAL;
		}

		i++; /* Increment as we have parsed the len field */

		data.type = ltv[i++];
		data.data_len = len - sizeof(data.type);

		if (data.data_len > 0) {
			data.data = &ltv[i];
		} else {
			data.data = NULL;
		}

		if (!func(&data, user_data)) {
			return -ECANCELED;
		}

		/* Since we are incrementing i by the value_len, we don't need to increment it
		 * further in the `for` statement
		 */
		i += data.data_len;
	}

	return 0;
}

#if defined(CONFIG_BT_CONN)

static uint8_t bt_audio_security_check(const struct bt_conn *conn)
{
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err < 0) {
		return BT_ATT_ERR_UNLIKELY;
	}

	/* Require an encryption key with at least 128 bits of entropy, derived from SC or OOB
	 * method.
	 */
	if ((info.security.flags & (BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC)) == 0) {
		/* If the client has insufficient security to read/write the requested attribute
		 * then an ATT_ERROR_RSP PDU shall be sent with the Error Code parameter set to
		 * Insufficient Authentication (0x05).
		 */
		return BT_ATT_ERR_AUTHENTICATION;
	}

	if (info.security.enc_key_size < BT_ENC_KEY_SIZE_MAX) {
		return BT_ATT_ERR_ENCRYPTION_KEY_SIZE;
	}

	return BT_ATT_ERR_SUCCESS;
}

ssize_t bt_audio_read_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	const struct bt_audio_attr_user_data *user_data = attr->user_data;

	if (user_data->read == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	if (conn != NULL) {
		uint8_t err;

		err = bt_audio_security_check(conn);
		if (err != 0) {
			return BT_GATT_ERR(err);
		}
	}

	return user_data->read(conn, attr, buf, len, offset);
}

ssize_t bt_audio_write_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const struct bt_audio_attr_user_data *user_data = attr->user_data;

	if (user_data->write == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (conn != NULL) {
		uint8_t err;

		err = bt_audio_security_check(conn);
		if (err != 0) {
			return BT_GATT_ERR(err);
		}
	}

	return user_data->write(conn, attr, buf, len, offset, flags);
}

ssize_t bt_audio_ccc_cfg_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       uint16_t value)
{
	if (conn != NULL) {
		uint8_t err;

		err = bt_audio_security_check(conn);
		if (err != 0) {
			return BT_GATT_ERR(err);
		}
	}

	return sizeof(value);
}
#endif /* CONFIG_BT_CONN */

/* Broadcast sink depends on Scan Delegator, so we can just guard it with the Scan Delegator */
#if defined(CONFIG_BT_BAP_SCAN_DELEGATOR)

static int decode_bis_data(struct net_buf_simple *buf, struct bt_bap_base_bis_data *bis)
{
	uint8_t len;

	if (buf->len < BT_BAP_BASE_BIS_DATA_MIN_SIZE) {
		LOG_DBG("Not enough bytes (%u) to decode BIS data", buf->len);

		return -ENOMEM;
	}

	bis->index = net_buf_simple_pull_u8(buf);
	if (!IN_RANGE(bis->index, BT_ISO_BIS_INDEX_MIN, BT_ISO_BIS_INDEX_MAX)) {
		LOG_DBG("Invalid BIS index %u", bis->index);

		return -EINVAL;
	}

	/* codec config data length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid BIS specific codec config data length: %u (buf is %u)", len,
			buf->len);

		return -EMSGSIZE;
	}

	if (len > 0) {
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
		void *ltv_data;

		if (len > sizeof(bis->data)) {
			LOG_DBG("Cannot store codec config data of length %u", len);

			return -ENOMEM;
		}

		ltv_data = net_buf_simple_pull_mem(buf, len);

		bis->data_len = len;
		memcpy(bis->data, ltv_data, len);
#else  /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE == 0 */
		LOG_DBG("Cannot store codec config data");

		return -ENOMEM;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE */
	}

	return 0;
}

static int decode_subgroup(struct net_buf_simple *buf, struct bt_bap_base_subgroup *subgroup)
{
	struct bt_audio_codec_cfg *codec_cfg;
	uint8_t len;

	codec_cfg = &subgroup->codec_cfg;

	subgroup->bis_count = net_buf_simple_pull_u8(buf);
	if (subgroup->bis_count > ARRAY_SIZE(subgroup->bis_data)) {
		LOG_DBG("BASE has more BIS %u than we support %u", subgroup->bis_count,
			(uint8_t)ARRAY_SIZE(subgroup->bis_data));

		return -ENOMEM;
	}

	codec_cfg->id = net_buf_simple_pull_u8(buf);
	codec_cfg->cid = net_buf_simple_pull_le16(buf);
	codec_cfg->vid = net_buf_simple_pull_le16(buf);

	/* codec configuration data length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid codec config data length: %u (buf is %u)", len, buf->len);

		return -EINVAL;
	}

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	void *cfg_ltv_data;

	if (len > sizeof(subgroup->codec_cfg.data)) {
		LOG_DBG("Cannot store codec config data of length %u", len);

		return -ENOMEM;
	}

	cfg_ltv_data = net_buf_simple_pull_mem(buf, len);

	subgroup->codec_cfg.data_len = len;
	memcpy(subgroup->codec_cfg.data, cfg_ltv_data, len);
#else  /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE == 0 */
	if (len > 0) {
		LOG_DBG("Cannot store codec config data of length %u", len);

		return -ENOMEM;
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

	/* codec metadata length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid codec config data length: %u (buf is %u)", len, buf->len);

		return -EMSGSIZE;
	}

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	void *meta_ltv_data;

	if (len > sizeof(subgroup->codec_cfg.meta)) {
		LOG_DBG("Cannot store codec config meta of length %u", len);

		return -ENOMEM;
	}

	meta_ltv_data = net_buf_simple_pull_mem(buf, len);

	subgroup->codec_cfg.meta_len = len;
	memcpy(subgroup->codec_cfg.meta, meta_ltv_data, len);
#else  /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE == 0 */
	if (len > 0) {
		LOG_DBG("Cannot store metadata");

		return -ENOMEM;
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE */

	for (size_t i = 0U; i < subgroup->bis_count; i++) {
		const int err = decode_bis_data(buf, &subgroup->bis_data[i]);

		if (err != 0) {
			LOG_DBG("Failed to decode BIS data for bis[%zu]: %d", i, err);

			return err;
		}
	}

	return 0;
}

int bt_bap_decode_base(struct bt_data *data, struct bt_bap_base *base)
{
	struct bt_uuid_16 broadcast_uuid;
	struct net_buf_simple net_buf;
	void *uuid;

	CHECKIF(data == NULL) {
		LOG_DBG("data is NULL");

		return -EINVAL;
	}

	CHECKIF(base == NULL) {
		LOG_DBG("base is NULL");

		return -EINVAL;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		LOG_DBG("Invalid type: %u", data->type);

		return -ENOMSG;
	}

	if (data->data_len < BT_BAP_BASE_MIN_SIZE) {
		return -EMSGSIZE;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)data->data,
				      data->data_len);

	uuid = net_buf_simple_pull_mem(&net_buf, BT_UUID_SIZE_16);
	if (!bt_uuid_create(&broadcast_uuid.uuid, uuid, BT_UUID_SIZE_16)) {
		LOG_ERR("bt_uuid_create failed");

		return -EINVAL;
	}

	if (bt_uuid_cmp(&broadcast_uuid.uuid, BT_UUID_BASIC_AUDIO) != 0) {
		LOG_DBG("Invalid UUID");

		return -ENOMSG;
	}

	base->pd = net_buf_simple_pull_le24(&net_buf);
	base->subgroup_count = net_buf_simple_pull_u8(&net_buf);

	if (base->subgroup_count > ARRAY_SIZE(base->subgroups)) {
		LOG_DBG("Cannot decode BASE with %u subgroups (max supported is %zu)",
			base->subgroup_count, ARRAY_SIZE(base->subgroups));

		return -ENOMEM;
	}

	for (size_t i = 0U; i < base->subgroup_count; i++) {
		const int err = decode_subgroup(&net_buf, &base->subgroups[i]);

		if (err != 0) {
			LOG_DBG("Failed to decode subgroup[%zu]: %d", i, err);

			return err;
		}
	}

	return 0;
}
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR */
