/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
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
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>
#include <sys/byteorder.h>

#define MAX_PAC 1

#define AVAILABLE_SINK_CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				 BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				 BT_AUDIO_CONTEXT_TYPE_GAME | \
				 BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL)

#define AVAILABLE_SOURCE_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				  BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				  BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				  BT_AUDIO_CONTEXT_TYPE_GAME)

/* Mandatory support preset by both client and server */
static struct bt_audio_lc3_preset preset_16_2_1 = BT_AUDIO_LC3_UNICAST_PRESET_16_2_1;

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, CONFIG_BT_ISO_TX_MTU, 8, NULL);
static struct bt_conn *default_conn;
static struct bt_audio_stream streams[MAX_PAC];


static uint8_t unicast_server_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL), /* ASCS UUID */
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED, /* Target Announcement */
	(((AVAILABLE_SINK_CONTEXT) >>  0) & 0xFF),
	(((AVAILABLE_SINK_CONTEXT) >>  8) & 0xFF),
	(((AVAILABLE_SOURCE_CONTEXT) >>  0) & 0xFF),
	(((AVAILABLE_SOURCE_CONTEXT) >>  8) & 0xFF),
	0x00, /* Metadata length */
};

/* TODO: Expand with BAP data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
	BT_DATA(BT_DATA_SVC_DATA16, unicast_server_addata, ARRAY_SIZE(unicast_server_addata)),
};

#if defined(CONFIG_LIBLC3CODEC)

#include "lc3_config.h"
#include "lc3_decoder.h"

#define AUDIO_SAMPLE_RATE_HZ    16000
#define AUDIO_LENGTH_US         10000
#define NUM_SAMPLES             ((AUDIO_LENGTH_US * AUDIO_SAMPLE_RATE_HZ) / 1000000)

static int16_t audio_buf[NUM_SAMPLES];
Lc3Decoder_t lc3_decoder;
#endif

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

#if defined(CONFIG_LIBLC3CODEC)
static const char *get_err_str(uint8_t err)
{
	switch (err) {
	case LC3_DECODE_ERR_FREE:
		return "LC3_DECODE_ERR_FREE";
	case LC3_DECODE_ERR_INVALID_CONFIGURATION:
		return "LC3_DECODE_ERR_INVALID_CONFIGURATION";
	case LC3_DECODE_ERR_INVALID_BYTE_COUNT:
		return "LC3_DECODE_ERR_INVALID_BYTE_COUNT";
	case LC3_DECODE_ERR_INVALID_X_OUT_SIZE:
		return "LC3_DECODE_ERR_INVALID_X_OUT_SIZE";
	case LC3_DECODE_ERR_INVALID_BITS_PER_AUDIO_SAMPLE:
		return "LC3_DECODE_ERR_INVALID_BITS_PER_AUDIO_SAMPLE";
	case LC3_DECODE_ERR_DECODER_ALLOCATION:
		return "LC3_DECODE_ERR_DECODER_ALLOCATION";
	}
	return "UNKNOWN LC3_DECODE ERROR";
}
#endif

static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					  struct bt_audio_ep *ep,
					  struct bt_audio_capability *cap,
					  struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p cap %p\n", conn, ep, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_audio_stream *stream = &streams[i];

		if (!stream->conn) {
			printk("ASE Codec Config stream %p\n", stream);
			return stream;
		}
	}

	printk("No streams available\n");

	return NULL;
}

static int lc3_reconfig(struct bt_audio_stream *stream,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	printk("ASE Codec Reconfig: stream %p cap %p\n", stream, cap);

	print_codec(codec);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream, uint8_t meta_count,
		      struct bt_codec_data *meta)
{
	printk("Enable: stream %p meta_count %u\n", stream, meta_count);

#if defined(CONFIG_LIBLC3CODEC)
	/* TODO: parse codec config data and extract frame duration and sample-rate
	 *       Currently there is no lib for this.
	 */
	lc3_decoder = Lc3Decoder_create(AUDIO_SAMPLE_RATE_HZ, lc3config_duration_10ms);
#endif

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	printk("Start: stream %p\n", stream);

	return 0;
}

static int lc3_metadata(struct bt_audio_stream *stream, uint8_t meta_count,
			struct bt_codec_data *meta)
{
	printk("Metadata: stream %p meta_count %u\n", stream, meta_count);

	return 0;
}

static int lc3_disable(struct bt_audio_stream *stream)
{
	printk("Disable: stream %p\n", stream);

	return 0;
}

static int lc3_stop(struct bt_audio_stream *stream)
{
	printk("Stop: stream %p\n", stream);

	return 0;
}

static int lc3_release(struct bt_audio_stream *stream)
{
	printk("Release: stream %p\n", stream);

#if defined(CONFIG_LIBLC3CODEC)
	Lc3Decoder_destroy(lc3_decoder);
	lc3_decoder = NULL;
#endif

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

static void stream_connected(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p connected\n", stream);
}

static void stream_disconnected(struct bt_audio_stream *stream, uint8_t reason)
{
	printk("Audio Stream %p disconnected (reason 0x%02x)\n", stream, reason);
}


#if defined(CONFIG_LIBLC3CODEC)

static void stream_recv_lc3_codec(struct bt_audio_stream *stream, struct net_buf *buf)
{
	uint8_t bit_error_detect = 0;
	uint8_t err;
	/* TODO: If there is a way to know if the controller supports indicating errors in the
	 *       payload one could feed that into bad-frame-indicator. The HCI layer allows to
	 *       include this information, but currently there is no controller support.
	 *       Here it is assumed that reveiving a zero-length payload means a lost frame -
	 *       but actually it could just as well indicate a pause in the stream.
	 */
	const uint8_t bad_frame_indicator = buf->len == 0 ? 1 : 0;

	/* This code is to demonstrate the use of the LC3 codec. On an actual implementation
	 * it might be required to offload the processing to another task to avoid blocking the
	 * BT stack.
	 */
	err = Lc3Decoder_run(lc3_decoder, buf->data, buf->len, bad_frame_indicator, audio_buf,
			     sizeof(audio_buf)/2, &bit_error_detect);

	printk("Incoming audio on stream %p len %u\n", stream, buf->len);

	if (err != LC3_DECODE_ERR_FREE) {
		printk("decoder return: %s (%d)", get_err_str(err), err);
		return;
	}

	if (bit_error_detect) {
		printk("  BIT errors detected\n");
		return;
	}
}

#else

static void stream_recv(struct bt_audio_stream *stream, struct net_buf *buf)
{
	printk("Incoming audio on stream %p len %u\n", stream, buf->len);
}

#endif

static struct bt_audio_stream_ops stream_ops = {
	.connected = stream_connected,
	.disconnected = stream_disconnected,
#if defined(CONFIG_LIBLC3CODEC)
	.recv = stream_recv_lc3_codec
#else
	.recv = stream_recv
#endif
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
				BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000,
				40000, 40000),
		.codec = &preset_16_2_1.codec,
		.ops = &lc3_ops,
	}
};

void main(void)
{
	struct bt_le_ext_adv *adv;
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

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		bt_audio_stream_cb_register(&streams[i], &stream_ops);
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN_NAME, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start advertising set (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}
