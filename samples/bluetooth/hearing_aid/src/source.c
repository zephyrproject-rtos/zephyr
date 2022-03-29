/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <stddef.h>
#include <errno.h>
#include <sys/slist.h>

#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

#include "hearing_aid.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ha_source);

struct hearing_aid_stream {
	struct bt_audio_stream stream;

	sys_snode_t node;
};

NET_BUF_POOL_FIXED_DEFINE(tx_pool, MAX_UNICAST_SOURCE_STREAMS,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  8, NULL);
K_MEM_SLAB_DEFINE(stream_slab, sizeof(struct bt_audio_stream),
		  MAX_UNICAST_SOURCE_STREAMS, __alignof__(struct bt_audio_stream));

static struct bt_codec lc3_codec_source =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_16KHZ, BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 40u, 1,
		     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct k_work_delayable audio_send_work;
static k_timeout_t audio_send_work_delay = K_NO_WAIT;
static sys_slist_t active_streams;

#if defined(CONFIG_LIBLC3CODEC)
#include "lc3.h"
#include "math.h"

#define MAX_SAMPLE_RATE         48000
#define MAX_FRAME_DURATION_US   10000
#define MAX_NUM_SAMPLES         ((MAX_FRAME_DURATION_US * MAX_SAMPLE_RATE) / USEC_PER_SEC)
#define AUDIO_VOLUME            (INT16_MAX - 3000) /* codec does clipping above INT16_MAX - 3000 */
#define AUDIO_TONE_FREQUENCY_HZ   400

static int16_t audio_buf[MAX_NUM_SAMPLES];
static lc3_encoder_t lc3_encoder;
static lc3_encoder_mem_48k_t lc3_encoder_mem;
static int octets_per_frame;
static int frames_per_sdu;
static int frame_duration_us;

/**
 * Use the math lib to generate a sine-wave using 16 bit samples into a buffer.
 *
 * @param buf Destination buffer
 * @param length_us Length of the buffer in microseconds
 * @param frequency_hz frequency in Hz
 * @param sample_rate_hz sample-rate in Hz.
 */
static void fill_audio_buf_sin(int16_t *buf, int length_us, int frequency_hz, int sample_rate_hz)
{
	const int sine_period_samples = sample_rate_hz / frequency_hz;
	const unsigned int num_samples = (length_us * sample_rate_hz) / USEC_PER_SEC;
	const float step = 2 * 3.1415 / sine_period_samples;

	for (unsigned int i = 0; i < num_samples; i++) {
		const float sample = sin(i * step);

		buf[i] = (int16_t)(AUDIO_VOLUME * sample);
	}
}

static void audio_timer_timeout(struct k_work *work)
{
	/* For the first call-back we push multiple audio frames to the buffer to use the
	 * controller ISO buffer to handle jitter.
	 */
	const uint8_t prime_count = 2;
	static int64_t start_time;
	static int32_t sdu_cnt;
	int32_t sdu_goal_cnt;
	int64_t uptime, run_time_ms;

	k_work_schedule(&audio_send_work, audio_send_work_delay);

	if (lc3_encoder == NULL) {
		LOG_DBG("LC3 encoder not setup, cannot encode data.");
		return;
	}

	if (start_time == 0) {
		/* Read start time and produce the number of frames needed to catch up with any
		 * inaccuracies in the timer. by calculating the number of frames we should
		 * have sent and compare to how many were actually sent.
		 */
		start_time = k_uptime_get();
	}

	uptime = k_uptime_get();
	run_time_ms = uptime - start_time;

	sdu_goal_cnt = (run_time_ms * 1000) / (frame_duration_us * frames_per_sdu);

	/* Add primer value to ensure the controller do not run low on data due to jitter */
	sdu_goal_cnt += prime_count;

	LOG_DBG("LC3 encode %d frames in %d SDUs", (sdu_goal_cnt - sdu_cnt) * frames_per_sdu,
						    (sdu_goal_cnt - sdu_cnt));

	while (sdu_cnt < sdu_goal_cnt) {
		struct hearing_aid_stream *ha_stream;
		int ret;
		struct net_buf *buf;
		uint8_t *net_buffer;
		int lc3_ret = -1;
		const uint16_t tx_sdu_len = frames_per_sdu * octets_per_frame;
		int offset = 0;

		SYS_SLIST_FOR_EACH_CONTAINER(&active_streams, ha_stream, node) {
			buf = net_buf_alloc(&tx_pool, K_FOREVER);
			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

			net_buffer = net_buf_tail(buf);
			buf->len += tx_sdu_len;

			for (int i = 0; i < frames_per_sdu; i++) {
				lc3_ret = lc3_encode(lc3_encoder, LC3_PCM_FORMAT_S16, audio_buf,
						1, octets_per_frame, net_buffer + offset);
				offset += octets_per_frame;
			}

			if (lc3_ret == -1) {
				LOG_DBG("LC3 encoder failed - wrong parameters?: %d", lc3_ret);
				net_buf_unref(buf);
				return;
			}

			ret = bt_audio_stream_send(&ha_stream->stream, buf);
			if (ret < 0) {
				LOG_DBG("Failed to send LC3 audio data on stream %p: (%d)",
				       &ha_stream->stream, ret);
				net_buf_unref(buf);
			} else {
				LOG_DBG("Sending LC3 audio data with len %zu on stream %p",
				       tx_sdu_len, &ha_stream->stream);
			}
		}

		sdu_cnt++;
	}
}
#else
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
	struct hearing_aid_stream *ha_stream;
	int ret;
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static bool data_initialized;
	struct net_buf *buf;
	static size_t len_to_send = 1;

	if (!data_initialized) {
		/* TODO: Actually encode some audio data */
		for (int i = 0; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&active_streams, ha_stream, node) {
		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, buf_data, len_to_send);

		ret = bt_audio_stream_send(&ha_stream->stream, buf);
		if (ret < 0) {
			LOG_DBG("Failed to send mock audio data on stream %p: (%d)",
			       &ha_stream->stream, ret);
			net_buf_unref(buf);
		} else {
			LOG_DBG("Sending mock audio data with len %zu on stream %p",
			       len_to_send, &ha_stream->stream);
		}
	}

	k_work_schedule(&audio_send_work, audio_send_work_delay);

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}
#endif /* CONFIG_LIBLC3CODEC */

static void stream_sent(struct bt_audio_stream *stream)
{
	LOG_DBG("Audio Stream %p sent", stream);
}

static struct bt_audio_stream_ops stream_ops = {
	.sent = stream_sent,
};

static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					  struct bt_audio_ep *ep,
					  enum bt_audio_dir dir,
					  struct bt_audio_capability *cap,
					  struct bt_codec *codec)
{
	struct hearing_aid_stream *ha_stream = NULL;

	LOG_DBG("ASE Codec Config: conn %p ep %p type %u, cap %p",
	       (void *)conn, (void *)ep, dir, cap);

	print_codec(codec);

	/* Alloc stream */
	if (k_mem_slab_alloc(&stream_slab, (void **)&ha_stream, K_NO_WAIT)) {
		LOG_DBG("Failed to allocate stream");
		return NULL;
	}

	bt_audio_stream_cb_register(&ha_stream->stream, &stream_ops);

	LOG_DBG("ASE Codec Config stream %p", &ha_stream->stream);

	return &ha_stream->stream;
}

static int lc3_reconfig(struct bt_audio_stream *stream,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	LOG_DBG("ASE Codec Reconfig: stream %p cap %p", stream, cap);

	print_codec(codec);

#if defined(CONFIG_LIBLC3CODEC)
	/* Nothing to free as static memory is used */
	lc3_encoder = NULL;
#endif

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	LOG_DBG("QoS: stream %p qos %p", stream, qos);

	print_qos(qos);

	audio_send_work_delay = K_USEC(qos->interval);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream,
		      struct bt_codec_data *meta,
		      size_t meta_count)
{
	LOG_DBG("Enable: stream %p meta_count %u", stream, meta_count);

#if defined(CONFIG_LIBLC3CODEC)
	const int freq_hz = bt_codec_cfg_get_freq(stream->codec);
	unsigned int num_samples;

	if (freq_hz < 0) {
		LOG_DBG("Error: Codec frequency not set, cannot start codec.");
		return -EINVAL;
	}

	frame_duration_us = bt_codec_cfg_get_frame_duration_us(stream->codec);
	if (frame_duration_us < 0) {
		LOG_DBG("Error: Frame duration not set, cannot start codec.");
		return -EINVAL;
	}

	octets_per_frame = bt_codec_cfg_get_octets_per_frame(stream->codec);
	if (octets_per_frame < 0) {
		LOG_DBG("Error: Octets per frame not set, cannot start codec.");
		return -EINVAL;
	}

	frames_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(stream->codec, true);
	if (frames_per_sdu < 0) {
		LOG_DBG("Error: Frames per SDU not set, cannot start codec.");
		return -EINVAL;
	}

	/* Fill audio buffer with Sine wave only once and repeat encoding the same tone frame */
	fill_audio_buf_sin(audio_buf, frame_duration_us, AUDIO_TONE_FREQUENCY_HZ, freq_hz);

	num_samples = ((frame_duration_us * freq_hz) / USEC_PER_SEC);
	for (unsigned int i = 0; i < num_samples; i++) {
		LOG_DBG("%3i: %6i", i, audio_buf[i]);
	}

	lc3_encoder = lc3_setup_encoder(frame_duration_us,
					freq_hz,
					0, /* No resampling */
					&lc3_encoder_mem);

	if (lc3_encoder == NULL) {
		LOG_DBG("ERROR: Failed to setup LC3 encoder - wrong parameters?");
	}
#endif /* CONFIG_LIBLC3CODEC */

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	struct hearing_aid_stream *ha_stream = CONTAINER_OF(stream, struct hearing_aid_stream,
							    stream);

	LOG_DBG("Start: stream %p", stream);

	sys_slist_append(&active_streams, &ha_stream->node);

	if (!k_work_delayable_is_pending(&audio_send_work)) {
		/* Start sending audio data */
		k_work_schedule(&audio_send_work, K_NO_WAIT);
	}

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

static void disactivate_stream(struct bt_audio_stream *stream)
{
	struct hearing_aid_stream *ha_stream;
	sys_snode_t *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&active_streams, ha_stream, node) {
		if (&ha_stream->stream == stream) {
			sys_slist_remove(&active_streams, prev, &ha_stream->node);
			break;
		}

		prev = &ha_stream->node;
	}
}

static int lc3_stop(struct bt_audio_stream *stream)
{
	LOG_DBG("Stop: stream %p", stream);

	disactivate_stream(stream);

	k_work_cancel_delayable(&audio_send_work);

	return 0;
}

static int lc3_release(struct bt_audio_stream *stream)
{
	struct hearing_aid_stream *ha_stream = CONTAINER_OF(stream, struct hearing_aid_stream,
							    stream);

	LOG_DBG("Release: stream %p", stream);

	k_work_cancel_delayable(&audio_send_work);

	disactivate_stream(stream);

	k_mem_slab_free(&stream_slab, (void **)&ha_stream);

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

static struct bt_audio_capability caps[] = {
	{
		.dir = BT_AUDIO_DIR_SOURCE,
		.pref = BT_AUDIO_CAPABILITY_PREF(BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
						 BT_GAP_LE_PHY_2M, 0x02, 10,
						 PD_MIN_USEC, PD_MAX_USEC,
						 PD_MIN_USEC, PD_MAX_USEC),
		.codec = &lc3_codec_source,
		.ops = &lc3_ops,
	},
};

int hearing_aid_source_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(caps); i++) {
		bt_audio_capability_register(&caps[i]);
	}

	if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_LEFT)) {
		bt_audio_capability_set_location(BT_AUDIO_DIR_SOURCE, BT_AUDIO_LOCATION_FRONT_LEFT);
	} else {
		bt_audio_capability_set_location(BT_AUDIO_DIR_SOURCE, BT_AUDIO_LOCATION_FRONT_RIGHT);
	}

	k_work_init_delayable(&audio_send_work, audio_timer_timeout);
	sys_slist_init(&active_streams);

	return 0;
}
