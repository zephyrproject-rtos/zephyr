/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include "bap_common.h"

#define VS_CODEC_CID 0x05F1 /* Linux foundation*/
#define VS_CODEC_VID 0x1234 /* any value*/

/* Vendor specific codec configuration with some random values */
struct bt_audio_codec_cfg vs_codec_cfg = {
	.path_id = BT_ISO_DATA_PATH_HCI,
	.ctlr_transcode = false,
	.id = BT_HCI_CODING_FORMAT_VS,
	.cid = VS_CODEC_CID,
	.vid = VS_CODEC_VID,
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	.data_len = 5,
	.data = {1, 2, 3, 4, 5}, /* any value */
#endif                           /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	.meta_len = 5,
	.meta = {10, 20, 30, 40, 50}, /* any value */
#endif                                /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 */
};

/* Vendor specific codec configuration with some random values */
struct bt_audio_codec_cap vs_codec_cap = {
	.path_id = BT_ISO_DATA_PATH_HCI,
	.ctlr_transcode = false,
	.id = BT_HCI_CODING_FORMAT_VS,
	.cid = VS_CODEC_CID,
	.vid = VS_CODEC_VID,
#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0
	.data_len = 5,
	.data = {1, 2, 3, 4, 5}, /* any value */
#endif                           /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE > 0
	.meta_len = 5,
	.meta = {10, 20, 30, 40, 50}, /* any value */
#endif                                /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 */
};

void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

struct print_ltv_info {
	const char *str;
	size_t cnt;
};

static bool print_ltv_elem(struct bt_data *data, void *user_data)
{
	struct print_ltv_info *ltv_info = user_data;

	printk("%s #%zu: type 0x%02x value_len %u", ltv_info->str, ltv_info->cnt, data->type,
	       data->data_len);
	print_hex(data->data, data->data_len);
	printk("\n");

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
	printk("codec_cap ID 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cap->id,
	       codec_cap->cid, codec_cap->vid, codec_cap->data_len);

	if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array("data", codec_cap->data, codec_cap->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		printk("data: ");
		print_hex(codec_cap->data, codec_cap->data_len);
		printk("\n");
	}

	print_ltv_array("meta", codec_cap->meta, codec_cap->meta_len);
}

void print_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg)
{
	printk("codec_cfg ID 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cfg->id,
	       codec_cfg->cid, codec_cfg->vid, codec_cfg->data_len);

	if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		print_ltv_array("data", codec_cfg->data, codec_cfg->data_len);
	} else { /* If not LC3, we cannot assume it's LTV */
		printk("data: ");
		print_hex(codec_cfg->data, codec_cfg->data_len);
		printk("\n");
	}

	print_ltv_array("meta", codec_cfg->meta, codec_cfg->meta_len);
}

void print_qos(const struct bt_bap_qos_cfg *qos)
{
	printk("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->interval, qos->framing, qos->phy, qos->sdu, qos->rtn, qos->latency, qos->pd);
}

void copy_unicast_stream_preset(struct unicast_stream *stream,
				const struct named_lc3_preset *named_preset)
{
	memcpy(&stream->qos, &named_preset->preset.qos, sizeof(stream->qos));
	memcpy(&stream->codec_cfg, &named_preset->preset.codec_cfg, sizeof(stream->codec_cfg));
}
