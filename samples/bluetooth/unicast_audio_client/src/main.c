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
#include <zephyr/sys/byteorder.h>

static void start_scan(void);

static struct bt_conn *default_conn;
static struct k_work_delayable audio_send_work;
static struct bt_audio_unicast_group *unicast_group;
static struct bt_codec *remote_codec_capabilities[CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];
static struct bt_audio_ep *sinks[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_audio_ep *sources[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT,
			  CONFIG_BT_ISO_TX_MTU + BT_ISO_CHAN_SEND_RESERVE,
			  8, NULL);

static struct bt_audio_stream streams[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT +
				      CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];
static size_t configured_sink_stream_count;
static size_t configured_stream_count;


/* Select a codec configuration to apply that is mandatory to support by both client and server.
 * Allows this sample application to work without logic to parse the codec capabilities of the
 * server and selection of an appropriate codec configuration.
 */
static struct bt_audio_lc3_preset codec_configuration = BT_AUDIO_LC3_UNICAST_PRESET_16_2_1;

static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_mtu_exchanged, 0, 1);
static K_SEM_DEFINE(sem_sinks_discovered, 0, 1);
static K_SEM_DEFINE(sem_sources_discovered, 0, 1);
static K_SEM_DEFINE(sem_stream_configured, 0, 1);
static K_SEM_DEFINE(sem_stream_qos, 0, 1);
static K_SEM_DEFINE(sem_stream_enabled, 0, 1);
static K_SEM_DEFINE(sem_stream_started, 0, 1);

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
static int freq_hz;
static int frame_duration_us;
static int frame_duration_100us;
static int frames_per_sdu;
static int octets_per_frame;


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

static void lc3_audio_timer_timeout(struct k_work *work)
{
	/* For the first call-back we push multiple audio frames to the buffer to use the
	 * controller ISO buffer to handle jitter.
	 */
	const uint8_t prime_count = 2;
	static int64_t start_time;
	static int32_t sdu_cnt;
	int32_t sdu_goal_cnt;
	int64_t uptime, run_time_ms, run_time_100us;

	k_work_schedule(&audio_send_work, K_USEC(codec_configuration.qos.interval));

	if (lc3_encoder == NULL) {
		printk("LC3 encoder not setup, cannot encode data.\n");
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

	/* PDU count calculations done in 100us units to allow 7.5ms framelength in fixed-point */
	run_time_100us = run_time_ms * 10;
	sdu_goal_cnt = run_time_100us / (frame_duration_100us * frames_per_sdu);

	/* Add primer value to ensure the controller do not run low on data due to jitter */
	sdu_goal_cnt += prime_count;

	printk("LC3 encode %d frames in %d SDUs\n", (sdu_goal_cnt - sdu_cnt) * frames_per_sdu,
						    (sdu_goal_cnt - sdu_cnt));

	while (sdu_cnt < sdu_goal_cnt) {
		const uint16_t tx_sdu_len = frames_per_sdu * octets_per_frame;
		struct net_buf *buf;
		uint8_t *net_buffer;
		off_t offset = 0;

		buf = net_buf_alloc(&tx_pool, K_FOREVER);
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

		for (size_t i = 0U; i < configured_sink_stream_count; i++) {
			struct net_buf *buf_to_send;
			int ret;

			/* Clone the buffer if sending on more than 1 stream */
			if (i == configured_sink_stream_count - 1) {
				buf_to_send = buf;
			} else {
				buf_to_send = net_buf_clone(buf, K_FOREVER);
			}

			ret = bt_audio_stream_send(&streams[i], buf_to_send);
			if (ret < 0) {
				printk("  Failed to send LC3 audio data on streams[%zu] (%d)\n",
				       i, ret);
				net_buf_unref(buf_to_send);
			} else {
				printk("  TX LC3 l on streams[%zu]: %zu\n",
				       tx_sdu_len, i);
				sdu_cnt++;
			}
		}
	}
}

static void init_lc3(void)
{
	unsigned int num_samples;

	freq_hz = bt_codec_cfg_get_freq(&codec_configuration.codec);
	frame_duration_us = bt_codec_cfg_get_frame_duration_us(&codec_configuration.codec);
	octets_per_frame = bt_codec_cfg_get_octets_per_frame(&codec_configuration.codec);
	frames_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(&codec_configuration.codec, true);
	octets_per_frame = bt_codec_cfg_get_octets_per_frame(&codec_configuration.codec);

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
	for (unsigned int i = 0; i < num_samples; i++) {
		printk("%3i: %6i\n", i, audio_buf[i]);
	}

	/* Create the encoder instance. This shall complete before stream_started() is called. */
	lc3_encoder = lc3_setup_encoder(frame_duration_us,
					freq_hz,
					0, /* No resampling */
					&lc3_encoder_mem);

	if (lc3_encoder == NULL) {
		printk("ERROR: Failed to setup LC3 encoder - wrong parameters?\n");
	}
}

#else

#define init_lc3(...)

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

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, buf_data, len_to_send);

	/* We configured the sink streams to be first in `streams`, so that
	 * we can use `stream[i]` to select sink streams (i.e. streams with
	 * data going to the server)
	 */
	for (size_t i = 0U; i < configured_sink_stream_count; i++) {
		struct net_buf *buf_to_send;
		int ret;

		/* Clone the buffer if sending on more than 1 stream */
		if (i == configured_sink_stream_count - 1) {
			buf_to_send = buf;
		} else {
			buf_to_send = net_buf_clone(buf, K_FOREVER);
		}

		ret = bt_audio_stream_send(&streams[i], buf_to_send);
		if (ret < 0) {
			printk("Failed to send audio data on streams[%zu]: (%d)\n",
			       i, ret);
			net_buf_unref(buf_to_send);
		} else {
			printk("Sending mock data with len %zu on streams[%zu]\n",
			       len_to_send, i);
		}
	}

	k_work_schedule(&audio_send_work, K_MSEC(1000));

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}

#endif

static void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

static void print_codec_capabilities(const struct bt_codec *codec)
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

static bool check_audio_support_and_connect(struct bt_data *data,
					    void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			printk("AD malformed\n");
			return true; /* Continue */
		}

		for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			struct bt_uuid *uuid;
			uint16_t uuid_val;
			int err;

			memcpy(&uuid_val, &data->data[i], sizeof(uuid_val));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(uuid_val));
			if (bt_uuid_cmp(uuid, BT_UUID_ASCS) != 0) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err != 0) {
				printk("Failed to stop scan: %d\n", err);
				return false;
			}

			printk("Audio server found; connecting\n");

			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
						BT_LE_CONN_PARAM_DEFAULT,
						&default_conn);
			if (err != 0) {
				printk("Create conn to failed (%u)\n", err);
				start_scan();
			}

			return false; /* Stop parsing */
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn != NULL) {
		/* Already connected */
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND &&
	    type != BT_GAP_ADV_TYPE_EXT_ADV) {
		return;
	}

	(void)bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		return;
	}

	bt_data_parse(ad, check_audio_support_and_connect, (void *)addr);
}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void stream_configured(struct bt_audio_stream *stream,
			      const struct bt_codec_qos_pref *pref)
{
	printk("Audio Stream %p configured\n", stream);

	k_sem_give(&sem_stream_configured);
}

static void stream_qos_set(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p QoS set\n", stream);

	k_sem_give(&sem_stream_qos);
}

static void stream_enabled(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p enabled\n", stream);

	k_sem_give(&sem_stream_enabled);
}

static void stream_started(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p started\n", stream);

	k_sem_give(&sem_stream_started);
}

static void stream_metadata_updated(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p metadata updated\n", stream);
}

static void stream_disabled(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p disabled\n", stream);
}

static void stream_stopped(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p stopped\n", stream);

	/* Stop send timer */
	k_work_cancel_delayable(&audio_send_work);
}

static void stream_released(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p released\n", stream);
}

static void stream_recv(struct bt_audio_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	printk("Incoming audio on stream %p len %u\n", stream, buf->len);
}

static struct bt_audio_stream_ops stream_ops = {
	.configured = stream_configured,
	.qos_set = stream_qos_set,
	.enabled = stream_enabled,
	.started = stream_started,
	.metadata_updated = stream_metadata_updated,
	.disabled = stream_disabled,
	.stopped = stream_stopped,
	.released = stream_released,
	.recv = stream_recv
};

static void add_remote_source(struct bt_audio_ep *ep, uint8_t index)
{
	printk("Sink #%u: ep %p\n", index, ep);

	if (index > ARRAY_SIZE(sources)) {
		printk("Could not add source ep[%u]\n", index);
		return;
	}

	sources[index] = ep;
}

static void add_remote_sink(struct bt_audio_ep *ep, uint8_t index)
{
	printk("Sink #%u: ep %p\n", index, ep);

	if (index > ARRAY_SIZE(sinks)) {
		printk("Could not add sink ep[%u]\n", index);
		return;
	}

	sinks[index] = ep;
}

static void add_remote_codec(struct bt_codec *codec_capabilities, int index,
			     enum bt_audio_dir dir)
{
	printk("#%u: codec_capabilities %p dir 0x%02x\n",
	       index, codec_capabilities, dir);

	print_codec_capabilities(codec_capabilities);

	if (dir != BT_AUDIO_DIR_SINK && dir != BT_AUDIO_DIR_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT) {
		remote_codec_capabilities[index] = codec_capabilities;
	}
}

static void discover_sinks_cb(struct bt_conn *conn,
			      struct bt_codec *codec,
			      struct bt_audio_ep *ep,
			      struct bt_audio_discover_params *params)
{
	if (params->err != 0) {
		printk("Discovery failed: %d\n", params->err);
		return;
	}

	if (codec != NULL) {
		add_remote_codec(codec, params->num_caps, params->dir);
		return;
	}

	if (ep != NULL) {
		add_remote_sink(ep, params->num_eps);

		return;
	}

	printk("Discover sinks complete: err %d\n", params->err);

	(void)memset(params, 0, sizeof(*params));

	k_sem_give(&sem_sinks_discovered);
}

static void discover_sources_cb(struct bt_conn *conn,
				struct bt_codec *codec,
				struct bt_audio_ep *ep,
				struct bt_audio_discover_params *params)
{
	if (params->err != 0) {
		printk("Discovery failed: %d\n", params->err);
		return;
	}

	if (codec != NULL) {
		add_remote_codec(codec, params->num_caps, params->dir);
		return;
	}

	if (ep != NULL) {
		add_remote_source(ep, params->num_eps);

		return;
	}

	printk("Discover sources complete: err %d\n", params->err);

	(void)memset(params, 0, sizeof(*params));

	k_sem_give(&sem_sources_discovered);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);
	k_sem_give(&sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged: %u/%u\n", tx, rx);
	k_sem_give(&sem_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i].ops = &stream_ops;
	}

	bt_gatt_cb_register(&gatt_callbacks);

#if defined(CONFIG_LIBLC3CODEC)
	k_work_init_delayable(&audio_send_work, lc3_audio_timer_timeout);
#else
	k_work_init_delayable(&audio_send_work, audio_timer_timeout);
#endif

	return 0;
}

static int scan_and_connect(void)
{
	int err;

	start_scan();

	err = k_sem_take(&sem_connected, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_connected (err %d)\n", err);
		return err;
	}

	err = k_sem_take(&sem_mtu_exchanged, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_mtu_exchanged (err %d)\n", err);
		return err;
	}

	return 0;
}

static int discover_sinks(void)
{
	static struct bt_audio_discover_params params;
	int err;

	params.func = discover_sinks_cb;
	params.dir = BT_AUDIO_DIR_SINK;

	err = bt_audio_discover(default_conn, &params);
	if (err != 0) {
		printk("Failed to discover sinks: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_sinks_discovered, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_sinks_discovered (err %d)\n", err);
		return err;
	}

	return 0;
}

static int discover_sources(void)
{
	static struct bt_audio_discover_params params;
	int err;

	params.func = discover_sources_cb;
	params.dir = BT_AUDIO_DIR_SOURCE;

	err = bt_audio_discover(default_conn, &params);
	if (err != 0) {
		printk("Failed to discover sources: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_sources_discovered, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_sources_discovered (err %d)\n", err);
		return err;
	}

	return 0;
}

static int configure_stream(struct bt_audio_stream *stream,
			    struct bt_audio_ep *ep)
{
	int err;

	err = bt_audio_stream_config(default_conn, stream, ep,
				     &codec_configuration.codec);
	if (err != 0) {
		return err;
	}

	err = k_sem_take(&sem_stream_configured, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_stream_configured (err %d)\n", err);
		return err;
	}

	return 0;
}

static int configure_streams(void)
{
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(sinks); i++) {
		struct bt_audio_ep *ep = sinks[i];
		struct bt_audio_stream *stream = &streams[i];

		if (ep == NULL) {
			continue;
		}

		err = configure_stream(stream, ep);
		if (err != 0) {
			printk("Could not configure sink stream[%zu]: %d\n",
			       i, err);
			return err;
		}

		printk("Configured sink stream[%zu]\n", i);
		configured_stream_count++;
		configured_sink_stream_count++;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sources); i++) {
		struct bt_audio_ep *ep = sources[i];
		struct bt_audio_stream *stream = &streams[i + configured_sink_stream_count];

		if (ep == NULL) {
			continue;
		}

		err = configure_stream(stream, ep);
		if (err != 0) {
			printk("Could not configure source stream[%zu]: %d\n",
			       i, err);
			return err;
		}

		printk("Configured source stream[%zu]\n", i);
		configured_stream_count++;
	}

	return 0;
}

static int create_group(void)
{
	struct bt_audio_stream *streams_p[ARRAY_SIZE(streams)];
	int err;

	for (size_t i = 0U; i < configured_stream_count; i++) {
		streams_p[i] = &streams[i];
	}

	err = bt_audio_unicast_group_create(streams_p, configured_stream_count,
					    &unicast_group);
	if (err != 0) {
		printk("Could not create unicast group (err %d)\n", err);
		return err;
	}

	return 0;
}

static int set_stream_qos(void)
{
	int err;

	err = bt_audio_stream_qos(default_conn, unicast_group,
				&codec_configuration.qos);
	if (err != 0) {
		printk("Unable to setup QoS: %d", err);
		return err;
	}

	err = k_sem_take(&sem_stream_qos, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_stream_qos (err %d)\n", err);
		return err;
	}

	return 0;
}

static int enable_streams(void)
{
	if (IS_ENABLED(CONFIG_LIBLC3CODEC)) {
		init_lc3();
	}

	for (size_t i = 0; i < configured_stream_count; i++) {
		int err;

		err = bt_audio_stream_enable(&streams[i],
					     codec_configuration.codec.meta,
					     codec_configuration.codec.meta_count);
		if (err != 0) {
			printk("Unable to enable stream: %d\n", err);
			return err;
		}

		err = k_sem_take(&sem_stream_enabled, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_stream_enabled (err %d)\n", err);
			return err;
		}
	}

	return 0;
}

static int start_streams(void)
{
	for (size_t i = 0; i < configured_stream_count; i++) {
		int err;

		err = bt_audio_stream_start(&streams[i]);
		if (err != 0) {
			printk("Unable to start stream: %d\n", err);
			return err;
		}

		err = k_sem_take(&sem_stream_started, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_stream_started (err %d)\n", err);
			return err;
		}
	}

	return 0;
}

void main(void)
{
	int err;

	printk("Initializing\n");
	err = init();
	if (err != 0) {
		return;
	}
	printk("Initialized\n");

	printk("Waiting for connection\n");
	err = scan_and_connect();
	if (err != 0) {
		return;
	}
	printk("Connected\n");

	printk("Discovering sinks\n");
	err = discover_sinks();
	if (err != 0) {
		return;
	}
	printk("Sinks discovered\n");

	printk("Discovering sources\n");
	err = discover_sources();
	if (err != 0) {
		return;
	}
	printk("Sources discovered\n");

	printk("Configuring streams\n");
	err = configure_streams();
	if (err != 0) {
		return;
	}
	printk("Stream configured\n");

	printk("Creating unicast group\n");
	err = create_group();
	if (err != 0) {
		return;
	}
	printk("Unicast group created\n");

	printk("Setting stream QoS\n");
	err = set_stream_qos();
	if (err != 0) {
		return;
	}
	printk("Stream QoS Set\n");

	printk("Enabling streams\n");
	err = enable_streams();
	if (err != 0) {
		return;
	}
	printk("Streams enabled\n");

	printk("Starting streams\n");
	err = start_streams();
	if (err != 0) {
		return;
	}
	printk("Streams started\n");

	/* Start send timer */
	k_work_schedule(&audio_send_work, K_MSEC(0));
}
