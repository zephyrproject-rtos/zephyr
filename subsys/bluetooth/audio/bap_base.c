/* bap_base.c - BAP BASE handling */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(bt_bap_base, CONFIG_BT_BAP_BASE_LOG_LEVEL);

/* The BASE and the following defines are defined by BAP v1.0.1, section 3.7.2.2 Basic Audio
 * Announcements
 */
#define BASE_MAX_SIZE            (UINT8_MAX - 1 /* type */ - BT_UUID_SIZE_16)
#define BASE_CODEC_ID_SIZE       (1 /* id */ + 2 /* cid */ + 2 /* vid */)
#define BASE_PD_SIZE             3
#define BASE_SUBGROUP_COUNT_SIZE 1
#define BASE_NUM_BIS_SIZE        1
#define BASE_CC_LEN_SIZE         1
#define BASE_META_LEN_SIZE       1
#define BASE_BIS_INDEX_SIZE      1
#define BASE_BIS_CC_LEN_SIZE     1
#define BASE_SUBGROUP_MAX_SIZE   (BASE_MAX_SIZE - BASE_PD_SIZE - BASE_SUBGROUP_COUNT_SIZE)
#define BASE_SUBGROUP_MIN_SIZE                                                                     \
	(BASE_NUM_BIS_SIZE + BASE_CODEC_ID_SIZE + BASE_CC_LEN_SIZE + BASE_META_LEN_SIZE +          \
	 BASE_BIS_INDEX_SIZE + BASE_BIS_CC_LEN_SIZE)
#define BASE_MIN_SIZE                                                                              \
	(BT_UUID_SIZE_16 + BASE_PD_SIZE + BASE_SUBGROUP_COUNT_SIZE + BASE_SUBGROUP_MIN_SIZE)
#define BASE_SUBGROUP_MAX_COUNT                                                                    \
	((BASE_MAX_SIZE - BASE_PD_SIZE - BASE_SUBGROUP_COUNT_SIZE) / BASE_SUBGROUP_MIN_SIZE)

static uint32_t base_pull_pd(struct net_buf_simple *net_buf)
{
	return net_buf_simple_pull_le24(net_buf);
}

static uint8_t base_pull_bis_count(struct net_buf_simple *net_buf)
{
	return net_buf_simple_pull_u8(net_buf);
}

static void base_pull_codec_id(struct net_buf_simple *net_buf,
			       struct bt_bap_base_codec_id *codec_id)
{
	struct bt_bap_base_codec_id codec;

	codec.id = net_buf_simple_pull_u8(net_buf);    /* coding format */
	codec.cid = net_buf_simple_pull_le16(net_buf); /* company id */
	codec.vid = net_buf_simple_pull_le16(net_buf); /* VS codec id */

	if (codec_id != NULL) {
		*codec_id = codec;
	}
}

static uint8_t base_pull_ltv(struct net_buf_simple *net_buf, uint8_t **data)
{
	const uint8_t len = net_buf_simple_pull_u8(net_buf);

	if (data == NULL) {
		net_buf_simple_pull_mem(net_buf, len);
	} else {
		*data = net_buf_simple_pull_mem(net_buf, len);
	}

	return len;
}

static bool check_pull_ltv(struct net_buf_simple *net_buf)
{
	uint8_t ltv_len;

	if (net_buf->len < sizeof(ltv_len)) {
		return false;
	}

	ltv_len = net_buf_simple_pull_u8(net_buf);
	if (net_buf->len < ltv_len) {
		return false;
	}
	net_buf_simple_pull_mem(net_buf, ltv_len);

	return true;
}

const struct bt_bap_base *bt_bap_base_get_base_from_ad(const struct bt_data *ad)
{
	struct bt_uuid_16 broadcast_uuid;
	const struct bt_bap_base *base;
	struct net_buf_simple net_buf;
	uint8_t subgroup_count;
	void *uuid;

	CHECKIF(ad == NULL) {
		LOG_DBG("data is NULL");

		return NULL;
	}

	if (ad->type != BT_DATA_SVC_DATA16) {
		LOG_DBG("Invalid type: %u", ad->type);

		return NULL;
	}

	if (ad->data_len < BASE_MIN_SIZE) {
		LOG_DBG("Invalid len: %u", ad->data_len);

		return NULL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)ad->data, ad->data_len);

	uuid = net_buf_simple_pull_mem(&net_buf, BT_UUID_SIZE_16);
	if (!bt_uuid_create(&broadcast_uuid.uuid, uuid, BT_UUID_SIZE_16)) {
		LOG_ERR("bt_uuid_create failed");

		return NULL;
	}

	if (bt_uuid_cmp(&broadcast_uuid.uuid, BT_UUID_BASIC_AUDIO) != 0) {
		LOG_DBG("Invalid UUID");

		return NULL;
	}

	/* Store the start of the BASE */
	base = (const struct bt_bap_base *)net_buf.data;

	/* Pull all data to verify that the result BASE is valid */
	base_pull_pd(&net_buf);
	subgroup_count = net_buf_simple_pull_u8(&net_buf);
	if (subgroup_count == 0 || subgroup_count > BASE_SUBGROUP_MAX_COUNT) {
		LOG_DBG("Invalid subgroup count: %u", subgroup_count);

		return NULL;
	}

	for (uint8_t i = 0U; i < subgroup_count; i++) {
		uint8_t bis_count;

		if (net_buf.len < sizeof(bis_count)) {
			LOG_DBG("Invalid BASE length: %u", ad->data_len);

			return NULL;
		}

		bis_count = base_pull_bis_count(&net_buf);
		if (bis_count == 0 || bis_count > BT_ISO_MAX_GROUP_ISO_COUNT) {
			LOG_DBG("Subgroup[%u]: Invalid BIS count: %u", i, bis_count);

			return NULL;
		}

		if (net_buf.len < BASE_CODEC_ID_SIZE) {
			LOG_DBG("Invalid BASE length: %u", ad->data_len);

			return NULL;
		}

		base_pull_codec_id(&net_buf, NULL);

		/* Pull CC */
		if (!check_pull_ltv(&net_buf)) {
			LOG_DBG("Invalid BASE length: %u", ad->data_len);

			return NULL;
		}

		/* Pull meta */
		if (!check_pull_ltv(&net_buf)) {
			LOG_DBG("Invalid BASE length: %u", ad->data_len);

			return NULL;
		}

		for (uint8_t j = 0U; j < bis_count; j++) {
			uint8_t bis_index;

			if (net_buf.len < sizeof(bis_index)) {
				LOG_DBG("Invalid BASE length: %u", ad->data_len);

				return NULL;
			}

			bis_index = net_buf_simple_pull_u8(&net_buf);
			if (bis_index == 0 || bis_index > BT_ISO_BIS_INDEX_MAX) {
				LOG_DBG("Subgroup[%u]: Invalid BIS index: %u", i, bis_index);

				return NULL;
			}

			/* Pull BIS CC data */
			if (!check_pull_ltv(&net_buf)) {
				LOG_DBG("Invalid BASE length: %u", ad->data_len);

				return NULL;
			}
		}
	}

	return base;
}

int bt_bap_base_get_size(const struct bt_bap_base *base)
{
	struct net_buf_simple net_buf;
	uint8_t subgroup_count;
	size_t size = 0;

	CHECKIF(base == NULL) {
		LOG_DBG("base is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)base, BASE_MAX_SIZE);
	base_pull_pd(&net_buf);
	size += BASE_PD_SIZE;
	subgroup_count = net_buf_simple_pull_u8(&net_buf);
	size += BASE_SUBGROUP_COUNT_SIZE;

	/* Parse subgroup data */
	for (uint8_t i = 0U; i < subgroup_count; i++) {
		uint8_t bis_count;
		uint8_t len;

		bis_count = base_pull_bis_count(&net_buf);
		size += BASE_NUM_BIS_SIZE;

		base_pull_codec_id(&net_buf, NULL);
		size += BASE_CODEC_ID_SIZE;

		/* Codec config */
		len = base_pull_ltv(&net_buf, NULL);
		size += len + BASE_CC_LEN_SIZE;

		/* meta */
		len = base_pull_ltv(&net_buf, NULL);
		size += len + BASE_META_LEN_SIZE;

		/* Parse BIS data */
		for (uint8_t j = 0U; j < bis_count; j++) {
			/* BIS index */
			net_buf_simple_pull_u8(&net_buf);
			size += BASE_BIS_INDEX_SIZE;

			/* Codec config */
			len = base_pull_ltv(&net_buf, NULL);
			size += len + BASE_BIS_CC_LEN_SIZE;
		}
	}

	return (int)size;
}

int bt_bap_base_get_pres_delay(const struct bt_bap_base *base)
{
	struct net_buf_simple net_buf;
	uint32_t pd;

	CHECKIF(base == NULL) {
		LOG_DBG("base is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)base, sizeof(pd));
	pd = base_pull_pd(&net_buf);

	return (int)pd; /* PD is 24-bit so it fits in an int */
}

int bt_bap_base_get_subgroup_count(const struct bt_bap_base *base)
{
	struct net_buf_simple net_buf;
	uint8_t subgroup_count;

	CHECKIF(base == NULL) {
		LOG_DBG("base is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)base, BASE_MAX_SIZE);
	base_pull_pd(&net_buf);
	subgroup_count = net_buf_simple_pull_u8(&net_buf);

	return (int)subgroup_count; /* subgroup_count is 8-bit so it fits in an int */
}

int bt_bap_base_foreach_subgroup(const struct bt_bap_base *base,
				 bool (*func)(const struct bt_bap_base_subgroup *data,
					      void *user_data),
				 void *user_data)
{
	struct bt_bap_base_subgroup *subgroup;
	struct net_buf_simple net_buf;
	uint8_t subgroup_count;

	CHECKIF(base == NULL) {
		LOG_DBG("base is NULL");

		return -EINVAL;
	}

	CHECKIF(func == NULL) {
		LOG_DBG("func is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)base, BASE_MAX_SIZE);
	base_pull_pd(&net_buf);
	subgroup_count = net_buf_simple_pull_u8(&net_buf);

	for (uint8_t i = 0U; i < subgroup_count; i++) {
		subgroup = (struct bt_bap_base_subgroup *)net_buf.data;
		if (!func(subgroup, user_data)) {
			LOG_DBG("user stopped parsing");

			return -ECANCELED;
		}

		/* Parse subgroup data to get next subgroup pointer */
		if (subgroup_count > 1) { /* Only parse data if it isn't the last one */
			uint8_t bis_count;

			bis_count = base_pull_bis_count(&net_buf);
			base_pull_codec_id(&net_buf, NULL);

			/* Codec config */
			base_pull_ltv(&net_buf, NULL);

			/* meta */
			base_pull_ltv(&net_buf, NULL);

			for (uint8_t j = 0U; j < bis_count; j++) {
				net_buf_simple_pull_u8(&net_buf); /* index */

				/* Codec config */
				base_pull_ltv(&net_buf, NULL);
			}
		}
	}

	return 0;
}

int bt_bap_base_get_subgroup_codec_id(const struct bt_bap_base_subgroup *subgroup,
				      struct bt_bap_base_codec_id *codec_id)
{
	struct net_buf_simple net_buf;

	CHECKIF(subgroup == NULL) {
		LOG_DBG("subgroup is NULL");

		return -EINVAL;
	}

	CHECKIF(codec_id == NULL) {
		LOG_DBG("codec_id is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)subgroup, BASE_SUBGROUP_MAX_SIZE);
	base_pull_bis_count(&net_buf);
	base_pull_codec_id(&net_buf, codec_id);

	return 0;
}

int bt_bap_base_get_subgroup_codec_data(const struct bt_bap_base_subgroup *subgroup, uint8_t **data)
{
	struct net_buf_simple net_buf;

	CHECKIF(subgroup == NULL) {
		LOG_DBG("subgroup is NULL");

		return -EINVAL;
	}

	CHECKIF(data == NULL) {
		LOG_DBG("data is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)subgroup, BASE_SUBGROUP_MAX_SIZE);
	base_pull_bis_count(&net_buf);
	base_pull_codec_id(&net_buf, NULL);

	/* Codec config */
	return base_pull_ltv(&net_buf, data);
}

int bt_bap_base_get_subgroup_codec_meta(const struct bt_bap_base_subgroup *subgroup, uint8_t **meta)
{
	struct net_buf_simple net_buf;

	CHECKIF(subgroup == NULL) {
		LOG_DBG("subgroup is NULL");

		return -EINVAL;
	}

	CHECKIF(meta == NULL) {
		LOG_DBG("meta is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)subgroup, BASE_SUBGROUP_MAX_SIZE);
	base_pull_bis_count(&net_buf);
	base_pull_codec_id(&net_buf, NULL);

	/* Codec config */
	base_pull_ltv(&net_buf, NULL);

	/* meta */
	return base_pull_ltv(&net_buf, meta);
}

int bt_bap_base_subgroup_codec_to_codec_cfg(const struct bt_bap_base_subgroup *subgroup,
					    struct bt_audio_codec_cfg *codec_cfg)
{
	struct bt_bap_base_codec_id codec_id;
	struct net_buf_simple net_buf;
	uint8_t *ltv_data;
	uint8_t ltv_len;

	CHECKIF(subgroup == NULL) {
		LOG_DBG("subgroup is NULL");

		return -EINVAL;
	}

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec_cfg is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)subgroup, BASE_SUBGROUP_MAX_SIZE);
	base_pull_bis_count(&net_buf);
	base_pull_codec_id(&net_buf, &codec_id);

	codec_cfg->id = codec_id.id;
	codec_cfg->cid = codec_id.cid;
	codec_cfg->vid = codec_id.vid;

	/* Codec config */
	ltv_len = base_pull_ltv(&net_buf, &ltv_data);

	if (ltv_len > ARRAY_SIZE(codec_cfg->data)) {
		LOG_DBG("Cannot fit %u octets of codec data (max %zu)", ltv_len,
			ARRAY_SIZE(codec_cfg->data));

		return -ENOMEM;
	}

	codec_cfg->data_len = ltv_len;
	memcpy(codec_cfg->data, ltv_data, ltv_len);

	/* Meta */
	ltv_len = base_pull_ltv(&net_buf, &ltv_data);

	if (ltv_len > ARRAY_SIZE(codec_cfg->meta)) {
		LOG_DBG("Cannot fit %u octets of codec meta (max %zu)", ltv_len,
			ARRAY_SIZE(codec_cfg->meta));

		return -ENOMEM;
	}

	codec_cfg->meta_len = ltv_len;
	memcpy(codec_cfg->meta, ltv_data, ltv_len);

	return 0;
}
int bt_bap_base_get_subgroup_bis_count(const struct bt_bap_base_subgroup *subgroup)
{
	struct net_buf_simple net_buf;

	CHECKIF(subgroup == NULL) {
		LOG_DBG("subgroup is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)subgroup, BASE_SUBGROUP_MAX_SIZE);

	return base_pull_bis_count(&net_buf);
}

int bt_bap_base_subgroup_foreach_bis(const struct bt_bap_base_subgroup *subgroup,
				     bool (*func)(const struct bt_bap_base_subgroup_bis *subgroup,
						  void *user_data),
				     void *user_data)
{
	struct net_buf_simple net_buf;
	uint8_t bis_count;

	CHECKIF(subgroup == NULL) {
		LOG_DBG("subgroup is NULL");

		return -EINVAL;
	}

	CHECKIF(func == NULL) {
		LOG_DBG("func is NULL");

		return -EINVAL;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)subgroup, BASE_SUBGROUP_MAX_SIZE);

	bis_count = base_pull_bis_count(&net_buf);
	base_pull_codec_id(&net_buf, NULL);

	/* Codec config */
	base_pull_ltv(&net_buf, NULL);

	/* meta */
	base_pull_ltv(&net_buf, NULL);

	for (uint8_t i = 0U; i < bis_count; i++) {
		struct bt_bap_base_subgroup_bis bis;

		bis.index = net_buf_simple_pull_u8(&net_buf); /* index */

		/* Codec config */
		bis.data_len = base_pull_ltv(&net_buf, &bis.data);

		if (!func(&bis, user_data)) {
			LOG_DBG("user stopped parsing");

			return -ECANCELED;
		}
	}

	return 0;
}

int bt_bap_base_subgroup_bis_codec_to_codec_cfg(const struct bt_bap_base_subgroup_bis *bis,
						struct bt_audio_codec_cfg *codec_cfg)
{
	CHECKIF(bis == NULL) {
		LOG_DBG("bis is NULL");

		return -EINVAL;
	}

	CHECKIF(codec_cfg == NULL) {
		LOG_DBG("codec_cfg is NULL");

		return -EINVAL;
	}

	if (bis->data_len > ARRAY_SIZE(codec_cfg->data)) {
		LOG_DBG("Cannot fit %u octets of codec data (max %zu)", bis->data_len,
			ARRAY_SIZE(codec_cfg->data));

		return -ENOMEM;
	}

	codec_cfg->data_len = bis->data_len;
	memcpy(codec_cfg->data, bis->data, bis->data_len);

	return 0;
}

static bool base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	uint32_t *base_bis_index_bitfield = user_data;

	*base_bis_index_bitfield |= BT_ISO_BIS_INDEX_BIT(bis->index);

	return true;
}

static bool base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	const int err = bt_bap_base_subgroup_foreach_bis(subgroup, base_subgroup_bis_cb, user_data);

	if (err != 0) {
		LOG_DBG("Failed to parse all BIS: %d", err);
		return false;
	}

	return true;
}

int bt_bap_base_subgroup_get_bis_indexes(const struct bt_bap_base_subgroup *subgroup,
					 uint32_t *bis_indexes)
{
	CHECKIF(subgroup == NULL) {
		LOG_DBG("subgroup is NULL");

		return -EINVAL;
	}

	CHECKIF(bis_indexes == NULL) {
		LOG_DBG("bis_indexes is NULL");

		return -EINVAL;
	}

	*bis_indexes = 0U;

	return bt_bap_base_subgroup_foreach_bis(subgroup, base_subgroup_bis_cb, bis_indexes);
}

int bt_bap_base_get_bis_indexes(const struct bt_bap_base *base, uint32_t *bis_indexes)
{
	CHECKIF(base == NULL) {
		LOG_DBG("base is NULL");

		return -EINVAL;
	}

	CHECKIF(bis_indexes == NULL) {
		LOG_DBG("bis_indexes is NULL");

		return -EINVAL;
	}

	*bis_indexes = 0U;

	return bt_bap_base_foreach_subgroup(base, base_subgroup_cb, bis_indexes);
}
