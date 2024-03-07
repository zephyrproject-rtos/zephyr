/** @file
 *  @brief Bluetooth audio shell generic functions
 *
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include "shell/bt.h"
#include "audio.h"

LOG_MODULE_REGISTER(audio_shell, LOG_LEVEL_DBG);

void print_qos(const struct bt_audio_codec_qos *qos)
{
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE) || defined(CONFIG_BT_BAP_UNICAST)
	LOG_DBG("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u latency %u pd %u",
		qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->latency, qos->pd);
#else
	LOG_DBG("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u rtn %u pd %u", qos->interval,
		qos->framing, qos->phy, qos->sdu, qos->rtn, qos->pd);
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE || CONFIG_BT_BAP_UNICAST */
}

struct print_ltv_info {
	const char *str;
	size_t cnt;
};

static bool print_ltv_elem(struct bt_data *data, void *user_data)
{
	struct print_ltv_info *ltv_info = user_data;

	LOG_DBG("%s #%zu: type 0x%02x value_len %u", ltv_info->str, ltv_info->cnt, data->type,
		data->data_len);
	LOG_HEXDUMP_DBG(data->data, data->data_len, "");

	ltv_info->cnt++;

	return true;
}

static void print_ltv_array(const char *str, const uint8_t *ltv_data, size_t ltv_data_len)
{
	struct print_ltv_info ltv_info = {
		.str = str,
		.cnt = 0U,
	};

	bt_audio_data_parse(ltv_data, ltv_data_len, print_ltv_elem, &ltv_info);
}

void print_codec_cap(const struct bt_audio_codec_cap *codec_cap)
{
	LOG_DBG("codec cap id 0x%02x cid 0x%04x vid 0x%04x count %u", codec_cap->id, codec_cap->cid,
		codec_cap->vid, codec_cap->data_len);

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0
	if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array("data", codec_cap->data, codec_cap->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		LOG_HEXDUMP_DBG(codec_cap->data, codec_cap->data_len, "data");
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0
	print_ltv_array("meta", codec_cap->meta, codec_cap->meta_len);
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0 */
}

void print_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg)
{
	LOG_DBG("codec cfg id 0x%02x cid 0x%04x vid 0x%04x count %u", codec_cfg->id, codec_cfg->cid,
		codec_cfg->vid, codec_cfg->data_len);

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array("data", codec_cfg->data, codec_cfg->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		LOG_HEXDUMP_DBG(codec_cfg->data, codec_cfg->data_len, "data");
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	print_ltv_array("meta", codec_cfg->meta, codec_cfg->meta_len);
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 */
}

bool print_base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	struct bt_bap_base_codec_id *codec_id = user_data;

	LOG_DBG("\t\tBIS index: 0x%02X", bis->index);
	/* Print CC data */
	if (codec_id->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array("\t\tdata", bis->data, bis->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		LOG_HEXDUMP_DBG(bis->data, bis->data_len, "data");
	}

	return true;
}

bool print_base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	struct bt_bap_base_codec_id codec_id;
	uint8_t *data;
	int ret;

	LOG_DBG("Subgroup %p:", (void *)subgroup);

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, &codec_id);
	if (ret < 0) {
		return false;
	}

	LOG_DBG("\tCodec Format: 0x%02X", codec_id.id);
	LOG_DBG("\tCompany ID  : 0x%04X", codec_id.cid);
	LOG_DBG("\tVendor ID   : 0x%04X", codec_id.vid);

	ret = bt_bap_base_get_subgroup_codec_data(subgroup, &data);
	if (ret < 0) {
		return false;
	}

	/* Print CC data */
	if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array("\tdata", data, (uint8_t)ret);
	} else { /* If not LC3, we cannot assume it's LTV */
		LOG_HEXDUMP_DBG(data, (uint8_t)ret, "\tdata");
	}

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &data);
	if (ret < 0) {
		return false;
	}

	/* Print metadata */
	if (codec_id.id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array("\tdata", data, (uint8_t)ret);
	} else { /* If not LC3, we cannot assume it's LTV */
		LOG_HEXDUMP_DBG(data, (uint8_t)ret, "\tdata");
	}

	ret = bt_bap_base_subgroup_foreach_bis(subgroup, print_base_subgroup_bis_cb, &codec_id);
	if (ret < 0) {
		return false;
	}

	return true;
}

void print_base(const struct bt_bap_base *base)
{
	int err;

	LOG_DBG("Presentation delay: %d", bt_bap_base_get_pres_delay(base));
	LOG_DBG("Subgroup count: %d", bt_bap_base_get_subgroup_count(base));

	err = bt_bap_base_foreach_subgroup(base, print_base_subgroup_cb, NULL);
	if (err < 0) {
		LOG_DBG("Invalid BASE: %d", err);
	}
}

void copy_unicast_stream_preset(struct shell_stream *stream,
				const struct named_lc3_preset *named_preset)
{
	memcpy(&stream->qos, &named_preset->preset.qos, sizeof(stream->qos));
	memcpy(&stream->codec_cfg, &named_preset->preset.codec_cfg, sizeof(stream->codec_cfg));
}

void copy_broadcast_source_preset(struct broadcast_source *source,
				  const struct named_lc3_preset *named_preset)
{
	memcpy(&source->qos, &named_preset->preset.qos, sizeof(source->qos));
	memcpy(&source->codec_cfg, &named_preset->preset.codec_cfg, sizeof(source->codec_cfg));
}
