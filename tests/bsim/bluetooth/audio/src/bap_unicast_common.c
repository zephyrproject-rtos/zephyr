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

static void print_ltv_elem(const char *str, uint8_t type, uint8_t value_len, const uint8_t *value,
			   size_t cnt)
{
	printk("%s #%zu: type 0x%02x value_len %u\n", str, cnt, type, value_len);
	print_hex(value, value_len);
	printk("\n");
}

static void print_ltv_array(const char *str, const uint8_t *ltv_data, size_t ltv_data_len)
{
	size_t cnt = 0U;

	for (size_t i = 0U; i < ltv_data_len;) {
		const uint8_t len = ltv_data[i++];
		const uint8_t type = ltv_data[i++];
		const uint8_t *value = &ltv_data[i];
		const uint8_t value_len = len - sizeof(type);

		print_ltv_elem(str, type, value_len, value, cnt++);
		/* Since we are incrementing i by the value_len, we don't need to increment it
		 * further in the `for` statement
		 */
		i += value_len;
	}
}

void print_codec_cap(const struct bt_audio_codec_cap *codec_cap)
{
	printk("codec_cap ID 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cap->id,
	       codec_cap->cid, codec_cap->vid, codec_cap->data_len);

	if (codec_cap->id == BT_AUDIO_CODEC_LC3_ID) {
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
