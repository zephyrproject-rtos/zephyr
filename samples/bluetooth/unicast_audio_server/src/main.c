/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>
#include <zephyr/sys/byteorder.h>

#define AVAILABLE_SINK_CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				 BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				 BT_AUDIO_CONTEXT_TYPE_GAME | \
				 BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL)

#define AVAILABLE_SOURCE_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				  BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				  BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				  BT_AUDIO_CONTEXT_TYPE_GAME)

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ASCS_ASE_SRC_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  8, NULL);

static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 120u, 1u,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

static struct bt_conn *default_conn;
static struct k_work_delayable audio_send_work;
static struct bt_audio_stream streams[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];
static struct bt_audio_source {
	struct bt_audio_stream *stream;
	uint32_t seq_num;
} source_streams[CONFIG_BT_ASCS_ASE_SRC_COUNT];
static size_t configured_source_stream_count;

static K_SEM_DEFINE(sem_disconnected, 0, 1);

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

static uint32_t get_and_incr_seq_num(const struct bt_audio_stream *stream)
{
	for (size_t i = 0U; i < configured_source_stream_count; i++) {
		if (stream == source_streams[i].stream) {
			return source_streams[i].seq_num++;
		}
	}

	printk("Could not find endpoint from stream %p\n", stream);

	return 0;
}

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

/**
 * @brief Send audio data on timeout
 *
 * This will send an increasing amount of audio data, starting from 1 octet.
 * The data is just mock data, and does not actually represent any audio.
 *
 * First iteration : 0x00
 * Second iteration: 0x00 0x01
 * Third iteration : 0x00 0x01 0x02
 *
 * And so on, until it wraps around the configured MTU (CONFIG_BT_ISO_TX_MTU)
 *
 * @param work Pointer to the work structure
 */
static void audio_timer_timeout(struct k_work *work)
{
	int ret;
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static bool data_initialized;
	struct net_buf *buf;
	static size_t len_to_send = 1;

	if (!data_initialized) {
		/* TODO: Actually encode some audio data */
		for (size_t i = 0U; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	/* We configured the sink streams to be first in `streams`, so that
	 * we can use `stream[i]` to select sink streams (i.e. streams with
	 * data going to the server)
	 */
	for (size_t i = 0; i < configured_source_stream_count; i++) {
		struct bt_audio_stream *stream = source_streams[i].stream;

		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len_to_send);

		ret = bt_audio_stream_send(stream, buf,
					   get_and_incr_seq_num(stream),
					   BT_ISO_TIMESTAMP_NONE);
		if (ret < 0) {
			printk("Failed to send audio data on streams[%zu] (%p): (%d)\n",
			       i, stream, ret);
			net_buf_unref(buf);
		} else {
			printk("Sending mock data with len %zu on streams[%zu] (%p)\n",
			       len_to_send, i, stream);
		}
	}

	k_work_schedule(&audio_send_work, K_MSEC(1000U));

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}

static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					  struct bt_audio_ep *ep,
					  enum bt_audio_dir dir,
					  struct bt_audio_capability *cap,
					  struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p dir %u, cap %p\n",
	       conn, ep, dir, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_audio_stream *stream = &streams[i];

		if (!stream->conn) {
			printk("ASE Codec Config stream %p\n", stream);
			if (dir == BT_AUDIO_DIR_SOURCE) {
				source_streams[configured_source_stream_count++].stream = stream;
			}

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

	for (size_t i = 0U; i < configured_source_stream_count; i++) {
		if (source_streams[i].stream == stream) {
			source_streams[i].seq_num = 0U;
			break;
		}
	}

	if (configured_source_stream_count > 0 &&
	    !k_work_delayable_is_pending(&audio_send_work)) {

		/* Start send timer */
		k_work_schedule(&audio_send_work, K_MSEC(0));
	}

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

static void stream_recv_lc3_codec(struct bt_audio_stream *stream,
				  const struct bt_iso_recv_info *info,
				  struct net_buf *buf)
{
	const uint8_t *in_buf;
	uint8_t err = -1;
	const int octets_per_frame = buf->len / frames_per_sdu;

	if (lc3_decoder == NULL) {
		printk("LC3 decoder not setup, cannot decode data.\n");
		return;
	}

	if ((info->flags & BT_ISO_FLAGS_VALID) == 0) {
		printk("Bad packet: 0x%02X\n", info->flags);

		in_buf = NULL;
	} else {
		in_buf = buf->data;
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

static void stream_recv(struct bt_audio_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
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

	k_sem_give(&sem_disconnected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_audio_capability caps[] = {
	{
		.dir = BT_AUDIO_DIR_SINK,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000,
				40000, 40000),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
	{
		.dir = BT_AUDIO_DIR_SOURCE,
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

	while (true) {
		struct k_work_sync sync;

		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start advertising set (err %d)\n", err);
			return;
		}

		printk("Advertising successfully started\n");

		k_work_init_delayable(&audio_send_work, audio_timer_timeout);

		err = k_sem_take(&sem_disconnected, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_disconnected (err %d)\n", err);
			return;
		}

		/* reset data */
		(void)memset(source_streams, 0, sizeof(source_streams));
		configured_source_stream_count = 0U;
		k_work_cancel_delayable_sync(&audio_send_work, &sync);

	}
}
