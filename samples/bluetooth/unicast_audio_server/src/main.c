/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/audio.h>
#include <sys/byteorder.h>

#define MAX_PAC 1

#define LC3_PRESET(_name, _codec, _qos) \
	{ \
		.name = _name, \
		.codec = _codec, \
		.qos = _qos, \
	}

struct lc3_preset {
	const char *name;
	struct bt_codec codec;
	struct bt_codec_qos qos;
};

/* Mandatory support preset by both client and server */
static struct lc3_preset preset_16_2_1 =
	LC3_PRESET("16_2_1",
		   BT_CODEC_LC3_CONFIG_16_2,
		   BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(40u, 2u, 10u, 40000u));

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, CONFIG_BT_ISO_TX_MTU, NULL);
static struct bt_conn *default_conn;
static struct bt_audio_chan channels[MAX_PAC];

/* TODO: Expand with BAP data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
};

void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

static void print_codec(const struct bt_codec *codec)
{
	printk("codec 0x%02x cid 0x%04x vid 0x%04x count %u\n",
	       codec->id, codec->cid, codec->vid, codec->data_count);

	for (size_t i = 0; i < codec->data_count; i++) {
		printk("data #%zu: type 0x%02x len %u\n",
		       i, codec->data[i].data.type,
		       codec->data[i].data.data_len);
		print_hex(codec->data[i].data.data,
			  codec->data[i].data.data_len -
				sizeof(codec->data[i].data.type));
		printk("\n");
	}

	for (size_t i = 0; i < codec->meta_count; i++) {
		printk("meta #%zu: type 0x%02x len %u\n",
		       i, codec->meta[i].data.type,
		       codec->meta[i].data.data_len);
		print_hex(codec->meta[i].data.data,
			  codec->meta[i].data.data_len -
				sizeof(codec->meta[i].data.type));
		printk("\n");
	}
}

static void print_qos(struct bt_codec_qos *qos)
{
	printk("QoS: dir 0x%02x interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->dir, qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);
}

static struct bt_audio_chan *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p cap %p\n", conn, ep, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		struct bt_audio_chan *chan = &channels[i];

		if (!chan->conn) {
			printk("ASE Codec Config chan %p\n", chan);
			return chan;
		}
	}

	printk("No channels available\n");

	return NULL;
}

static int lc3_reconfig(struct bt_audio_chan *chan,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	printk("ASE Codec Reconfig: chan %p cap %p\n", chan, cap);

	print_codec(codec);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos)
{
	printk("QoS: chan %p qos %p\n", chan, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_chan *chan, uint8_t meta_count,
		      struct bt_codec_data *meta)
{
	printk("Enable: chan %p meta_count %u\n", chan, meta_count);

	return 0;
}

static int lc3_start(struct bt_audio_chan *chan)
{
	printk("Start: chan %p\n", chan);

	return 0;
}

static int lc3_metadata(struct bt_audio_chan *chan, uint8_t meta_count,
			struct bt_codec_data *meta)
{
	printk("Metadata: chan %p meta_count %u\n", chan, meta_count);

	return 0;
}

static int lc3_disable(struct bt_audio_chan *chan)
{
	printk("Disable: chan %p\n", chan);

	return 0;
}

static int lc3_stop(struct bt_audio_chan *chan)
{
	printk("Stop: chan %p\n", chan);

	return 0;
}

static int lc3_release(struct bt_audio_chan *chan)
{
	printk("Release: chan %p\n", chan);

	return 0;
}

static struct bt_audio_capability_ops lc3_ops = {
	.config = lc3_config,
	.reconfig = lc3_reconfig,
	.qos = lc3_qos,
	.enable = lc3_enable,
	.start = lc3_start,
	.metadata = lc3_metadata,
	.disable = lc3_disable,
	.stop = lc3_stop,
	.release = lc3_release,
};

static void chan_connected(struct bt_audio_chan *chan)
{
	printk("Audio Channel %p connected\n", chan);
}

static void chan_disconnected(struct bt_audio_chan *chan, uint8_t reason)
{
	printk("Audio Channel %p disconnected (reason 0x%02x)\n", chan, reason);
}

static void chan_recv(struct bt_audio_chan *chan, struct net_buf *buf)
{
	printk("Incoming audio on channel %p len %u\n", chan, buf->len);
}

static struct bt_audio_chan_ops chan_ops = {
	.connected = chan_connected,
	.disconnected = chan_disconnected,
	.recv = chan_recv
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		default_conn = NULL;
		return;
	}

	printk("Connected: %s\n", addr);
	default_conn = conn;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_audio_capability caps[] = {
	{
		.type = BT_AUDIO_SINK,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000),
		.codec = &preset_16_2_1.codec,
		.ops = &lc3_ops,
	}
};

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	for (size_t i = 0; i < ARRAY_SIZE(caps); i++) {
		bt_audio_capability_register(&caps[i]);
	}

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		bt_audio_chan_cb_register(&channels[i], &chan_ops);
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}
