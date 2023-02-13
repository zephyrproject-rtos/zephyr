/** @file
 *  @brief Bluetooth Basic Audio Profile shell
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>

#include "bt.h"

#define LOCATION BT_AUDIO_LOCATION_FRONT_LEFT
#define CONTEXT BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA

#if defined(CONFIG_BT_AUDIO_UNICAST)
#define UNICAST_SERVER_STREAM_COUNT \
	COND_CODE_1(CONFIG_BT_ASCS, \
		    (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT), (0))
#define UNICAST_CLIENT_STREAM_COUNT \
	COND_CODE_1(CONFIG_BT_AUDIO_UNICAST_CLIENT, \
		    (CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT + \
		     CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT), (0))

static struct bt_audio_stream streams[UNICAST_SERVER_STREAM_COUNT + UNICAST_CLIENT_STREAM_COUNT];

static const struct bt_codec_qos_pref qos_pref = BT_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0u, 60u,
								   20000u, 40000u, 20000u, 40000u);

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
static struct bt_audio_unicast_group *default_unicast_group;
static struct bt_codec *rcodecs[2][CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
static struct bt_audio_ep *snks[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
static struct bt_audio_ep *srcs[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
#endif /* CONFIG_BT_AUDIO_UNICAST */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
static struct bt_audio_stream broadcast_source_streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static struct bt_audio_broadcast_source *default_source;
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static struct bt_audio_stream broadcast_sink_streams[BROADCAST_SNK_STREAM_CNT];
static struct bt_audio_broadcast_sink *default_sink;
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */
static struct bt_audio_stream *default_stream;
static uint16_t seq_num;
static size_t rx_cnt;

struct named_lc3_preset {
	const char *name;
	struct bt_audio_lc3_preset preset;
};

static struct named_lc3_preset lc3_unicast_presets[] = {
	{"8_1_1",   BT_AUDIO_LC3_UNICAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1",   BT_AUDIO_LC3_UNICAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_AUDIO_LC3_UNICAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_AUDIO_LC3_UNICAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2",   BT_AUDIO_LC3_UNICAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2",   BT_AUDIO_LC3_UNICAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_AUDIO_LC3_UNICAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_AUDIO_LC3_UNICAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};

static struct named_lc3_preset lc3_broadcast_presets[] = {
	{"8_1_1",   BT_AUDIO_LC3_BROADCAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1",   BT_AUDIO_LC3_BROADCAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_AUDIO_LC3_BROADCAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_AUDIO_LC3_BROADCAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2",   BT_AUDIO_LC3_BROADCAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2",   BT_AUDIO_LC3_BROADCAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_AUDIO_LC3_BROADCAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_AUDIO_LC3_BROADCAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};

/* Default to 16_2_1 */
static struct named_lc3_preset *default_preset = &lc3_unicast_presets[3];
static bool initialized;

static uint16_t get_next_seq_num(uint32_t interval_us)
{
	static int64_t last_ticks;
	int64_t uptime_ticks, delta_ticks;
	uint64_t delta_us;
	uint64_t seq_num_incr;
	uint64_t next_seq_num;

	/* Note: This does not handle wrapping of ticks when they go above
	 * 2^(62-1)
	 */
	uptime_ticks = k_uptime_ticks();
	delta_ticks = uptime_ticks - last_ticks;
	last_ticks = uptime_ticks;

	delta_us = k_ticks_to_us_near64((uint64_t)delta_ticks);
	seq_num_incr = delta_us / interval_us;
	next_seq_num = (seq_num_incr + seq_num);

	return (uint16_t)next_seq_num;
}

#if defined(CONFIG_LIBLC3)
NET_BUF_POOL_FIXED_DEFINE(sine_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  CONFIG_BT_ISO_TX_MTU + BT_ISO_CHAN_SEND_RESERVE,
			  8, NULL);

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
static int freq_hz;
static int frame_duration_us;
static int frame_duration_100us;
static int frames_per_sdu;
static int octets_per_frame;
static int64_t lc3_start_time;
static int32_t lc3_sdu_cnt;

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
	const uint32_t sine_period_samples = sample_rate_hz / frequency_hz;
	const size_t num_samples = (length_us * sample_rate_hz) / USEC_PER_SEC;
	const float step = 2 * 3.1415 / sine_period_samples;

	for (size_t i = 0; i < num_samples; i++) {
		const float sample = sin(i * step);

		buf[i] = (int16_t)(AUDIO_VOLUME * sample);
	}
}

static void init_lc3(void)
{
	size_t num_samples;

	freq_hz = bt_codec_cfg_get_freq(&default_preset->preset.codec);
	frame_duration_us = bt_codec_cfg_get_frame_duration_us(&default_preset->preset.codec);
	octets_per_frame = bt_codec_cfg_get_octets_per_frame(&default_preset->preset.codec);
	frames_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(&default_preset->preset.codec, true);
	octets_per_frame = bt_codec_cfg_get_octets_per_frame(&default_preset->preset.codec);

	if (freq_hz < 0) {
		printk("Error: Codec frequency not set, cannot start codec.");
		return;
	}

	if (frame_duration_us < 0) {
		printk("Error: Frame duration not set, cannot start codec.");
		return;
	}

	if (octets_per_frame < 0) {
		printk("Error: Octets per frame not set, cannot start codec.");
		return;
	}

	frame_duration_100us = frame_duration_us / 100;

	/* Fill audio buffer with Sine wave only once and repeat encoding the same tone frame */
	fill_audio_buf_sin(audio_buf, frame_duration_us, AUDIO_TONE_FREQUENCY_HZ, freq_hz);

	num_samples = ((frame_duration_us * freq_hz) / USEC_PER_SEC);
	for (size_t i = 0; i < num_samples; i++) {
		printk("%zu: %6i\n", i, audio_buf[i]);
	}

	/* Create the encoder instance. This shall complete before stream_started() is called. */
	lc3_encoder = lc3_setup_encoder(frame_duration_us, freq_hz,
					0, /* No resampling */
					&lc3_encoder_mem);

	if (lc3_encoder == NULL) {
		printk("ERROR: Failed to setup LC3 encoder - wrong parameters?\n");
	}
}

static void lc3_audio_timer_timeout(struct k_work *work)
{
	/* For the first call-back we push multiple audio frames to the buffer to use the
	 * controller ISO buffer to handle jitter.
	 */
	const uint8_t prime_count = 2;
	static bool lc3_initialized;
	int64_t run_time_100us;
	int32_t sdu_goal_cnt;
	int64_t run_time_ms;
	int64_t uptime;

	if (!lc3_initialized) {
		init_lc3();
		lc3_initialized = true;
	}

	if (lc3_encoder == NULL) {
		printk("LC3 encoder not setup, cannot encode data.\n");
		return;
	}

	k_work_schedule(k_work_delayable_from_work(work),
			K_USEC(default_preset->preset.qos.interval));

	if (lc3_start_time == 0) {
		/* Read start time and produce the number of frames needed to catch up with any
		 * inaccuracies in the timer. by calculating the number of frames we should
		 * have sent and compare to how many were actually sent.
		 */
		lc3_start_time = k_uptime_get();
	}

	uptime = k_uptime_get();
	run_time_ms = uptime - lc3_start_time;

	/* PDU count calculations done in 100us units to allow 7.5ms framelength in fixed-point */
	run_time_100us = run_time_ms * 10;
	sdu_goal_cnt = run_time_100us / (frame_duration_100us * frames_per_sdu);

	/* Add primer value to ensure the controller do not run low on data due to jitter */
	sdu_goal_cnt += prime_count;

	if ((lc3_sdu_cnt % 100) == 0) {
		printk("LC3 encode %d frames in %d SDUs\n",
		       (sdu_goal_cnt - lc3_sdu_cnt) * frames_per_sdu,
		       (sdu_goal_cnt - lc3_sdu_cnt));
	}

	seq_num = get_next_seq_num(default_preset->preset.qos.interval);

	while (lc3_sdu_cnt < sdu_goal_cnt) {
		const uint16_t tx_sdu_len = frames_per_sdu * octets_per_frame;
		struct net_buf *buf;
		uint8_t *net_buffer;
		off_t offset = 0;
		int err;

		buf = net_buf_alloc(&sine_tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

		net_buffer = net_buf_tail(buf);
		buf->len += tx_sdu_len;

		for (int i = 0; i < frames_per_sdu; i++) {
			int lc3_ret;

			lc3_ret = lc3_encode(lc3_encoder, LC3_PCM_FORMAT_S16,
					     audio_buf, 1, octets_per_frame,
					     net_buffer + offset);
			offset += octets_per_frame;

			if (lc3_ret == -1) {
				printk("LC3 encoder failed - wrong parameters?: %d",
					lc3_ret);
				net_buf_unref(buf);
				return;
			}
		}

		err = bt_audio_stream_send(default_stream, buf, seq_num,
					   BT_ISO_TIMESTAMP_NONE);
		if (err < 0) {
			printk("Failed to send LC3 audio data (%d)\n",
				err);
			net_buf_unref(buf);
			return;
		}

		if ((lc3_sdu_cnt % 100) == 0) {
			printk("TX LC3: %zu\n", tx_sdu_len);
		}
		lc3_sdu_cnt++;
		seq_num++;
	}
}

static K_WORK_DELAYABLE_DEFINE(audio_send_work, lc3_audio_timer_timeout);
#endif /* CONFIG_LIBLC3 */

static void print_codec(const struct bt_codec *codec)
{
	int i;

	shell_print(ctx_shell, "codec 0x%02x cid 0x%04x vid 0x%04x count %u",
		    codec->id, codec->cid, codec->vid, codec->data_count);

	for (i = 0; i < codec->data_count; i++) {
		shell_print(ctx_shell, "data #%u: type 0x%02x len %u", i,
			    codec->data[i].data.type,
			    codec->data[i].data.data_len);
		shell_hexdump(ctx_shell, codec->data[i].data.data,
			      codec->data[i].data.data_len -
				sizeof(codec->data[i].data.type));
	}

	for (i = 0; i < codec->meta_count; i++) {
		shell_print(ctx_shell, "meta #%u: type 0x%02x len %u", i,
			    codec->meta[i].data.type,
			    codec->meta[i].data.data_len);
		shell_hexdump(ctx_shell, codec->meta[i].data.data,
			      codec->meta[i].data.data_len -
				sizeof(codec->meta[i].data.type));
	}
}

static struct named_lc3_preset *set_preset(bool is_unicast, size_t argc,
					   char **argv)
{
	static struct named_lc3_preset named_preset;
	int i;

	if (is_unicast) {
		for (i = 0; i < ARRAY_SIZE(lc3_unicast_presets); i++) {
			if (!strcmp(argv[0], lc3_unicast_presets[i].name)) {
				default_preset = &lc3_unicast_presets[i];
				break;
			}
		}

		if (i == ARRAY_SIZE(lc3_unicast_presets)) {
			return NULL;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(lc3_broadcast_presets); i++) {
			if (!strcmp(argv[0], lc3_broadcast_presets[i].name)) {
				default_preset = &lc3_broadcast_presets[i];
				break;
			}
		}

		if (i == ARRAY_SIZE(lc3_broadcast_presets)) {
			return NULL;
		}

	}

	if (argc == 1) {
		return default_preset;
	}

	named_preset = *default_preset;
	default_preset = &named_preset;

	if (argc > 1) {
		named_preset.preset.qos.interval = strtol(argv[1], NULL, 0);
	}

	if (argc > 2) {
		named_preset.preset.qos.framing = strtol(argv[2], NULL, 0);
	}

	if (argc > 3) {
		named_preset.preset.qos.latency = strtol(argv[3], NULL, 0);
	}

	if (argc > 4) {
		named_preset.preset.qos.pd = strtol(argv[4], NULL, 0);
	}

	if (argc > 5) {
		named_preset.preset.qos.sdu = strtol(argv[5], NULL, 0);
	}

	if (argc > 6) {
		named_preset.preset.qos.phy = strtol(argv[6], NULL, 0);
	}

	if (argc > 7) {
		named_preset.preset.qos.rtn = strtol(argv[7], NULL, 0);
	}

	return default_preset;
}

static void set_stream(struct bt_audio_stream *stream)
{
	int i;

	default_stream = stream;

	for (i = 0; i < ARRAY_SIZE(streams); i++) {
		if (stream == &streams[i]) {
			shell_print(ctx_shell, "Default stream: %u", i + 1);
		}
	}
}

static void print_qos(const struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: interval %u framing 0x%02x "
		    "phy 0x%02x sdu %u rtn %u latency %u pd %u",
		    qos->interval, qos->framing, qos->phy, qos->sdu,
		    qos->rtn, qos->latency, qos->pd);
}

static int cmd_select_unicast(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_stream *stream;
	uint8_t index;

	index = strtol(argv[1], NULL, 0);
	if (index < 0 || index > ARRAY_SIZE(streams)) {
		shell_error(sh, "Invalid index: %d", index);
		return -ENOEXEC;
	}

	stream = &streams[index];
	if (stream->conn == NULL) {
		shell_error(sh, "Invalid index");
		return -ENOEXEC;
	}

	set_stream(stream);

	return 0;
}

static struct bt_audio_stream *stream_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_audio_stream *stream = &streams[i];

		if (!stream->conn) {
			return stream;
		}
	}

	return NULL;
}

static int lc3_config(struct bt_conn *conn, const struct bt_audio_ep *ep, enum bt_audio_dir dir,
		      const struct bt_codec *codec, struct bt_audio_stream **stream,
		      struct bt_codec_qos_pref *const pref)
{
	shell_print(ctx_shell, "ASE Codec Config: conn %p ep %p dir %u", conn, ep, dir);

	print_codec(codec);

	*stream = stream_alloc();
	if (*stream == NULL) {
		shell_print(ctx_shell, "No streams available");

		return -ENOMEM;
	}

	shell_print(ctx_shell, "ASE Codec Config stream %p", *stream);

	set_stream(*stream);

	*pref = qos_pref;

	return 0;
}

static int lc3_reconfig(struct bt_audio_stream *stream, enum bt_audio_dir dir,
			const struct bt_codec *codec, struct bt_codec_qos_pref *const pref)
{
	shell_print(ctx_shell, "ASE Codec Reconfig: stream %p", stream);

	print_codec(codec);

	if (default_stream == NULL) {
		set_stream(stream);
	}

	*pref = qos_pref;

	return 0;
}

static int lc3_qos(struct bt_audio_stream *stream, const struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: stream %p %p", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream, const struct bt_codec_data *meta,
		      size_t meta_count)
{
	shell_print(ctx_shell, "Enable: stream %p meta_count %zu", stream,
		    meta_count);

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Start: stream %p", stream);

	seq_num = 0;

	return 0;
}


static bool valid_metadata_type(uint8_t type, uint8_t len)
{
	switch (type) {
	case BT_AUDIO_METADATA_TYPE_PREF_CONTEXT:
	case BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT:
		if (len != 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_STREAM_LANG:
		if (len != 3) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PARENTAL_RATING:
		if (len != 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 1 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR: /* 1 - 255 octets */
		if (len < 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST: /* 2 - 254 octets */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO: /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

static int lc3_metadata(struct bt_audio_stream *stream, const struct bt_codec_data *meta,
			size_t meta_count)
{
	shell_print(ctx_shell, "Metadata: stream %p meta_count %zu", stream,
		    meta_count);

	for (size_t i = 0; i < meta_count; i++) {
		if (!valid_metadata_type(meta->data.type, meta->data.data_len)) {
			shell_print(ctx_shell,
				    "Invalid metadata type %u or length %u",
				    meta->data.type, meta->data.data_len);

			return -EINVAL;
		}
	}

	return 0;
}

static int lc3_disable(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Disable: stream %p", stream);

	return 0;
}

static int lc3_stop(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Stop: stream %p", stream);

	return 0;
}

static int lc3_release(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Release: stream %p", stream);

	if (stream == default_stream) {
		default_stream = NULL;
	}

	return 0;
}

static struct bt_codec lc3_codec = BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY,
						BT_CODEC_LC3_DURATION_ANY,
						BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1, 2), 30, 240, 2,
						(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
						BT_AUDIO_CONTEXT_TYPE_MEDIA));

static const struct bt_audio_unicast_server_cb unicast_server_cb = {
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

static struct bt_pacs_cap cap_sink = {
	.codec = &lc3_codec,
};

static struct bt_pacs_cap cap_source = {
	.codec = &lc3_codec,
};
#if defined(CONFIG_BT_AUDIO_UNICAST)


static uint16_t strmeta(const char *name)
{
	if (strcmp(name, "Unspecified") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
	} else if (strcmp(name, "Conversational") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL;
	} else if (strcmp(name, "Media") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_MEDIA;
	} else if (strcmp(name, "Game") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_GAME;
	} else if (strcmp(name, "Instructional") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL;
	} else if (strcmp(name, "VoiceAssistants") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS;
	} else if (strcmp(name, "Live") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_LIVE;
	} else if (strcmp(name, "SoundEffects") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS;
	} else if (strcmp(name, "Notifications") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS;
	} else if (strcmp(name, "Ringtone") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_RINGTONE;
	} else if (strcmp(name, "Alerts") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_ALERTS;
	} else if (strcmp(name, "EmergencyAlarm") == 0) {
		return BT_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM;
	}

	return 0u;
}

static int handle_metadata_update(const char *meta_str,
				  struct bt_codec_data *meta_out[],
				  size_t *meta_count_out)
{
	static struct bt_codec_data meta[CONFIG_BT_CODEC_MAX_METADATA_COUNT];
	size_t meta_count;

	/* We create a copy of the preset meta, as the presets cannot be modified */
	meta_count = default_preset->preset.codec.meta_count;
	(void)memset(meta, 0, sizeof(meta));
	for (size_t i = 0U; i < meta_count; i++) {
		(void)memcpy(meta[i].value,
			     default_preset->preset.codec.meta[i].data.data,
			     default_preset->preset.codec.meta[i].data.data_len);
		meta[i].data.type = default_preset->preset.codec.meta[i].data.type;
		meta[i].data.data_len = default_preset->preset.codec.meta[i].data.data_len;
		meta[i].data.data = meta[i].value;
	}

	if (meta_str != NULL) {
		uint16_t context;

		context = strmeta(meta_str);
		if (context == 0) {
			return -ENOEXEC;
		}

		/* TODO: Check the type and only overwrite the streaming context */
		sys_put_le16(context, meta[0].value);
	}

	*meta_count_out = meta_count;
	*meta_out = meta;

	return 0;
}

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
static uint8_t stream_dir(const struct bt_audio_stream *stream)
{
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0; i < ARRAY_SIZE(snks); i++) {
		if (snks[i] != NULL && stream->ep == snks[i]) {
			return BT_AUDIO_DIR_SINK;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0; i < ARRAY_SIZE(srcs); i++) {
		if (srcs[i] != NULL && stream->ep == srcs[i]) {
			return BT_AUDIO_DIR_SOURCE;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	__ASSERT(false, "Invalid stream");
	return 0;
}

static void add_codec(struct bt_codec *codec, uint8_t index, enum bt_audio_dir dir)
{
	shell_print(ctx_shell, "#%u: codec %p dir 0x%02x", index, codec, dir);

	print_codec(codec);

	if (dir != BT_AUDIO_DIR_SINK && dir != BT_AUDIO_DIR_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT) {
		rcodecs[dir - 1][index] = codec;
	}
}

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
static void add_sink(struct bt_audio_ep *ep, uint8_t index)
{
	shell_print(ctx_shell, "Sink #%u: ep %p", index, ep);

	snks[index] = ep;
}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
static void add_source(struct bt_audio_ep *ep, uint8_t index)
{
	shell_print(ctx_shell, "Source #%u: ep %p", index, ep);

	srcs[index] = ep;
}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

static void discover_cb(struct bt_conn *conn, struct bt_codec *codec,
			struct bt_audio_ep *ep,
			struct bt_audio_discover_params *params)
{
	if (codec != NULL) {
		add_codec(codec, params->num_caps, params->dir);
		return;
	}

	if (ep) {
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
		if (params->dir == BT_AUDIO_DIR_SINK) {
			add_sink(ep, params->num_eps);
		}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
		if (params->dir == BT_AUDIO_DIR_SOURCE) {
			add_source(ep, params->num_eps);
		}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0*/

		return;
	}

	shell_print(ctx_shell, "Discover complete: err %d", params->err);

	memset(params, 0, sizeof(*params));
}

static void discover_all(struct bt_conn *conn, struct bt_codec *codec,
			struct bt_audio_ep *ep,
			struct bt_audio_discover_params *params)
{
	if (codec != NULL) {
		add_codec(codec, params->num_caps, params->dir);
		return;
	}

	if (ep) {
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
		if (params->dir == BT_AUDIO_DIR_SINK) {
			add_sink(ep, params->num_eps);
		}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
		if (params->dir == BT_AUDIO_DIR_SOURCE) {
			add_source(ep, params->num_eps);
		}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0*/

		return;
	}

	/* Sinks discovery complete, now discover sources */
	if (params->dir == BT_AUDIO_DIR_SINK) {
		int err;

		params->func = discover_cb;
		params->dir = BT_AUDIO_DIR_SOURCE;

		err = bt_audio_discover(default_conn, params);
		if (err) {
			shell_error(ctx_shell, "bt_audio_discover err %d", err);
			discover_cb(conn, NULL, NULL, params);
		}
	}
}

static void unicast_client_location_cb(struct bt_conn *conn,
				      enum bt_audio_dir dir,
				      enum bt_audio_location loc)
{
	shell_print(ctx_shell, "dir %u loc %X\n", dir, loc);
}

static void available_contexts_cb(struct bt_conn *conn,
				  enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	shell_print(ctx_shell, "snk ctx %u src ctx %u\n", snk_ctx, src_ctx);
}

const struct bt_audio_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
};

static int cmd_discover(const struct shell *sh, size_t argc, char *argv[])
{
	static struct bt_audio_discover_params params;
	static bool cbs_registered;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!initialized) {
		shell_error(sh, "Not initialized");
		return -ENOEXEC;
	}

	if (params.func) {
		shell_error(sh, "Discover in progress");
		return -ENOEXEC;
	}

	if (!cbs_registered) {
		int err = bt_audio_unicast_client_register_cb(&unicast_client_cbs);

		if (err != 0) {
			shell_error(sh, "Failed to register unicast client callbacks: %d", err);
			return err;
		}

		cbs_registered = true;
	}

	params.func = discover_all;
	params.dir = BT_AUDIO_DIR_SINK;

	if (argc > 1) {
		if (!strcmp(argv[1], "sink")) {
			params.func = discover_cb;
		} else if (!strcmp(argv[1], "source")) {
			params.func = discover_cb;
			params.dir = BT_AUDIO_DIR_SOURCE;
		} else {
			shell_error(sh, "Unsupported dir: %s", argv[1]);
			return -ENOEXEC;
		}
	}

	return bt_audio_discover(default_conn, &params);
}

static int cmd_config(const struct shell *sh, size_t argc, char *argv[])
{
	int32_t index, dir;
	struct bt_audio_ep *ep = NULL;
	struct named_lc3_preset *named_preset;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = strtol(argv[2], NULL, 0);
	if (index < 0) {
		shell_error(sh, "Invalid index");
		return -ENOEXEC;
	}

	if (false) {

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	} else if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
		ep = snks[index];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
		ep = srcs[index];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	if (!ep) {
		shell_error(sh, "Unable to find endpoint");
		return -ENOEXEC;
	}

	named_preset = default_preset;

	if (argc > 3) {
		named_preset = set_preset(true, 1, argv + 3);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s",
				    argv[4]);
			return -ENOEXEC;
		}
	}

	if (default_stream && default_stream->ep == ep) {
		if (bt_audio_stream_reconfig(default_stream,
					     &named_preset->preset.codec) < 0) {
			shell_error(sh, "Unable reconfig stream");
			return -ENOEXEC;
		}
	} else {
		struct bt_audio_stream *stream;
		int err;

		if (default_stream == NULL) {
			stream = &streams[0];
		} else {
			stream = default_stream;
		}

		err = bt_audio_stream_config(default_conn, stream, ep,
					     &named_preset->preset.codec);
		if (err != 0) {
			shell_error(sh, "Unable to config stream: %d", err);
			return err;
		}

		default_stream = stream;
	}

	shell_print(sh, "ASE config: preset %s", named_preset->name);

	return 0;
}

static int cmd_qos(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct named_lc3_preset *named_preset = NULL;

	if (default_stream == NULL) {
		shell_print(sh, "No stream selected");
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	named_preset = default_preset;

	if (argc > 1) {
		named_preset = set_preset(true, argc - 1, argv + 1);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	if (default_unicast_group == NULL) {
		struct bt_audio_unicast_group_stream_param stream_param = {
			.stream = default_stream,
			.qos = &default_preset->preset.qos,
		};
		struct bt_audio_unicast_group_stream_pair_param pair_param;
		struct bt_audio_unicast_group_param param = {
			.packing = BT_ISO_PACKING_SEQUENTIAL,
			.params = &pair_param,
			.params_count = 1,
		};

		if (stream_dir(default_stream) == BT_AUDIO_DIR_SOURCE) {
			pair_param.rx_param = &stream_param;
			pair_param.tx_param = NULL;
		} else {
			pair_param.rx_param = NULL;
			pair_param.tx_param = &stream_param;
		}

		err = bt_audio_unicast_group_create(&param, &default_unicast_group);
		if (err != 0) {
			shell_error(sh, "Unable to create default unicast group: %d", err);
			return -ENOEXEC;
		}
	}

	err = bt_audio_stream_qos(default_conn, default_unicast_group);
	if (err) {
		shell_error(sh, "Unable to setup QoS: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "ASE config: preset %s", named_preset->name);

	return 0;
}

static int cmd_enable(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_codec_data *meta;
	size_t meta_count;
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		err = handle_metadata_update(argv[1], &meta, &meta_count);
	} else {
		err = handle_metadata_update(NULL, &meta, &meta_count);
	}

	if (err != 0) {
		shell_error(sh, "Unable to handle metadata update: %d", err);
		return err;
	}

	err = bt_audio_stream_enable(default_stream, meta, meta_count);
	if (err) {
		shell_error(sh, "Unable to enable Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_stop(default_stream);
	if (err) {
		shell_error(sh, "Unable to stop Channel");
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

static int cmd_preset(const struct shell *sh, size_t argc, char *argv[])
{
	struct named_lc3_preset *named_preset;

	named_preset = default_preset;

	if (argc > 1) {
		named_preset = set_preset(true, argc - 1, argv + 1);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	shell_print(sh, "%s", named_preset->name);

	print_codec(&named_preset->preset.codec);
	print_qos(&named_preset->preset.qos);

	return 0;
}

#define MAX_META_DATA \
	(CONFIG_BT_CODEC_MAX_METADATA_COUNT * sizeof(struct bt_codec_data))

static int cmd_metadata(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_codec_data *meta;
	size_t meta_count;
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		err = handle_metadata_update(argv[1], &meta, &meta_count);
	} else {
		err = handle_metadata_update(NULL, &meta, &meta_count);
	}

	if (err != 0) {
		shell_error(sh, "Unable to handle metadata update: %d", err);
		return err;
	}

	err = bt_audio_stream_metadata(default_stream,
				       default_preset->preset.codec.meta,
				       default_preset->preset.codec.meta_count);
	if (err) {
		shell_error(sh, "Unable to set Channel metadata");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_start(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_start(default_stream);
	if (err) {
		shell_error(sh, "Unable to start Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_disable(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_disable(default_stream);
	if (err) {
		shell_error(sh, "Unable to disable Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_list(const struct shell *sh, size_t argc, char *argv[])
{
	int i;

	shell_print(sh, "Configured Channels:");

	for (i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_audio_stream *stream = &streams[i];

		if (stream->conn) {
			shell_print(sh, "  %s#%u: stream %p ep %p group %p",
				    stream == default_stream ? "*" : " ", i,
				    stream, stream->ep, stream->group);
		}
	}

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	shell_print(sh, "Sinks:");

	for (i = 0; i < ARRAY_SIZE(snks); i++) {
		struct bt_audio_ep *ep = snks[i];

		if (ep) {
			shell_print(sh, "  #%u: ep %p", i, ep);
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	shell_print(sh, "Sources:");

	for (i = 0; i < ARRAY_SIZE(srcs); i++) {
		struct bt_audio_ep *ep = srcs[i];

		if (ep) {
			shell_print(sh, "  #%u: ep %p", i, ep);
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

	return 0;
}

static int cmd_release(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_print(sh, "No stream selected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_release(default_stream);
	if (err) {
		shell_error(sh, "Unable to release Channel");
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_AUDIO_UNICAST */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static uint32_t accepted_broadcast_id;
static struct bt_audio_base received_base;
static bool sink_syncable;

static bool broadcast_scan_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *ad,
				uint32_t broadcast_id)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	shell_print(ctx_shell, "Found broadcaster with ID 0x%06X and addr %s",
		    broadcast_id, le_addr);

	if (broadcast_id == accepted_broadcast_id) {
		shell_print(ctx_shell, "PA syncing to broadcaster");
		accepted_broadcast_id = 0;
		return true;
	}

	return false;
}

static void pa_synced(struct bt_audio_broadcast_sink *sink,
		      struct bt_le_per_adv_sync *sync,
		      uint32_t broadcast_id)
{
	shell_print(ctx_shell,
		    "PA synced to broadcaster with ID 0x%06X as sink %p",
		    broadcast_id, sink);

	if (default_sink == NULL) {
		default_sink = sink;

		shell_print(ctx_shell, "Sink %p is set as default", sink);
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

	shell_print(ctx_shell, "Received BASE from sink %p:", sink);

	for (int i = 0; i < base->subgroup_count; i++) {
		const struct bt_audio_base_subgroup *subgroup;

		subgroup = &base->subgroups[i];

		shell_print(ctx_shell, "Subgroup[%d]:", i);
		print_codec(&subgroup->codec);

		for (int j = 0; j < subgroup->bis_count; j++) {
			const struct bt_audio_base_bis_data *bis_data;

			bis_data = &subgroup->bis_data[j];

			shell_print(ctx_shell, "BIS[%d] index 0x%02x",
				    i, bis_data->index);
			bis_indexes[index_count++] = bis_data->index;

			for (int k = 0; k < bis_data->data_count; k++) {
				const struct bt_codec_data *codec_data;

				codec_data = &bis_data->data[k];

				shell_print(ctx_shell,
					    "data #%u: type 0x%02x len %u",
					    i, codec_data->data.type,
					    codec_data->data.data_len);
				shell_hexdump(ctx_shell, codec_data->data.data,
					      codec_data->data.data_len -
						sizeof(codec_data->data.type));
			}
		}
	}

	memset(bis_indexes_str, 0, sizeof(bis_indexes_str));
	/* Create space separated list of indexes as hex values */
	for (int i = 0; i < index_count; i++) {
		char bis_index_str[6];

		sprintf(bis_index_str, "0x%02x ", bis_indexes[i]);

		strcat(bis_indexes_str, bis_index_str);
		shell_print(ctx_shell, "[%d]: %s", i, bis_index_str);
	}

	shell_print(ctx_shell, "Possible indexes: %s", bis_indexes_str);

	(void)memcpy(&received_base, base, sizeof(received_base));
}

static void syncable(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	if (sink_syncable) {
		return;
	}

	shell_print(ctx_shell, "Sink %p is ready to sync %s encryption",
		    sink, encrypted ? "with" : "without");
	sink_syncable = true;
}

static void scan_term(int err)
{
	shell_print(ctx_shell, "Broadcast scan was terminated: %d", err);

}

static void pa_sync_lost(struct bt_audio_broadcast_sink *sink)
{
	shell_warn(ctx_shell, "Sink %p disconnected", sink);

	if (default_sink == sink) {
		default_sink = NULL;
		sink_syncable = false;
		(void)memset(&received_base, 0, sizeof(received_base));
	}
}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static struct bt_audio_broadcast_sink_cb sink_cbs = {
	.scan_recv = broadcast_scan_recv,
	.pa_synced = pa_synced,
	.base_recv = base_recv,
	.syncable = syncable,
	.scan_term = scan_term,
	.pa_sync_lost = pa_sync_lost,
};
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static void audio_recv(struct bt_audio_stream *stream,
		       const struct bt_iso_recv_info *info,
		       struct net_buf *buf)
{
	static struct bt_iso_recv_info last_info;

	/* TODO: Make it possible to only print every X packets, and make X settable by the shell */
	if ((rx_cnt % 100) == 0) {
		shell_print(ctx_shell,
			    "[%zu]: Incoming audio on stream %p len %u ts %u seq_num %u flags %u",
			    rx_cnt, stream, buf->len, info->ts, info->seq_num,
			    info->flags);
	}

	if (info->ts == last_info.ts) {
		shell_error(ctx_shell, "[%zu]: Duplicate TS: %u",
			    rx_cnt, info->ts);
	}

	if (info->seq_num == last_info.seq_num) {
		shell_error(ctx_shell, "[%zu]: Duplicate seq_num: %u",
			    rx_cnt, info->seq_num);
	}

	(void)memcpy(&last_info, info, sizeof(last_info));

	rx_cnt++;
}
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SINK */

static void stream_started_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p started\n", stream);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p stopped\n", stream);


#if defined(CONFIG_LIBLC3)
	if (stream == default_stream) {
		lc3_start_time = 0;
		lc3_sdu_cnt = 0;

		k_work_cancel_delayable(&audio_send_work);
	}
#endif /* CONFIG_LIBLC3 */
}

#if defined(CONFIG_BT_AUDIO_UNICAST)
static void stream_released_cb(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Stream %p released\n", stream);

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
	/* The current shell application only supports a single stream in
	 * the unicast group, so when that gets disconnected, we delete the
	 * unicast group so that it can be recreated when settings the QoS
	 */
	if (default_unicast_group != NULL) {
		int err = bt_audio_unicast_group_delete(default_unicast_group);

		if (err != 0) {
			shell_error(ctx_shell,
				    "Failed to delete unicast group: %d",
				    err);
		} else {
			default_unicast_group = NULL;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

#if defined(CONFIG_LIBLC3)
	/* stop sending */
	if (stream == default_stream) {
		lc3_start_time = 0;
		lc3_sdu_cnt = 0;

		k_work_cancel_delayable(&audio_send_work);
	}
#endif /* CONFIG_LIBLC3 */
}
#endif /* CONFIG_BT_AUDIO_UNICAST */

static struct bt_audio_stream_ops stream_ops = {
#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	.recv = audio_recv,
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SINK */
#if defined(CONFIG_BT_AUDIO_UNICAST)
	.released = stream_released_cb,
#endif /* CONFIG_BT_AUDIO_UNICAST */
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
};

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
static int cmd_select_broadcast_source(const struct shell *sh, size_t argc,
				       char *argv[])
{
	struct bt_audio_stream *stream;
	uint8_t index;

	index = strtol(argv[1], NULL, 0);
	if (index < 0 || index > ARRAY_SIZE(broadcast_source_streams)) {
		shell_error(sh, "Invalid index: %d", index);
		return -ENOEXEC;
	}

	stream = &broadcast_source_streams[index];

	set_stream(stream);

	return 0;
}

static int cmd_create_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	struct bt_audio_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_audio_broadcast_source_subgroup_param subgroup_param;
	struct bt_audio_broadcast_source_create_param create_param = { 0 };
	struct named_lc3_preset *named_preset;
	int err;

	if (default_source != NULL) {
		shell_info(sh, "Broadcast source already created");
		return -ENOEXEC;
	}

	named_preset = default_preset;

	for (size_t i = 1U; i < argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "enc") == 0) {
			if (argc > i) {
				size_t bcode_len;

				i++;
				arg = argv[i];

				bcode_len = hex2bin(arg, strlen(arg),
						    create_param.broadcast_code,
						    sizeof(create_param.broadcast_code));

				if (bcode_len != sizeof(create_param.broadcast_code)) {
					shell_error(sh, "Invalid Broadcast Code Length: %zu",
						    bcode_len);

					return -ENOEXEC;
				}

				create_param.encryption = true;
			} else {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}
		} else if (strcmp(arg, "preset") == 0) {
			if (argc > i) {

				i++;
				arg = argv[i];

				named_preset = set_preset(false, 1, &arg);
				if (named_preset == NULL) {
					shell_error(sh, "Unable to parse named_preset %s",
						    arg);

					return -ENOEXEC;
				}
			} else {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}
		}
	}

	(void)memset(stream_params, 0, sizeof(stream_params));
	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &broadcast_source_streams[i];
	}
	subgroup_param.params_count = ARRAY_SIZE(stream_params);
	subgroup_param.params = stream_params;
	subgroup_param.codec = &named_preset->preset.codec;
	create_param.params_count = 1U;
	create_param.params = &subgroup_param;
	create_param.qos = &named_preset->preset.qos;

	err = bt_audio_broadcast_source_create(&create_param, &default_source);
	if (err != 0) {
		shell_error(sh, "Unable to create broadcast source: %d", err);
		return err;
	}

	shell_print(sh, "Broadcast source created: preset %s",
		    named_preset->name);

	if (default_stream == NULL) {
		default_stream = &broadcast_source_streams[0];
	}

	return 0;
}

static int cmd_start_broadcast(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (adv == NULL) {
		shell_info(sh, "Extended advertising set is NULL");
		return -ENOEXEC;
	}

	if (default_source == NULL) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_source_start(default_source,
					      adv_sets[selected_adv]);
	if (err != 0) {
		shell_error(sh, "Unable to start broadcast source: %d", err);
		return err;
	}

	return 0;
}

static int cmd_stop_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_source == NULL) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_source_stop(default_source);
	if (err != 0) {
		shell_error(sh, "Unable to stop broadcast source: %d", err);
		return err;
	}

	return 0;
}

static int cmd_delete_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	int err;

	if (default_source == NULL) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_source_delete(default_source);
	if (err != 0) {
		shell_error(sh, "Unable to delete broadcast source: %d", err);
		return err;
	}
	default_source = NULL;

	return 0;
}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static int cmd_broadcast_scan(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_le_scan_param param = {
			.type       = BT_LE_SCAN_TYPE_ACTIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW,
			.timeout    = 0 };

	if (!initialized) {
		shell_error(sh, "Not initialized");
		return -ENOEXEC;
	}

	if (strcmp(argv[1], "on") == 0) {
		err =  bt_audio_broadcast_sink_scan_start(&param);
		if (err != 0) {
			shell_error(sh, "Could not start scan: %d", err);
		}
	} else if (strcmp(argv[1], "off") == 0) {
		err = bt_audio_broadcast_sink_scan_stop();
		if (err != 0) {
			shell_error(sh, "Could not stop scan: %d", err);
		}
	} else {
		shell_help(sh);
		err = -ENOEXEC;
	}

	return err;
}

static int cmd_accept_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	accepted_broadcast_id = strtoul(argv[1], NULL, 16);

	return 0;
}

static int cmd_sync_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_stream *streams[ARRAY_SIZE(broadcast_sink_streams)];
	uint32_t bis_bitfield;
	int err;

	bis_bitfield = 0;
	for (int i = 1; i < argc; i++) {
		unsigned long val = strtoul(argv[i], NULL, 16);

		if (val < 0x01 || val > 0x1F) {
			shell_error(sh, "Invalid index: %ld", val);
		}
		bis_bitfield |= BIT(val);
	}

	if (default_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	(void)memset(streams, 0, sizeof(streams));
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i] = &broadcast_sink_streams[i];
	}

	err = bt_audio_broadcast_sink_sync(default_sink, bis_bitfield,
					   streams, NULL);
	if (err != 0) {
		shell_error(sh, "Failed to sync to broadcast: %d", err);
		return err;
	}

	return 0;
}

static int cmd_stop_broadcast_sink(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int err;

	if (default_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_sink_stop(default_sink);
	if (err != 0) {
		shell_error(sh, "Failed to stop sink: %d", err);
		return err;
	}

	return err;
}

static int cmd_term_broadcast_sink(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int err;

	if (default_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_sink_delete(default_sink);
	if (err != 0) {
		shell_error(sh, "Failed to term sink: %d", err);
		return err;
	}

	default_sink = NULL;
	sink_syncable = false;

	return err;
}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

static int cmd_set_loc(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_audio_dir dir;
	enum bt_audio_location loc;

	if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	loc = shell_strtoul(argv[2], 16, &err);
	err = bt_pacs_set_location(dir, loc);
	if (err) {
		shell_error(ctx_shell, "Set available contexts err %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_context(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_audio_dir dir;
	enum bt_audio_context ctx;

	if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	ctx = shell_strtoul(argv[2], 16, &err);
	if (err) {
		shell_error(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	if (!strcmp(argv[3], "supported")) {
		err = bt_pacs_set_supported_contexts(dir, ctx);
		if (err) {
			shell_error(ctx_shell, "Set supported contexts err %d", err);
			return err;
		}
	} else if (!strcmp(argv[3], "available")) {
		err = bt_pacs_set_available_contexts(dir, ctx);
		if (err) {
			shell_error(ctx_shell, "Set available contexts err %d", err);
			return err;
		}
	} else {
		shell_error(sh, "Unsupported context type: %s", argv[3]);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_init(const struct shell *sh, size_t argc, char *argv[])
{
	int err, i;

	ctx_shell = sh;

	if (initialized) {
		shell_print(sh, "Already initialized");
		return -ENOEXEC;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_SERVER)) {
		bt_audio_unicast_server_register_cb(&unicast_server_cb);
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_SERVER) ||
	    IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_SINK)) {
		bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap_sink);
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_SERVER)) {
		bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap_source);
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, LOCATION);
		__ASSERT(err == 0, "Failed to set sink location: %d", err);

		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
						     CONTEXT);
		__ASSERT(err == 0, "Failed to set sink supported contexts: %d",
			 err);

		err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
						     CONTEXT);
		__ASSERT(err == 0, "Failed to set sink available contexts: %d",
			 err);
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, LOCATION);
		__ASSERT(err == 0, "Failed to set source location: %d", err);

		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
						     CONTEXT);
		__ASSERT(err == 0, "Failed to set sink supported contexts: %d",
			 err);

		err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
						     CONTEXT);
		__ASSERT(err == 0,
			 "Failed to set source available contexts: %d",
			 err);
	}

#if defined(CONFIG_BT_AUDIO_UNICAST)
	for (i = 0; i < ARRAY_SIZE(streams); i++) {
		bt_audio_stream_cb_register(&streams[i], &stream_ops);
	}
#endif /* CONFIG_BT_AUDIO_UNICAST */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	bt_audio_broadcast_sink_register_cb(&sink_cbs);

	for (i = 0; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		bt_audio_stream_cb_register(&broadcast_sink_streams[i],
					    &stream_ops);
	}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	for (i = 0; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		bt_audio_stream_cb_register(&broadcast_source_streams[i],
					    &stream_ops);
	}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

	initialized = true;

	return 0;
}

#define DATA_MTU CONFIG_BT_ISO_TX_MTU
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, DATA_MTU, 8, NULL);

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	static uint8_t data[DATA_MTU - BT_ISO_CHAN_SEND_RESERVE];
	int ret, len;
	struct net_buf *buf;

	if (argc > 1) {
		len = hex2bin(argv[1], strlen(argv[1]), data, sizeof(data));
		if (len > default_preset->preset.qos.sdu) {
			shell_print(sh, "Unable to send: len %d > %u MTU",
				    len, default_preset->preset.qos.sdu);
			return -ENOEXEC;
		}
	} else {
		len = MIN(default_preset->preset.qos.sdu, sizeof(data));
		memset(data, 0xff, len);
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, data, len);

	seq_num = get_next_seq_num(default_preset->preset.qos.interval);

	ret = bt_audio_stream_send(default_stream, buf, seq_num,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		shell_print(sh, "Unable to send: %d", -ret);
		net_buf_unref(buf);
		return -ENOEXEC;
	}
	shell_print(sh, "Sending:");
	shell_hexdump(sh, data, len);

	return 0;
}

#if defined(CONFIG_LIBLC3)
static int cmd_start_sine(const struct shell *sh, size_t argc, char *argv[])
{
	k_work_schedule(&audio_send_work, K_MSEC(0));

	return 0;
}

static int cmd_stop_sine(const struct shell *sh, size_t argc, char *argv[])
{
	lc3_start_time = 0;
	lc3_sdu_cnt = 0;

	k_work_cancel_delayable(&audio_send_work);

	return 0;
}
#endif /* CONFIG_LIBLC3 */

SHELL_STATIC_SUBCMD_SET_CREATE(audio_cmds,
	SHELL_CMD_ARG(init, NULL, NULL, cmd_init, 1, 0),
#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	SHELL_CMD_ARG(select_broadcast, NULL, "<stream>",
		      cmd_select_broadcast_source, 2, 0),
	SHELL_CMD_ARG(create_broadcast, NULL,
		      "[preset <preset_name>] [enc <broadcast_code>]",
		      cmd_create_broadcast, 1, 2),
	SHELL_CMD_ARG(start_broadcast, NULL, "", cmd_start_broadcast, 1, 0),
	SHELL_CMD_ARG(stop_broadcast, NULL, "", cmd_stop_broadcast, 1, 0),
	SHELL_CMD_ARG(delete_broadcast, NULL, "", cmd_delete_broadcast, 1, 0),
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	SHELL_CMD_ARG(broadcast_scan, NULL, "<on, off>",
		      cmd_broadcast_scan, 2, 0),
	SHELL_CMD_ARG(accept_broadcast, NULL, "0x<broadcast_id>",
		      cmd_accept_broadcast, 2, 0),
	SHELL_CMD_ARG(sync_broadcast, NULL,
		      "0x<bis_index> [[[0x<bis_index>] 0x<bis_index>] ...]",
		      cmd_sync_broadcast, 2,
		      ARRAY_SIZE(broadcast_sink_streams) - 1),
	SHELL_CMD_ARG(stop_broadcast_sink, NULL, "Stops broadcast sink",
		      cmd_stop_broadcast_sink, 1, 0),
	SHELL_CMD_ARG(term_broadcast_sink, NULL, "",
		      cmd_term_broadcast_sink, 1, 0),
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */
#if defined(CONFIG_BT_AUDIO_UNICAST)
#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
	SHELL_CMD_ARG(discover, NULL, "[dir: sink, source]",
		      cmd_discover, 1, 1),
	SHELL_CMD_ARG(config, NULL,
		      "<direction: sink, source> <index> [codec] [preset]",
		      cmd_config, 3, 2),
	SHELL_CMD_ARG(qos, NULL,
		      "[preset] [interval] [framing] [latency] [pd] [sdu] [phy]"
		      " [rtn]", cmd_qos, 1, 8),
	SHELL_CMD_ARG(enable, NULL, NULL, cmd_enable, 1, 1),
	SHELL_CMD_ARG(stop, NULL, NULL, cmd_stop, 1, 0),
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
	SHELL_CMD_ARG(preset, NULL, "[preset]", cmd_preset, 1, 1),
	SHELL_CMD_ARG(metadata, NULL, "[context]", cmd_metadata, 1, 1),
	SHELL_CMD_ARG(start, NULL, NULL, cmd_start, 1, 0),
	SHELL_CMD_ARG(disable, NULL, NULL, cmd_disable, 1, 0),
	SHELL_CMD_ARG(release, NULL, NULL, cmd_release, 1, 0),
	SHELL_CMD_ARG(list, NULL, NULL, cmd_list, 1, 0),
	SHELL_CMD_ARG(select_unicast, NULL, "<stream>",
		      cmd_select_unicast, 2, 0),
#endif /* CONFIG_BT_AUDIO_UNICAST */
	SHELL_CMD_ARG(send, NULL, "Send to Audio Stream [data]",
		      cmd_send, 1, 1),
#if defined(CONFIG_LIBLC3)
	SHELL_CMD_ARG(start_sine, NULL, "Start sending a LC3 encoded sine wave",
			   cmd_start_sine, 1, 0),
	SHELL_CMD_ARG(stop_sine, NULL, "Stop sending a LC3 encoded sine wave",
			   cmd_stop_sine, 1, 0),
#endif /* CONFIG_LIBLC3 */
	SHELL_COND_CMD_ARG(CONFIG_BT_PACS, set_location, NULL,
			   "<direction: sink, source> <location bitmask>",
			   cmd_set_loc, 3, 0),
	SHELL_COND_CMD_ARG(CONFIG_BT_PACS, set_context, NULL,
			   "<direction: sink, source>"
			   "<context bitmask> <type: supported, available>",
			   cmd_context, 4, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_audio(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(audio, &audio_cmds, "Bluetooth audio shell commands",
		       cmd_audio, 1, 1);

static ssize_t connectable_ad_data_add(struct bt_data *data_array,
				       size_t data_array_size)
{
	static const uint8_t ad_ext_uuid16[] = {
		IF_ENABLED(CONFIG_BT_MICP_MIC_DEV, (BT_UUID_16_ENCODE(BT_UUID_MICS_VAL),))
		IF_ENABLED(CONFIG_BT_ASCS, (BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),))
		IF_ENABLED(CONFIG_BT_BAP_SCAN_DELEGATOR, (BT_UUID_16_ENCODE(BT_UUID_BASS_VAL),))
		IF_ENABLED(CONFIG_BT_PACS, (BT_UUID_16_ENCODE(BT_UUID_PACS_VAL),))
		IF_ENABLED(CONFIG_BT_GTBS, (BT_UUID_16_ENCODE(BT_UUID_GTBS_VAL),))
		IF_ENABLED(CONFIG_BT_TBS, (BT_UUID_16_ENCODE(BT_UUID_TBS_VAL),))
		IF_ENABLED(CONFIG_BT_VCP_VOL_REND, (BT_UUID_16_ENCODE(BT_UUID_VCS_VAL),))
		IF_ENABLED(CONFIG_BT_HAS, (BT_UUID_16_ENCODE(BT_UUID_HAS_VAL),)) /* Shall be last */
	};
	size_t ad_len = 0;

	if (IS_ENABLED(CONFIG_BT_ASCS)) {
		static uint8_t ad_bap_announcement[8] = {
			BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),
			BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
		};
		enum bt_audio_context snk_context, src_context;

		snk_context = bt_pacs_get_available_contexts(BT_AUDIO_DIR_SINK);
		sys_put_le16(snk_context, &ad_bap_announcement[3]);

		src_context = bt_pacs_get_available_contexts(BT_AUDIO_DIR_SOURCE);
		sys_put_le16(snk_context, &ad_bap_announcement[5]);

		/* Metadata length */
		ad_bap_announcement[7] = 0x00;

		__ASSERT(data_array_size > ad_len, "No space for AD_BAP_ANNOUNCEMENT");
		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_bap_announcement);
		data_array[ad_len].data = &ad_bap_announcement[0];
		ad_len++;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR)) {
		static const uint8_t ad_cap_announcement[3] = {
			BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
			BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
		};

		__ASSERT(data_array_size > ad_len, "No space for AD_CAP_ANNOUNCEMENT");
		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_cap_announcement);
		data_array[ad_len].data = &ad_cap_announcement[0];
		ad_len++;
	}

	if (ARRAY_SIZE(ad_ext_uuid16) > 0) {
		if (data_array_size <= ad_len) {
			shell_warn(ctx_shell, "No space for AD_UUID16");
			return ad_len;
		}

		data_array[ad_len].type = BT_DATA_UUID16_SOME;

		if (IS_ENABLED(CONFIG_BT_HAS) && IS_ENABLED(CONFIG_BT_PRIVACY)) {
			/* If the HA is in one of the GAP connectable modes and is using a
			 * resolvable private address, the HA shall not include the Hearing Access
			 * Service UUID in the Service UUID AD type field of the advertising data
			 * or scan response.
			 */
			data_array[ad_len].data_len = ARRAY_SIZE(ad_ext_uuid16) - BT_UUID_SIZE_16;
		} else {
			data_array[ad_len].data_len = ARRAY_SIZE(ad_ext_uuid16);
		}

		data_array[ad_len].data = &ad_ext_uuid16[0];
		ad_len++;
	}

	return ad_len;
}

static ssize_t nonconnectable_ad_data_add(struct bt_data *data_array,
					  const size_t data_array_size)
{
	static const uint8_t ad_ext_uuid16[] = {
		IF_ENABLED(CONFIG_BT_PACS, (BT_UUID_16_ENCODE(BT_UUID_PACS_VAL),))
		IF_ENABLED(CONFIG_BT_CAP_ACCEPTOR, (BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),))
	};
	size_t ad_len = 0;

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR)) {
		static const uint8_t ad_cap_announcement[3] = {
			BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
			BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
		};

		__ASSERT(data_array_size > ad_len, "No space for AD_CAP_ANNOUNCEMENT");
		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_cap_announcement);
		data_array[ad_len].data = &ad_cap_announcement[0];
		ad_len++;
	}

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	if (default_source) {
		static uint8_t ad_bap_broadcast_announcement[5] = {
			BT_UUID_16_ENCODE(BT_UUID_BROADCAST_AUDIO_VAL),
		};
		uint32_t broadcast_id;
		int err;

		err = bt_audio_broadcast_source_get_id(default_source,
						       &broadcast_id);
		if (err != 0) {
			printk("Unable to get broadcast ID: %d\n", err);

			return -1;
		}

		ad_bap_broadcast_announcement[2] = (uint8_t)(broadcast_id >> 16);
		ad_bap_broadcast_announcement[3] = (uint8_t)(broadcast_id >> 8);
		ad_bap_broadcast_announcement[4] = (uint8_t)(broadcast_id >> 0);
		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_bap_broadcast_announcement);
		data_array[ad_len].data = ad_bap_broadcast_announcement;
		ad_len++;
	}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

	if (ARRAY_SIZE(ad_ext_uuid16) > 0) {
		if (data_array_size <= ad_len) {
			shell_warn(ctx_shell, "No space for AD_UUID16");
			return ad_len;
		}

		data_array[ad_len].type = BT_DATA_UUID16_SOME;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_ext_uuid16);
		data_array[ad_len].data = &ad_ext_uuid16[0];
		ad_len++;
	}

	return ad_len;
}

ssize_t audio_ad_data_add(struct bt_data *data_array, const size_t data_array_size,
			  const bool discoverable, const bool connectable)
{
	if (!discoverable) {
		return 0;
	}

	if (connectable) {
		return connectable_ad_data_add(data_array, data_array_size);
	} else {
		return nonconnectable_ad_data_add(data_array, data_array_size);
	}
}

ssize_t audio_pa_data_add(struct bt_data *data_array,
			  const size_t data_array_size)
{
	size_t ad_len = 0;

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	if (default_source) {
		/* Required size of the buffer depends on what has been
		 * configured. We just use the maximum size possible.
		 */
		NET_BUF_SIMPLE_DEFINE_STATIC(base_buf, UINT8_MAX);
		int err;

		err = bt_audio_broadcast_source_get_base(default_source,
							 &base_buf);
		if (err != 0) {
			printk("Unable to get BASE: %d\n", err);

			return -1;
		}

		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = base_buf.len;
		data_array[ad_len].data = base_buf.data;
		ad_len++;
	}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

	return ad_len;
}
