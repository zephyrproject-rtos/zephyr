/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include "bap_unicast_common.h"

void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

void print_codec_cap(const struct bt_audio_codec_cap *codec_cap)
{
	printk("codec_cfg ID 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cap->id,
	       codec_cap->cid, codec_cap->vid, codec_cap->data_count);

	for (uint8_t i = 0; i < codec_cap->data_count; i++) {
		printk("data #%u: type 0x%02x len %u\n", i, codec_cap->data[i].data.type,
		       codec_cap->data[i].data.data_len);
		print_hex(codec_cap->data[i].data.data,
			  codec_cap->data[i].data.data_len - sizeof(codec_cap->data[i].data.type));
		printk("\n");
	}

	for (uint8_t i = 0; i < codec_cap->meta_count; i++) {
		printk("meta #%u: type 0x%02x len %u\n", i, codec_cap->meta[i].data.type,
		       codec_cap->meta[i].data.data_len);
		print_hex(codec_cap->meta[i].data.data,
			  codec_cap->meta[i].data.data_len - sizeof(codec_cap->meta[i].data.type));
		printk("\n");
	}
}

void print_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg)
{
	printk("codec_cap ID 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cfg->id,
	       codec_cfg->cid, codec_cfg->vid, codec_cfg->data_count);

	for (uint8_t i = 0; i < codec_cfg->data_count; i++) {
		printk("data #%u: type 0x%02x len %u\n",
		       i, codec_cfg->data[i].data.type,
		       codec_cfg->data[i].data.data_len);
		print_hex(codec_cfg->data[i].data.data,
			  codec_cfg->data[i].data.data_len -
				sizeof(codec_cfg->data[i].data.type));
		printk("\n");
	}

	for (uint8_t i = 0; i < codec_cfg->meta_count; i++) {
		printk("meta #%u: type 0x%02x len %u\n",
		       i, codec_cfg->meta[i].data.type,
		       codec_cfg->meta[i].data.data_len);
		print_hex(codec_cfg->meta[i].data.data,
			  codec_cfg->meta[i].data.data_len -
				sizeof(codec_cfg->meta[i].data.type));
		printk("\n");
	}
}

void print_qos(const struct bt_audio_codec_qos *qos)
{
	printk("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);
}
