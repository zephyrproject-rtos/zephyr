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

#define CHANNEL_COUNT_1 BIT(0)

static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10, CHANNEL_COUNT_1, 40u, 120u,
		     1u, (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA),
		     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

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

#include "lc3.h"

#define MAX_SAMPLE_RATE         48000
#define MAX_FRAME_DURATION_US   10000
#define MAX_NUM_SAMPLES         ((MAX_FRAME_DURATION_US * MAX_SAMPLE_RATE) / USEC_PER_SEC)

static int16_t audio_buf[MAX_NUM_SAMPLES];
static lc3_decoder_t lc3_decoder;
static lc3_decoder_mem_48k_t lc3_decoder_mem;
static int frames_per_sdu;

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

	if (codec->id == BT_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */

		uint32_t chan_allocation;

		printk("  Frequency: %d Hz\n", bt_codec_cfg_get_freq(codec));
		printk("  Frame Duration: %d us\n", bt_codec_cfg_get_frame_duration_us(codec));
		if (bt_codec_cfg_get_chan_allocation_val(codec, &chan_allocation) == 0) {
			printk("  Channel allocation: 0x%x\n", chan_allocation);
		}

		printk("  Octets per frame: %d (negative means value not pressent)\n",
		       bt_codec_cfg_get_octets_per_frame(codec));
		printk("  Frames per SDU: %d\n",
		       bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
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
	printk("QoS: interval %u framing 0x%02x phy 0x%02x sdu %u "
	       "rtn %u latency %u pd %u\n",
	       qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);
}

static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					  struct bt_audio_ep *ep,
					  enum bt_audio_pac_type type,
					  struct bt_audio_capability *cap,
					  struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p type %u, cap %p\n",
	       conn, ep, type, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_audio_stream *stream = &streams[i];

		if (!stream->conn) {
			printk("ASE Codec Config stream %p\n", stream);
			return stream;
		}
	}

	printk("No streams available\n");

#if defined(CONFIG_LIBLC3CODEC)
	/* Nothing to free as static memory is used */
	lc3_decoder = NULL;
#endif

	return NULL;
}

static int lc3_reconfig(struct bt_audio_stream *stream,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	printk("ASE Codec Reconfig: stream %p cap %p\n", stream, cap);

	print_codec(codec);

#if defined(CONFIG_LIBLC3CODEC)
	/* Nothing to free as static memory is used */
	lc3_decoder = NULL;
#endif

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream,
		      struct bt_codec_data *meta,
		      size_t meta_count)
{
	printk("Enable: stream %p meta_count %u\n", stream, meta_count);

#if defined(CONFIG_LIBLC3CODEC)
	{
		const int freq = bt_codec_cfg_get_freq(stream->codec);
		const int frame_duration_us = bt_codec_cfg_get_frame_duration_us(stream->codec);

		if (freq < 0) {
			printk("Error: Codec frequency not set, cannot start codec.");
			return -1;
		}

		if (frame_duration_us < 0) {
			printk("Error: Frame duration not set, cannot start codec.");
			return -1;
		}

		frames_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(stream->codec, true);

		lc3_decoder = lc3_setup_decoder(frame_duration_us,
						freq,
						0, /* No resampling */
						&lc3_decoder_mem);

		if (lc3_decoder == NULL) {
			printk("ERROR: Failed to setup LC3 encoder - wrong parameters?\n");
			return -1;
		}
	}
#endif

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	printk("Start: stream %p\n", stream);

	return 0;
}

static int lc3_metadata(struct bt_audio_stream *stream,
			struct bt_codec_data *meta,
			size_t meta_count)
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


#if defined(CONFIG_LIBLC3CODEC)

static void stream_recv_lc3_codec(struct bt_audio_stream *stream, struct net_buf *buf)
{
	uint8_t err = -1;

	/* TODO: If there is a way to know if the controller supports indicating errors in the
	 *       payload one could feed that into bad-frame-indicator. The HCI layer allows to
	 *       include this information, but currently there is no controller support.
	 *       Here it is assumed that reveiving a zero-length payload means a lost frame -
	 *       but actually it could just as well indicate a pause in the stream.
	 */
	const uint8_t bad_frame_indicator = buf->len == 0 ? 1 : 0;
	uint8_t *in_buf = (bad_frame_indicator ? NULL : buf->data);
	const int octets_per_frame = buf->len / frames_per_sdu;

	if (lc3_decoder == NULL) {
		printk("LC3 decoder not setup, cannot decode data.\n");
		return;
	}

	/* This code is to demonstrate the use of the LC3 codec. On an actual implementation
	 * it might be required to offload the processing to another task to avoid blocking the
	 * BT stack.
	 */
	for (int i = 0; i < frames_per_sdu; i++) {

		int offset = 0;

		err = lc3_decode(lc3_decoder, in_buf + offset, octets_per_frame,
				 LC3_PCM_FORMAT_S16, audio_buf, 1);

		if (in_buf != NULL) {
			offset += octets_per_frame;
		}
	}

	printk("RX stream %p len %u\n", stream, buf->len);

	if (err == 1) {
		printk("  decoder performed PLC\n");
		return;

	} else if (err < 0) {
		printk("  decoder failed - wrong parameters?\n");
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
	default_conn = bt_conn_ref(conn);
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
		.codec = &lc3_codec,
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
