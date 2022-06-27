/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include "unicast_common.h"

void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

void print_codec(const struct bt_codec *codec)
{
	printk("codec 0x%02x cid 0x%04x vid 0x%04x count %u\n",
	       codec->id, codec->cid, codec->vid, codec->data_count);

	for (uint8_t i = 0; i < codec->data_count; i++) {
		printk("data #%u: type 0x%02x len %u\n",
		       i, codec->data[i].data.type,
		       codec->data[i].data.data_len);
		print_hex(codec->data[i].data.data,
			  codec->data[i].data.data_len -
				sizeof(codec->data[i].data.type));
		printk("\n");
	}

	for (uint8_t i = 0; i < codec->meta_count; i++) {
		printk("meta #%u: type 0x%02x len %u\n",
		       i, codec->meta[i].data.type,
		       codec->meta[i].data.data_len);
		print_hex(codec->meta[i].data.data,
			  codec->meta[i].data.data_len -
				sizeof(codec->meta[i].data.type));
		printk("\n");
	}
}

void print_qos(const struct bt_codec_qos *qos)
{
	printk("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);
}
