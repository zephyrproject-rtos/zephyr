/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>

#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>
#include <sys/byteorder.h>

#include "hearing_aid.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ha_sink);

static struct bt_codec lc3_codec_sink =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_16KHZ | BT_CODEC_LC3_FREQ_24KHZ, BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 60u, 1,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA),
		      BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct bt_audio_stream unicast_streams[MAX_UNICAST_SINK_STREAMS];

#if defined(CONFIG_LIBLC3CODEC)
#include "lc3.h"

#define MAX_SAMPLE_RATE         48000
#define MAX_FRAME_DURATION_US   10000
#define MAX_NUM_SAMPLES         ((MAX_FRAME_DURATION_US * MAX_SAMPLE_RATE) / USEC_PER_SEC)

static int16_t audio_buf[MAX_NUM_SAMPLES];
static lc3_decoder_t lc3_decoder;
static lc3_decoder_mem_48k_t lc3_decoder_mem;
static int frames_per_sdu;

#endif /* CONFIG_LIBLC3CODEC */

static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					  struct bt_audio_ep *ep,
					  enum bt_audio_dir dir,
					  struct bt_audio_capability *cap,
					  struct bt_codec *codec)
{
	LOG_DBG("ASE Codec Config: conn %p ep %p type %u, cap %p",
	       (void *)conn, (void *)ep, dir, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		struct bt_audio_stream *stream = &unicast_streams[i];

		if (!stream->conn) {
			LOG_DBG("ASE Codec Config stream %p", stream);
			return stream;
		}
	}

	LOG_DBG("No streams available");

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
	LOG_DBG("ASE Codec Reconfig: stream %p cap %p", stream, cap);

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
	LOG_DBG("QoS: stream %p qos %p", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream,
		      struct bt_codec_data *meta,
		      size_t meta_count)
{
	LOG_DBG("Enable: stream %p meta_count %u", stream, meta_count);

#if defined(CONFIG_LIBLC3CODEC)
	{
		const int freq_hz = bt_codec_cfg_get_freq(stream->codec);
		const int frame_duration_us = bt_codec_cfg_get_frame_duration_us(stream->codec);

		if (freq_hz < 0) {
			LOG_DBG("Error: Codec frequency not set, cannot start codec.");
			return -1;
		}

		if (frame_duration_us < 0) {
			LOG_DBG("Error: Frame duration not set, cannot start codec.");
			return -1;
		}

		frames_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(stream->codec, true);

		lc3_decoder = lc3_setup_decoder(frame_duration_us,
						freq_hz,
						0, /* No resampling */
						&lc3_decoder_mem);

		if (lc3_decoder == NULL) {
			LOG_DBG("ERROR: Failed to setup LC3 encoder - wrong parameters?");
			return -1;
		}
	}
#endif

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	LOG_DBG("Start: stream %p", stream);

	return 0;
}

static int lc3_metadata(struct bt_audio_stream *stream,
			struct bt_codec_data *meta,
			size_t meta_count)
{
	LOG_DBG("Metadata: stream %p meta_count %u", stream, meta_count);

	return 0;
}

static int lc3_disable(struct bt_audio_stream *stream)
{
	LOG_DBG("Disable: stream %p", stream);

	return 0;
}

static int lc3_stop(struct bt_audio_stream *stream)
{
	LOG_DBG("Stop: stream %p", stream);

	return 0;
}

static int lc3_release(struct bt_audio_stream *stream)
{
	LOG_DBG("Release: stream %p", stream);
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
static void stream_recv(struct bt_audio_stream *stream, const struct bt_iso_recv_info *info,
			struct net_buf *buf)
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
		LOG_DBG("LC3 decoder not setup, cannot decode data.");
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

	LOG_DBG("RX stream %p len %u", stream, buf->len);

	if (err == 1) {
		LOG_DBG("  decoder performed PLC");
		return;

	} else if (err < 0) {
		LOG_DBG("  decoder failed - wrong parameters?");
	}
}
#else
static void stream_recv(struct bt_audio_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	LOG_DBG("Incoming audio on stream %p len %u", stream, buf->len);
}
#endif /* CONFIG_LIBLC3CODEC */

static struct bt_audio_stream_ops stream_ops = {
	.recv = stream_recv,
};

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static uint32_t accepted_broadcast_id;
static struct bt_audio_base received_base;
static bool sink_syncable;
static struct bt_audio_stream broadcast_streams[MAX_BROADCAST_SINK_STREAMS];
static struct bt_audio_broadcast_sink *default_sink;

static bool scan_recv(const struct bt_le_scan_recv_info *info,
		     uint32_t broadcast_id)
{
	LOG_DBG("Found broadcaster with ID 0x%06X", broadcast_id);

	if (broadcast_id == accepted_broadcast_id) {
		LOG_DBG("PA syncing to broadcaster");
		accepted_broadcast_id = 0;
		return true;
	}

	return false;
}

static void pa_synced(struct bt_audio_broadcast_sink *sink,
		      struct bt_le_per_adv_sync *sync,
		      uint32_t broadcast_id)
{
	LOG_DBG("PA synced to broadcaster with ID 0x%06X as sink %p", broadcast_id, (void *)sink);

	if (default_sink == NULL) {
		default_sink = sink;

		LOG_DBG("Sink %p is set as default", (void *)sink);
	}
}

static void base_recv(struct bt_audio_broadcast_sink *sink,
		      const struct bt_audio_base *base)
{
	uint8_t bis_indexes[BROADCAST_SNK_STREAM_CNT] = { 0 };
	/* "0xXX " requires 5 characters */
	char bis_indexes_str[5 * ARRAY_SIZE(bis_indexes) + 1];
	size_t index_count = 0;

	if (memcmp(base, &received_base, sizeof(received_base)) == 0) {
		/* Don't print duplicates */
		return;
	}

	LOG_DBG("Received BASE from sink %p:", (void *)sink);

	for (int i = 0; i < base->subgroup_count; i++) {
		const struct bt_audio_base_subgroup *subgroup;

		subgroup = &base->subgroups[i];

		LOG_DBG("Subgroup[%d]:", i);
		print_codec(&subgroup->codec);

		for (int j = 0; j < subgroup->bis_count; j++) {
			const struct bt_audio_base_bis_data *bis_data;

			bis_data = &subgroup->bis_data[j];

			LOG_DBG("BIS[%d] index 0x%02x", i, bis_data->index);
			bis_indexes[index_count++] = bis_data->index;

			for (int k = 0; k < bis_data->data_count; k++) {
				const struct bt_codec_data *codec_data;

				codec_data = &bis_data->data[k];

				LOG_DBG("data #%u: type 0x%02x len %u", i, codec_data->data.type,
				       codec_data->data.data_len);
				print_hex(codec_data->data.data,
					  codec_data->data.data_len - sizeof(codec_data->data.type));
			}

			LOG_DBG("");
		}
	}

	memset(bis_indexes_str, 0, sizeof(bis_indexes_str));
	/* Create space separated list of indexes as hex values */
	for (int i = 0; i < index_count; i++) {
		char bis_index_str[6];

		sprintf(bis_index_str, "0x%02x ", bis_indexes[i]);

		strcat(bis_indexes_str, bis_index_str);
		LOG_DBG("[%d]: %s", i, bis_index_str);
	}

	LOG_DBG("Possible indexes: %s", bis_indexes_str);

	(void)memcpy(&received_base, base, sizeof(received_base));
}

static void syncable(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	if (sink_syncable) {
		return;
	}

	LOG_DBG("Sink %p is ready to sync %s encryption", (void *)sink, encrypted ? "with" : "without");
	sink_syncable = true;
}

static void scan_term(int err)
{
	LOG_DBG("Broadcast scan was terminated: %d", err);
}

static void pa_sync_lost(struct bt_audio_broadcast_sink *sink)
{
	LOG_DBG("Sink %p disconnected", (void *)sink);

	if (default_sink == sink) {
		default_sink = NULL;
		sink_syncable = false;
	}
}

static struct bt_audio_broadcast_sink_cb broadcast_sink_cb = {
	.scan_recv = scan_recv,
	.pa_synced = pa_synced,
	.base_recv = base_recv,
	.syncable = syncable,
	.scan_term = scan_term,
	.pa_sync_lost = pa_sync_lost,
};
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

/* HAP_d1.0r00; 3.7 BAP Unicast Server role requirements
 * The HA shall support a Presentation Delay range in the Codec Configured state that includes
 * the value of 20ms, in addition to the requirement of Table 5.2 of [3].
 */
#define PD_MIN_USEC 20000

/* BAP_v1.0; Table 5.2: QoS configuration support setting requirements for the Unicast Client and
 * Unicast Server
 */
#define PD_MAX_USEC 40000

static struct bt_audio_capability caps[] = {
	{
		.dir = BT_AUDIO_DIR_SINK,
		.pref = BT_AUDIO_CAPABILITY_PREF(BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
						 BT_GAP_LE_PHY_2M, 0x02, 10,
						 PD_MIN_USEC, PD_MAX_USEC,
						 PD_MIN_USEC, PD_MAX_USEC),
		.codec = &lc3_codec_sink,
		.ops = &lc3_ops,
	},
};

int hearing_aid_sink_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(caps); i++) {
		bt_audio_capability_register(&caps[i]);
	}

	for (size_t i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		bt_audio_stream_cb_register(&unicast_streams[i], &stream_ops);
	}

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	bt_audio_broadcast_sink_register_cb(&broadcast_sink_cb);

	for (size_t i = 0; i < ARRAY_SIZE(broadcast_streams); i++) {
		bt_audio_stream_cb_register(&broadcast_streams[i], &stream_ops);
	}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

	return 0;
}
