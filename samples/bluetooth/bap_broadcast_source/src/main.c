/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

BUILD_ASSERT(strlen(CONFIG_BROADCAST_CODE) <= BT_ISO_BROADCAST_CODE_SIZE, "Invalid broadcast code");

/* Zephyr Controller works best while Extended Advertising interval to be a multiple
 * of the ISO Interval minus 10 ms (max. advertising random delay). This is
 * required to place the AUX_ADV_IND PDUs in a non-overlapping interval with the
 * Broadcast ISO radio events.
 *
 * I.e. for a 7.5 ms ISO interval use 90 ms minus 10 ms ==> 80 ms advertising
 * interval.
 * And, for 10 ms ISO interval, can use 90 ms minus 10 ms ==> 80 ms advertising
 * interval.
 */
#define BT_LE_EXT_ADV_CUSTOM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV, 0x0080, 0x0080, NULL)

/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 3U
#define TOTAL_BUF_NEEDED        (BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT");

#if defined(CONFIG_BAP_BROADCAST_16_2_1)

static struct bt_bap_lc3_preset preset_active = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

#define BROADCAST_SAMPLE_RATE 16000

#elif defined(CONFIG_BAP_BROADCAST_24_2_1)

static struct bt_bap_lc3_preset preset_active = BT_BAP_LC3_BROADCAST_PRESET_24_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

#define BROADCAST_SAMPLE_RATE 24000

#endif

#if defined(CONFIG_BAP_BROADCAST_16_2_1)
#define MAX_SAMPLE_RATE 16000
#elif defined(CONFIG_BAP_BROADCAST_24_2_1)
#define MAX_SAMPLE_RATE 24000
#endif
#define MAX_FRAME_DURATION_US 10000
#define MAX_NUM_SAMPLES       ((MAX_FRAME_DURATION_US * MAX_SAMPLE_RATE) / USEC_PER_SEC)

#if defined(CONFIG_LIBLC3)
#include "lc3.h"

#if defined(CONFIG_USB_DEVICE_AUDIO)
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>
#include <zephyr/sys/ring_buffer.h>

/* USB Audio Data is downsampled from 48kHz to match broadcast preset when receiving data */
#define USB_SAMPLE_RATE       48000
#define USB_DOWNSAMPLE_RATE   BROADCAST_SAMPLE_RATE
#define USB_FRAME_DURATION_US 1000
#define USB_NUM_SAMPLES       ((USB_FRAME_DURATION_US * USB_DOWNSAMPLE_RATE) / USEC_PER_SEC)
#define USB_BYTES_PER_SAMPLE  2
#define USB_CHANNELS          2

#define RING_BUF_USB_FRAMES  20
#define AUDIO_RING_BUF_BYTES (USB_NUM_SAMPLES * USB_BYTES_PER_SAMPLE * RING_BUF_USB_FRAMES)
#else /* !defined(CONFIG_USB_DEVICE_AUDIO) */

#include <math.h>

#define AUDIO_VOLUME            (INT16_MAX - 3000) /* codec does clipping above INT16_MAX - 3000 */
#define AUDIO_TONE_FREQUENCY_HZ 400

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
	const float step = 2 * 3.1415f / sine_period_samples;

	for (unsigned int i = 0; i < num_samples; i++) {
		const float sample = sinf(i * step);

		buf[i] = (int16_t)(AUDIO_VOLUME * sample);
	}
}
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */
#endif /* defined(CONFIG_LIBLC3) */

static struct broadcast_source_stream {
	struct bt_bap_stream stream;
	uint16_t seq_num;
	size_t sent_cnt;
#if defined(CONFIG_LIBLC3)
	lc3_encoder_t lc3_encoder;
#if defined(CONFIG_BAP_BROADCAST_16_2_1)
	lc3_encoder_mem_16k_t lc3_encoder_mem;
#elif defined(CONFIG_BAP_BROADCAST_24_2_1)
	lc3_encoder_mem_48k_t lc3_encoder_mem;
#endif
#if defined(CONFIG_USB_DEVICE_AUDIO)
	struct ring_buf audio_ring_buf;
	uint8_t _ring_buffer_memory[AUDIO_RING_BUF_BYTES];
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */
#endif /* defined(CONFIG_LIBLC3) */
} streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static struct bt_bap_broadcast_source *broadcast_source;

NET_BUF_POOL_FIXED_DEFINE(tx_pool, TOTAL_BUF_NEEDED, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static int16_t send_pcm_data[MAX_NUM_SAMPLES];
static uint16_t seq_num;
static bool stopping;

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

#define BROADCAST_SOURCE_LIFETIME 120U /* seconds */

#if defined(CONFIG_LIBLC3)
static int freq_hz;
static int frame_duration_us;
static int frames_per_sdu;
static int octets_per_frame;

static K_SEM_DEFINE(lc3_encoder_sem, 0U, TOTAL_BUF_NEEDED);
#endif

static void send_data(struct broadcast_source_stream *source_stream)
{
	struct bt_bap_stream *stream = &source_stream->stream;
	struct net_buf *buf;
	int ret;

	if (stopping) {
		return;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n", stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
#if defined(CONFIG_LIBLC3)
	uint8_t lc3_encoded_buffer[preset_active.qos.sdu];

	if (source_stream->lc3_encoder == NULL) {
		printk("LC3 encoder not setup, cannot encode data.\n");
		net_buf_unref(buf);
		return;
	}

#if defined(CONFIG_USB_DEVICE_AUDIO)
	uint32_t size = ring_buf_get(&source_stream->audio_ring_buf, (uint8_t *)send_pcm_data,
				     sizeof(send_pcm_data));

	if (size < sizeof(send_pcm_data)) {
		const size_t padding_size = sizeof(send_pcm_data) - size;

		printk("Not enough bytes ready, padding %d!\n", padding_size);
		memset(&((uint8_t *)send_pcm_data)[size], 0, padding_size);
	}
#endif

	ret = lc3_encode(source_stream->lc3_encoder, LC3_PCM_FORMAT_S16, send_pcm_data, 1,
			 octets_per_frame, lc3_encoded_buffer);
	if (ret == -1) {
		printk("LC3 encoder failed - wrong parameters?: %d", ret);
		net_buf_unref(buf);
		return;
	}

	net_buf_add_mem(buf, lc3_encoded_buffer, preset_active.qos.sdu);
#else
	net_buf_add_mem(buf, send_pcm_data, preset_active.qos.sdu);
#endif /* defined(CONFIG_LIBLC3) */

	ret = bt_bap_stream_send(stream, buf, source_stream->seq_num++);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		return;
	}

	source_stream->sent_cnt++;
	if ((source_stream->sent_cnt % 1000U) == 0U) {
		printk("Stream %p: Sent %u total ISO packets\n", stream, source_stream->sent_cnt);
	}
}

#if defined(CONFIG_LIBLC3)
static void init_lc3_thread(void *arg1, void *arg2, void *arg3)
{
	const struct bt_audio_codec_cfg *codec_cfg = &preset_active.codec_cfg;
	int ret;

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	if (ret > 0) {
		freq_hz = bt_audio_codec_cfg_freq_to_freq_hz(ret);
	} else {
		return;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
	if (ret > 0) {
		frame_duration_us = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
	} else {
		printk("Error: Frame duration not set, cannot start codec.");
		return;
	}

	octets_per_frame = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	frames_per_sdu = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);

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

#if !defined(CONFIG_USB_DEVICE_AUDIO)
	/* If USB is not used as a sound source, generate a sine wave */
	fill_audio_buf_sin(send_pcm_data, frame_duration_us, AUDIO_TONE_FREQUENCY_HZ, freq_hz);
#endif

	/* Create the encoder instance. This shall complete before stream_started() is called. */
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		printk("Initializing lc3 encoder for stream %zu\n", i);
		streams[i].lc3_encoder = lc3_setup_encoder(frame_duration_us, freq_hz, 0,
							   &streams[i].lc3_encoder_mem);

		if (streams[i].lc3_encoder == NULL) {
			printk("ERROR: Failed to setup LC3 encoder - wrong parameters?\n");
		}
	}

	while (true) {
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			k_sem_take(&lc3_encoder_sem, K_FOREVER);
		}
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			send_data(&streams[i]);
		}
	}
}

#define LC3_ENCODER_STACK_SIZE 4 * 4096
#define LC3_ENCODER_PRIORITY   5

K_THREAD_DEFINE(encoder, LC3_ENCODER_STACK_SIZE, init_lc3_thread, NULL, NULL, NULL,
		LC3_ENCODER_PRIORITY, 0, -1);

#if defined(CONFIG_USB_DEVICE_AUDIO)
static void data_received(const struct device *dev, struct net_buf *buffer, size_t size)
{
	static int count;
	int16_t *pcm;
	int nsamples, ratio;
	int16_t usb_pcm_data[USB_CHANNELS][USB_NUM_SAMPLES];

	if (!buffer) {
		return;
	}

	if (!size) {
		net_buf_unref(buffer);
		return;
	}

	pcm = (int16_t *)net_buf_pull_mem(buffer, size);

	/* 'size' is in bytes, containing 1ms, 48kHz, stereo, 2 bytes per sample.
	 * Take left channel and do a simple downsample to 16kHz/24Khz
	 * matching the broadcast preset.
	 */

	ratio = USB_SAMPLE_RATE / USB_DOWNSAMPLE_RATE;
	nsamples = size / (sizeof(int16_t) * USB_CHANNELS * ratio);
	for (size_t i = 0, j = 0; i < nsamples; i++, j += USB_CHANNELS * ratio) {
		usb_pcm_data[0][i] = pcm[j];
		usb_pcm_data[1][i] = pcm[j + 1];
	}

	for (size_t i = 0U; i < MIN(ARRAY_SIZE(streams), 2); i++) {
		const uint32_t size_put =
			ring_buf_put(&(streams[i].audio_ring_buf), (uint8_t *)(usb_pcm_data[i]),
				     nsamples * USB_BYTES_PER_SAMPLE);
		if (size_put < nsamples * USB_BYTES_PER_SAMPLE) {
			printk("Not enough room for samples in %s buffer: %u < %u, total capacity: "
			       "%u\n",
			       i == 0 ? "left" : "right", size_put, nsamples * USB_BYTES_PER_SAMPLE,
			       ring_buf_capacity_get(&(streams[i].audio_ring_buf)));
		}
	}

	count++;
	if ((count % 1000) == 0) {
		printk("USB Data received (count = %d)\n", count);
	}

	net_buf_unref(buffer);
}

static const struct usb_audio_ops ops = {.data_received_cb = data_received};
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */
#endif /* defined(CONFIG_LIBLC3) */

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct broadcast_source_stream *source_stream =
		CONTAINER_OF(stream, struct broadcast_source_stream, stream);

	source_stream->seq_num = 0U;
	source_stream->sent_cnt = 0U;
	k_sem_give(&sem_started);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	k_sem_give(&sem_stopped);
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
#if defined(CONFIG_LIBLC3)
	k_sem_give(&lc3_encoder_sem);
#else
	/* If no LC3 encoder is used, just send mock data directly */
	struct broadcast_source_stream *source_stream =
		CONTAINER_OF(stream, struct broadcast_source_stream, stream);

	send_data(source_stream);
#endif
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb, .stopped = stream_stopped_cb, .sent = stream_sent_cb};

static int setup_broadcast_source(struct bt_bap_broadcast_source **source)
{
	struct bt_bap_broadcast_source_stream_param
		stream_params[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_param[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_param create_param = {0};
	const size_t streams_per_subgroup = ARRAY_SIZE(stream_params) / ARRAY_SIZE(subgroup_param);
	uint8_t left[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
					      BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_LEFT))};
	uint8_t right[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
					       BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_RIGHT))};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_param); i++) {
		subgroup_param[i].params_count = streams_per_subgroup;
		subgroup_param[i].params = stream_params + i * streams_per_subgroup;
		subgroup_param[i].codec_cfg = &preset_active.codec_cfg;
	}

	for (size_t j = 0U; j < ARRAY_SIZE(stream_params); j++) {
		stream_params[j].stream = &streams[j].stream;
		stream_params[j].data = j == 0 ? left : right;
		stream_params[j].data_len = j == 0 ? sizeof(left) : sizeof(right);
		bt_bap_stream_cb_register(stream_params[j].stream, &stream_ops);
	}

	create_param.params_count = ARRAY_SIZE(subgroup_param);
	create_param.params = subgroup_param;
	create_param.qos = &preset_active.qos;
	create_param.encryption = strlen(CONFIG_BROADCAST_CODE) > 0;
	create_param.packing = (IS_ENABLED(CONFIG_BT_ISO_PACKING_INTERLEAVED) ?
				BT_ISO_PACKING_INTERLEAVED :
				BT_ISO_PACKING_SEQUENTIAL);

	if (create_param.encryption) {
		memcpy(create_param.broadcast_code, CONFIG_BROADCAST_CODE,
		       strlen(CONFIG_BROADCAST_CODE));
	}

	printk("Creating broadcast source with %zu subgroups with %zu streams\n",
	       ARRAY_SIZE(subgroup_param), ARRAY_SIZE(subgroup_param) * streams_per_subgroup);

	err = bt_bap_broadcast_source_create(&create_param, source);
	if (err != 0) {
		printk("Unable to create broadcast source: %d\n", err);
		return err;
	}

	return 0;
}

int main(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
	printk("Bluetooth initialized\n");

	for (size_t i = 0U; i < ARRAY_SIZE(send_pcm_data); i++) {
		/* Initialize mock data */
		send_pcm_data[i] = i;
	}

#if defined(CONFIG_LIBLC3)
#if defined(CONFIG_USB_DEVICE_AUDIO)
	const struct device *hs_dev;

	hs_dev = DEVICE_DT_GET(DT_NODELABEL(hs_0));

	if (!device_is_ready(hs_dev)) {
		printk("Device USB Headset is not ready\n");
		return 0;
	}

	printk("Found USB Headset Device\n");

	(void)memset(streams, 0, sizeof(streams));

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		ring_buf_init(&(streams[i].audio_ring_buf), sizeof(streams[i]._ring_buffer_memory),
			      streams[i]._ring_buffer_memory);
		printk("Initialized ring buf %zu: capacity: %u\n", i,
		       ring_buf_capacity_get(&(streams[i].audio_ring_buf)));
	}

	usb_audio_register(hs_dev, &ops);

	err = usb_enable(NULL);
	if (err && err != -EALREADY) {
		printk("Failed to enable USB (%d)", err);
		return 0;
	}

#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */
	k_thread_start(encoder);
#endif /* defined(CONFIG_LIBLC3) */

	while (true) {
		/* Broadcast Audio Streaming Endpoint advertising data */
		NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
		NET_BUF_SIMPLE_DEFINE(base_buf, 128);
		struct bt_data ext_ad[2];
		struct bt_data per_ad;
		uint32_t broadcast_id;

		/* Create a connectable advertising set */
		err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CUSTOM, NULL, &adv);
		if (err != 0) {
			printk("Unable to create extended advertising set: %d\n", err);
			return 0;
		}

		/* Set periodic advertising parameters */
		err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
		if (err) {
			printk("Failed to set periodic advertising parameters (err %d)\n", err);
			return 0;
		}

		printk("Creating broadcast source\n");
		err = setup_broadcast_source(&broadcast_source);
		if (err != 0) {
			printk("Unable to setup broadcast source: %d\n", err);
			return 0;
		}

#if defined(CONFIG_STATIC_BROADCAST_ID)
		broadcast_id = CONFIG_BROADCAST_ID;
#else
		err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
		if (err) {
			printk("Unable to generate broadcast ID: %d\n", err);
			return err;
		}
#endif /* CONFIG_STATIC_BROADCAST_ID */

		/* Setup extended advertising data */
		net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
		net_buf_simple_add_le24(&ad_buf, broadcast_id);
		ext_ad[0].type = BT_DATA_SVC_DATA16;
		ext_ad[0].data_len = ad_buf.len;
		ext_ad[0].data = ad_buf.data;
		ext_ad[1] = (struct bt_data)BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
						    sizeof(CONFIG_BT_DEVICE_NAME) - 1);
		err = bt_le_ext_adv_set_data(adv, ext_ad, 2, NULL, 0);
		if (err != 0) {
			printk("Failed to set extended advertising data: %d\n", err);
			return 0;
		}

		/* Setup periodic advertising data */
		err = bt_bap_broadcast_source_get_base(broadcast_source, &base_buf);
		if (err != 0) {
			printk("Failed to get encoded BASE: %d\n", err);
			return 0;
		}

		per_ad.type = BT_DATA_SVC_DATA16;
		per_ad.data_len = base_buf.len;
		per_ad.data = base_buf.data;
		err = bt_le_per_adv_set_data(adv, &per_ad, 1);
		if (err != 0) {
			printk("Failed to set periodic advertising data: %d\n", err);
			return 0;
		}

		/* Start extended advertising */
		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start extended advertising: %d\n", err);
			return 0;
		}

		/* Enable Periodic Advertising */
		err = bt_le_per_adv_start(adv);
		if (err) {
			printk("Failed to enable periodic advertising: %d\n", err);
			return 0;
		}

		printk("Starting broadcast source\n");
		stopping = false;
		err = bt_bap_broadcast_source_start(broadcast_source, adv);
		if (err != 0) {
			printk("Unable to start broadcast source: %d\n", err);
			return 0;
		}

		/* Wait for all to be started */
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			k_sem_take(&sem_started, K_FOREVER);
		}
		printk("Broadcast source started\n");

		/* Initialize sending */
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
				stream_sent_cb(&streams[i].stream);
			}
		}

#if defined(CONFIG_LIBLC3) && defined(CONFIG_USB_DEVICE_AUDIO)
		/* Never stop streaming when using USB Audio as input */
		k_sleep(K_FOREVER);
#endif /* defined(CONFIG_LIBLC3) && defined(CONFIG_USB_DEVICE_AUDIO) */
		printk("Waiting %u seconds before stopped\n", BROADCAST_SOURCE_LIFETIME);
		k_sleep(K_SECONDS(BROADCAST_SOURCE_LIFETIME));
		printk("Stopping broadcast source\n");
		stopping = true;
		err = bt_bap_broadcast_source_stop(broadcast_source);
		if (err != 0) {
			printk("Unable to stop broadcast source: %d\n", err);
			return 0;
		}

		/* Wait for all to be stopped */
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			k_sem_take(&sem_stopped, K_FOREVER);
		}
		printk("Broadcast source stopped\n");

		printk("Deleting broadcast source\n");
		err = bt_bap_broadcast_source_delete(broadcast_source);
		if (err != 0) {
			printk("Unable to delete broadcast source: %d\n", err);
			return 0;
		}
		printk("Broadcast source deleted\n");
		broadcast_source = NULL;
		seq_num = 0;

		err = bt_le_per_adv_stop(adv);
		if (err) {
			printk("Failed to stop periodic advertising (err %d)\n", err);
			return 0;
		}

		err = bt_le_ext_adv_stop(adv);
		if (err) {
			printk("Failed to stop extended advertising (err %d)\n", err);
			return 0;
		}

		err = bt_le_ext_adv_delete(adv);
		if (err) {
			printk("Failed to delete extended advertising (err %d)\n", err);
			return 0;
		}
	}
	return 0;
}
