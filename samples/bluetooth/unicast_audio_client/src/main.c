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
#include <sys/byteorder.h>

static void start_scan(void);

static struct bt_conn *default_conn;
static struct k_work_delayable audio_send_work;
static struct bt_audio_stream audio_stream;
static struct bt_audio_unicast_group *unicast_group;
static struct bt_codec *remote_codecs[CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];
static struct bt_audio_ep *sinks[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1,
			  CONFIG_BT_ISO_TX_MTU + BT_ISO_CHAN_SEND_RESERVE,
			  8, NULL);

/* Mandatory support preset by both client and server */
static struct bt_audio_lc3_preset preset_16_2_1 = BT_AUDIO_LC3_UNICAST_PRESET_16_2_1;

static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_sink_discovered, 0, 1);
static K_SEM_DEFINE(sem_stream_configured, 0, 1);
static K_SEM_DEFINE(sem_stream_qos, 0, 1);
static K_SEM_DEFINE(sem_stream_enabled, 0, 1);
static K_SEM_DEFINE(sem_stream_started, 0, 1);


#if defined(CONFIG_LIBLC3CODEC)

#include "lc3.h"
#include "math.h"

/* Current sample do not use codec configuration parameters, hence below shall match the selected
 * codec configuration.
 * One sample data buffer en generated and repeated.
 */
#define AUDIO_SAMPLE_RATE_HZ    16000
#define AUDIO_FREQUENCY_HZ      400
#define AUDIO_LENGTH_US         10000 /* amount of sample data - shall match LC3 frame length */
#define AUDIO_LENGTH_100US      (AUDIO_LENGTH_US / 100)
#define NUM_SAMPLES             ((AUDIO_LENGTH_US * AUDIO_SAMPLE_RATE_HZ) / USEC_PER_SEC)
#define AUDIO_VOLUME            (INT16_MAX - 3000) /* codec does clipping above INT16_MAX - 3000 */

static int16_t audio_buf[NUM_SAMPLES];
static lc3_encoder_t lc3_encoder;
static lc3_encoder_mem_48k_t lc3_encoder_mem;


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

	for (int i = 0; i < num_samples; i++) {
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
	static int32_t pdu_cnt;
	int32_t pdu_goal_cnt;
	int64_t uptime, run_time_ms, run_time_100us;

	k_work_schedule(&audio_send_work, K_USEC(preset_16_2_1.qos.interval));

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
	pdu_goal_cnt = run_time_100us / AUDIO_LENGTH_100US;

	/* Add primer value to ensure the controller do not run low on data due to jitter */
	pdu_goal_cnt += prime_count;

	printk("LC3 encode %d frames\n", pdu_goal_cnt - pdu_cnt);

	while (pdu_cnt < pdu_goal_cnt) {

		int ret;
		struct net_buf *buf;
		uint8_t *net_buffer;
		uint16_t lc3_ret;
		const uint16_t tx_sdu_len = audio_stream.iso->qos->tx->sdu;

		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

		net_buffer = net_buf_tail(buf);
		buf->len += tx_sdu_len;

		lc3_ret = lc3_encode(lc3_encoder, LC3_PCM_FORMAT_S16, audio_buf,
				     1, tx_sdu_len, net_buffer);

		if (lc3_ret == -1) {
			printk("LC3 encoder failed - wrong parameters?: %d", lc3_ret);
		}

		ret = bt_audio_stream_send(&audio_stream, buf);

		if (ret < 0) {
			printk("  Failed to send LC3 audio data (%d)\n", ret);
			net_buf_unref(buf);
		} else {
			printk("  TX LC3 l: %zu\n", tx_sdu_len);
			pdu_cnt++;
		}
	}
}

static void init_lc3(void)
{
	/* Fill audio buffer with Sine wave */
	fill_audio_buf_sin(audio_buf, AUDIO_LENGTH_US, AUDIO_FREQUENCY_HZ,
			   AUDIO_SAMPLE_RATE_HZ);

	for (int i = 0; i < NUM_SAMPLES; i++) {
		printk("%3i: %6i\n", i, audio_buf[i]);
	}

	/* Create the encoder instance. This shall complete before stream_started() is called. */
	lc3_encoder = lc3_setup_encoder(AUDIO_LENGTH_US, AUDIO_SAMPLE_RATE_HZ, 0, &lc3_encoder_mem);

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

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, buf_data, len_to_send);

	ret = bt_audio_stream_send(&audio_stream, buf);
	if (ret < 0) {
		printk("Failed to send audio data (%d)\n", ret);
		net_buf_unref(buf);
	} else {
		printk("Sending mock data with len %zu\n", len_to_send);
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

	/* Start send timer */
	k_work_schedule(&audio_send_work, K_MSEC(0));
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

static struct bt_audio_stream_ops stream_ops = {
	.configured = stream_configured,
	.qos_set = stream_qos_set,
	.enabled = stream_enabled,
	.started = stream_started,
	.metadata_updated = stream_metadata_updated,
	.disabled = stream_disabled,
	.stopped = stream_stopped,
	.released = stream_released,
};

static void add_remote_sink(struct bt_audio_ep *ep, uint8_t index)
{
	printk("Sink #%u: ep %p\n", index, ep);

	sinks[index] = ep;
}

static void add_remote_codec(struct bt_codec *codec, int index,
				  uint8_t type)
{
	printk("#%u: codec %p type 0x%02x\n", index, codec, type);

	print_codec(codec);

	if (type != BT_AUDIO_SINK && type != BT_AUDIO_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT) {
		remote_codecs[index] = codec;
	}
}

static void discover_sink_cb(struct bt_conn *conn,
			     struct bt_codec *codec,
			     struct bt_audio_ep *ep,
			     struct bt_audio_discover_params *params)
{
	if (params->err != 0) {
		printk("Discovery failed: %d\n", params->err);
		return;
	}

	if (codec != NULL) {
		add_remote_codec(codec, params->num_caps, params->type);
		return;
	}

	if (ep != NULL) {
		if (params->type == BT_AUDIO_SINK) {
			add_remote_sink(ep, params->num_eps);
		} else {
			printk("Invalid param type: %u\n", params->type);
		}

		return;
	}

	printk("Discover complete: err %d\n", params->err);

	(void)memset(params, 0, sizeof(*params));

	k_sem_give(&sem_sink_discovered);
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

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	audio_stream.ops = &stream_ops;

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

	return 0;
}

static int discover_sink(void)
{
	static struct bt_audio_discover_params params;
	int err;

	params.func = discover_sink_cb;
	params.type = BT_AUDIO_SINK;

	err = bt_audio_discover(default_conn, &params);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_sink_discovered, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_sink_discovered (err %d)\n", err);
		return err;
	}

	return 0;
}

static int configure_stream(struct bt_audio_stream *stream)
{
	int err;

	err = bt_audio_stream_config(default_conn, stream, sinks[0],
				     &preset_16_2_1.codec);
	if (err != 0) {
		printk("Could not configure stream\n");
		return err;
	}

	err = k_sem_take(&sem_stream_configured, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_stream_configured (err %d)\n", err);
		return err;
	}

	return 0;
}

static int create_group(struct bt_audio_stream *stream)
{
	int err;

	err = bt_audio_unicast_group_create(stream, 1, &unicast_group);
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
				&preset_16_2_1.qos);
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

static int enable_stream(struct bt_audio_stream *stream)
{
	int err;

	if (IS_ENABLED(CONFIG_LIBLC3CODEC)) {
		init_lc3();
	}

	err = bt_audio_stream_enable(stream, preset_16_2_1.codec.meta,
				     preset_16_2_1.codec.meta_count);
	if (err != 0) {
		printk("Unable to enable stream: %d", err);
		return err;
	}

	err = k_sem_take(&sem_stream_enabled, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_stream_enabled (err %d)\n", err);
		return err;
	}

	return 0;
}

static int start_stream(struct bt_audio_stream *stream)
{
	int err;

	err = bt_audio_stream_start(stream);
	if (err != 0) {
		printk("Unable to start stream: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_stream_started, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_stream_started (err %d)\n", err);
		return err;
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

	printk("Discovering sink\n");
	err = discover_sink();
	if (err != 0) {
		return;
	}
	printk("Sink discovered\n");

	printk("Configuring stream\n");
	err = configure_stream(&audio_stream);
	if (err != 0) {
		return;
	}
	printk("Stream configured\n");

	printk("Creating unicast group\n");
	err = create_group(&audio_stream);
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

	printk("Enabling stream\n");
	err = enable_stream(&audio_stream);
	if (err != 0) {
		return;
	}
	printk("Stream enabled\n");

	printk("Starting stream\n");
	err = start_stream(&audio_stream);
	if (err != 0) {
		return;
	}
	printk("Stream started\n");
}
