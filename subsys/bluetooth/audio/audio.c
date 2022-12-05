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

static int decode_codec_ltv(struct net_buf_simple *buf,
			    struct bt_codec_data *codec_data)
{
	if (buf->len < sizeof(codec_data->data.data_len)) {
		LOG_DBG("Not enough data for LTV length field: %u", buf->len);

		return -ENOMEM;
	}

	codec_data->data.data_len = net_buf_simple_pull_u8(buf);

	if (buf->len < sizeof(codec_data->data.type)) {
		LOG_DBG("Not enough data for LTV type field: %u", buf->len);

		return -EMSGSIZE;
	}

	/* LTV structures include the data.type in the length field,
	 * but we do not do that for the bt_data struct in Zephyr
	 */
	codec_data->data.data_len -= sizeof(codec_data->data.type);

	codec_data->data.type = net_buf_simple_pull_u8(buf);
#if CONFIG_BT_CODEC_MAX_DATA_LEN > 0
	void *value;

	codec_data->data.data = codec_data->value;

	if (buf->len < codec_data->data.data_len) {
		LOG_DBG("Not enough data for LTV value field: %u/%zu", buf->len,
			codec_data->data.data_len);

		return -EMSGSIZE;
	}

	value = net_buf_simple_pull_mem(buf, codec_data->data.data_len);
	(void)memcpy(codec_data->value, value, codec_data->data.data_len);
#else /* CONFIG_BT_CODEC_MAX_DATA_LEN == 0 */
	if (codec_data->data.data_len > 0) {
		LOG_DBG("Cannot store data");

		return -ENOMEM;
	}

	codec_data->data.data = NULL;
#endif /* CONFIG_BT_CODEC_MAX_DATA_LEN > 0 */

	return 0;
}

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
#if CONFIG_BT_CODEC_MAX_DATA_LEN > 0
		struct net_buf_simple ltv_buf;
		void *ltv_data;

		/* Use an extra net_buf_simple to be able to decode until it
		 * is empty (len = 0)
		 */
		ltv_data = net_buf_simple_pull_mem(buf, len);
		net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);


		while (ltv_buf.len != 0) {
			struct bt_codec_data *bis_codec_data;
			int err;

			if (bis->data_count >= ARRAY_SIZE(bis->data)) {
				LOG_WRN("BIS data overflow; discarding");
				break;
			}

			bis_codec_data = &bis->data[bis->data_count];

			err = decode_codec_ltv(&ltv_buf, bis_codec_data);
			if (err != 0) {
				LOG_DBG("Failed to decode BIS config data for entry[%u]: %d",
					bis->data_count, err);

				return err;
			}

			bis->data_count++;
		}
#else /* CONFIG_BT_CODEC_MAX_DATA_LEN == 0 */
		LOG_DBG("Cannot store codec config data");

		return -ENOMEM;
#endif /* CONFIG_BT_CODEC_MAX_DATA_LEN */
	}

	return 0;
}

static int decode_subgroup(struct net_buf_simple *buf, struct bt_bap_base_subgroup *subgroup)
{
	struct net_buf_simple ltv_buf;
	struct bt_codec	*codec;
	void *ltv_data;
	uint8_t len;

	codec = &subgroup->codec;

	subgroup->bis_count = net_buf_simple_pull_u8(buf);
	if (subgroup->bis_count > ARRAY_SIZE(subgroup->bis_data)) {
		LOG_DBG("BASE has more BIS %u than we support %u", subgroup->bis_count,
			(uint8_t)ARRAY_SIZE(subgroup->bis_data));

		return -ENOMEM;
	}

	codec->id = net_buf_simple_pull_u8(buf);
	codec->cid = net_buf_simple_pull_le16(buf);
	codec->vid = net_buf_simple_pull_le16(buf);

	/* codec configuration data length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid codec config data length: %u (buf is %u)", len, buf->len);

		return -EINVAL;
	}

#if CONFIG_BT_CODEC_MAX_DATA_COUNT > 0
	/* Use an extra net_buf_simple to be able to decode until it
	 * is empty (len = 0)
	 */
	ltv_data = net_buf_simple_pull_mem(buf, len);
	net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

	/* The loop below is very similar to codec_config_store with notable
	 * exceptions that it can do early termination, and also does not log
	 * every LTV entry, which would simply be too much for handling
	 * broadcasted BASEs
	 */
	while (ltv_buf.len != 0) {
		struct bt_codec_data *codec_data;
		int err;

		if (codec->data_count >= ARRAY_SIZE(codec->data)) {
			LOG_WRN("BIS codec data overflow; discarding");
			break;
		}

		codec_data = &codec->data[codec->data_count];

		err = decode_codec_ltv(&ltv_buf, codec_data);
		if (err != 0) {
			LOG_DBG("Failed to decode codec config data for entry %u: %d",
				codec->data_count, err);

			return err;
		}

		codec->data_count++;
	}

	if (buf->len < sizeof(len)) {
		LOG_DBG("Cannot store BASE in buffer");

		return -ENOMEM;
	}
#else /* CONFIG_BT_CODEC_MAX_DATA_COUNT == 0 */
	if (len > 0) {
		LOG_DBG("Cannot store codec config data");

		return -ENOMEM;
	}
#endif /* CONFIG_BT_CODEC_MAX_DATA_COUNT */

	/* codec metadata length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid codec config data length: %u (buf is %u)", len, buf->len);

		return -EMSGSIZE;
	}

#if CONFIG_BT_CODEC_MAX_METADATA_COUNT > 0
	/* Use an extra net_buf_simple to be able to decode until it
	 * is empty (len = 0)
	 */
	ltv_data = net_buf_simple_pull_mem(buf, len);
	net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

	/* The loop below is very similar to codec_config_store with notable
	 * exceptions that it can do early termination, and also does not log
	 * every LTV entry, which would simply be too much for handling
	 * broadcasted BASEs
	 */
	while (ltv_buf.len != 0) {
		struct bt_codec_data *metadata;
		int err;

		if (codec->meta_count >= ARRAY_SIZE(codec->meta)) {
			LOG_WRN("BIS codec metadata overflow; discarding");
			break;
		}

		metadata = &codec->meta[codec->meta_count];

		err = decode_codec_ltv(&ltv_buf, metadata);
		if (err != 0) {
			LOG_DBG("Failed to decode codec metadata for entry %u: %d",
				codec->meta_count, err);

			return err;
		}

		codec->meta_count++;
	}
#else /* CONFIG_BT_CODEC_MAX_METADATA_COUNT == 0 */
	if (len > 0) {
		LOG_DBG("Cannot store metadata");

		return -ENOMEM;
	}
#endif /* CONFIG_BT_CODEC_MAX_METADATA_COUNT */

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

ssize_t bt_audio_codec_data_to_buf(const struct bt_codec_data *codec_data, size_t count,
				   uint8_t *buf, size_t buf_size)
{
	size_t total_len = 0;

	for (size_t i = 0; i < count; i++) {
		const struct bt_data *ltv = &codec_data[i].data;
		const uint8_t length = ltv->data_len;
		const uint8_t type = ltv->type;
		const uint8_t *value = ltv->data;
		const size_t ltv_len = sizeof(length) + sizeof(type) + length;

		/* Verify that the buffer can hold the next LTV structure */
		if (buf_size < total_len + ltv_len) {
			return -ENOMEM;
		}

		/* Copy data */
		buf[total_len++] = length + sizeof(type);
		buf[total_len++] = type;
		(void)memcpy(&buf[total_len], value, length);

		total_len += length;
	}

	return total_len;
}
