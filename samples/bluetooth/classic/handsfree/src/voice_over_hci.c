/* voice_over_hci.c - voice implementation for Bluetooth Hands-Free Profile */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sco.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#if defined(CONFIG_LIBSBC)
#include <zephyr/bluetooth/sbc.h>
#endif /* CONFIG_LIBSBC */

#include "voice.h"

static struct bt_sco_stream voice_stream;
static uint8_t selected_codec_id;
static voice_rx_cb_t rx_cb;
static uint8_t sco_mtu;
static uint32_t pcm_len;
static uint32_t pcm_samples;
static uint32_t sco_trans_interval; /* Unit is us */
static atomic_t flags;

#define VOICE_OVER_HCI_STARTED 1

static K_KERNEL_STACK_MEMBER(voice_tx_thread_stack, CONFIG_VOICE_TX_THREAD_STACK_SIZE);
static K_KERNEL_STACK_MEMBER(voice_rx_thread_stack, CONFIG_VOICE_RX_THREAD_STACK_SIZE);

#define MAX_SAMPLE_FREQ      16000
#define MAX_SAMPLE_BIT_WIDTH 16
#define MAX_CHANNELS         1

#define SAMPLES_PER_BLOCK ((MAX_SAMPLE_FREQ * CONFIG_AUDIO_TRANSFER_INTERVAL / 1000) * MAX_CHANNELS)
#define BLOCK_SIZE     (MAX_SAMPLE_BIT_WIDTH * SAMPLES_PER_BLOCK / 8)

RING_BUF_DECLARE(sco_rx_ringbuf, (CONFIG_BT_SCO_RX_BUF_COUNT + 1) * BT_SCO_SDU_SIZE(UINT8_MAX));

RING_BUF_DECLARE(raw_rx_ringbuf, (CONFIG_BT_SCO_RX_BUF_COUNT + 1) * BLOCK_SIZE);
RING_BUF_DECLARE(raw_tx_ringbuf, (CONFIG_VOICE_BUFFERS + 1) * BLOCK_SIZE);

static K_SEM_DEFINE(voice_tx_sem, 0, (CONFIG_VOICE_BUFFERS + 1) * BLOCK_SIZE);
static K_SEM_DEFINE(voice_rx_sem, 0, (CONFIG_BT_SCO_RX_BUF_COUNT + 1) * BLOCK_SIZE);

static K_MUTEX_DEFINE(voice_tx_mutex);
static K_MUTEX_DEFINE(voice_rx_mutex);

static void tx_pool_destroy(struct net_buf *buf)
{
	k_sem_give(&voice_tx_sem);
	net_buf_destroy(buf);
}

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_VOICE_BUFFERS + 1, BT_SCO_SDU_SIZE(UINT8_MAX),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, tx_pool_destroy);

#if defined(CONFIG_LIBSBC)
static struct sbc_encoder encoder;
static struct sbc_decoder decoder;
#endif /* CONFIG_LIBSBC */

static void msbc_decode(uint8_t *data, uint32_t len)
{
	printk("mSBC codec is not supported\n");
#if defined(CONFIG_LIBSBC)
	/* Not Supported */
#endif /* CONFIG_LIBSBC */
}

static void cvsd_decode(uint8_t *data, uint32_t len)
{
	uint32_t added_len;

	if (IS_ENABLED(CONFIG_APP_CVSD_CODEC_ENABLED)) {
		/* Not Supported */
		printk("CVSD codec is not supported\n");
		return;
	}

	added_len = ring_buf_put(&raw_rx_ringbuf, data, len);
	if (added_len != len) {
		printk("Raw RX ring buffer overflow\n");
	}
}

static void voice_rx_task(void *p1, void *p2, void *p3)
{
	uint32_t required_len;
	uint32_t len;
	uint8_t *data;
	bool is_started = true;
	static uint8_t temp_buffer[BLOCK_SIZE];

	while (true) {
		k_sem_take(&voice_rx_sem, K_MSEC(CONFIG_AUDIO_TRANSFER_INTERVAL));

		if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
			k_mutex_lock(&voice_rx_mutex, K_FOREVER);
			ring_buf_reset(&sco_rx_ringbuf);
			k_mutex_unlock(&voice_rx_mutex);
			ring_buf_reset(&raw_rx_ringbuf);
			is_started = true;
			continue;
		}

		if (!IS_ENABLED(CONFIG_APP_CVSD_CODEC_ENABLED)) {
			required_len = pcm_samples * MAX_SAMPLE_BIT_WIDTH / 8;
		} else {
			required_len = pcm_samples;
		}

		k_mutex_lock(&voice_rx_mutex, K_FOREVER);
		len = ring_buf_size_get(&sco_rx_ringbuf);
		if (len < required_len) {
			k_mutex_unlock(&voice_rx_mutex);
			continue;
		}

		len = ring_buf_get_ptr(&sco_rx_ringbuf, &data, 0);
		if (len < required_len) {
			uint32_t offset = len;

			memcpy(&temp_buffer[0], data, len);
			len = ring_buf_get_ptr(&sco_rx_ringbuf, &data, offset);
			memcpy(&temp_buffer[offset], data, required_len - offset);
			data = temp_buffer;
		}
		k_mutex_unlock(&voice_rx_mutex);

		if (selected_codec_id == BT_HFP_HF_CODEC_CVSD) {
			cvsd_decode(data, required_len);
		} else if (IS_ENABLED(CONFIG_LIBSBC) && selected_codec_id == BT_HFP_HF_CODEC_MSBC) {
			msbc_decode(data, required_len);
		}

		k_mutex_lock(&voice_rx_mutex, K_FOREVER);
		ring_buf_consume(&sco_rx_ringbuf, required_len);
		k_mutex_unlock(&voice_rx_mutex);

		len = ring_buf_size_get(&raw_rx_ringbuf);
		if (is_started && len < (BLOCK_SIZE * 2)) {
			continue;
		}

		is_started = false;

		if (len >= BLOCK_SIZE) {
			len = ring_buf_get_ptr(&raw_rx_ringbuf, &data, 0);
			if (len < BLOCK_SIZE) {
				uint32_t offset = len;

				memcpy(&temp_buffer[0], data, len);
				len = ring_buf_get_ptr(&raw_rx_ringbuf, &data, offset);
				memcpy(&temp_buffer[offset], data, BLOCK_SIZE - offset);
				data = temp_buffer;
			}

			if (rx_cb != NULL) {
				rx_cb(data, BLOCK_SIZE);
			}
			ring_buf_consume(&raw_rx_ringbuf, BLOCK_SIZE);
		}

		k_yield();
	}
}

static void msbc_encode(struct net_buf *buf, const uint8_t *data, uint32_t len)
{
	printk("mSBC codec is not supported\n");
#if defined(CONFIG_LIBSBC)
	/* Not Supported */
#endif /* CONFIG_LIBSBC */
}

static void cvsd_encode(struct net_buf *buf, const uint8_t *data, uint32_t len)
{
	if (IS_ENABLED(CONFIG_APP_CVSD_CODEC_ENABLED)) {
		/* Not Supported */
		printk("CVSD codec is not supported\n");
		return;
	}

	len = MIN(len, sco_mtu);
	net_buf_add_mem(buf, data, len);
}

static void voice_tx_task(void *p1, void *p2, void *p3)
{
	struct net_buf *tx_buf = NULL;
	uint32_t len;
	uint8_t *data;
	int err;
	k_timeout_t timeout = K_FOREVER;
	int64_t last_sent_time = 0;
	int64_t current_time;
	int64_t delta_time;
	bool is_silence_data = false;
	static uint8_t temp_buffer[BLOCK_SIZE];

	while (true) {
		k_sem_take(&voice_tx_sem, timeout);

		if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
			net_buf_drop(&tx_buf);
			k_mutex_lock(&voice_tx_mutex, K_FOREVER);
			ring_buf_reset(&raw_tx_ringbuf);
			k_mutex_unlock(&voice_tx_mutex);
			timeout = K_FOREVER;
			last_sent_time = 0;
			is_silence_data = false;
			continue;
		}

		if (tx_buf == NULL) {
			tx_buf = net_buf_alloc(&tx_pool, K_MSEC(CONFIG_AUDIO_TRANSFER_INTERVAL));
			if (tx_buf == NULL) {
				continue;
			}
			net_buf_reserve(tx_buf, BT_SCO_CHAN_SEND_RESERVE);
		}

		k_mutex_lock(&voice_tx_mutex, K_FOREVER);
		len = ring_buf_size_get(&raw_tx_ringbuf);
		k_mutex_unlock(&voice_tx_mutex);
		if (last_sent_time == 0 && len < (BLOCK_SIZE * 2)) {
			continue;
		}

		current_time = k_uptime_get();
		if (last_sent_time == 0) {
			last_sent_time = current_time * 1000;
		}

		delta_time = current_time * 1000 - last_sent_time;
		if (delta_time < sco_trans_interval) {
			timeout = K_NSEC((sco_trans_interval - delta_time)  * 1000);
			continue;
		}

		delta_time = delta_time - sco_trans_interval;
		if (delta_time > sco_trans_interval) {
			timeout = K_NO_WAIT;
		} else {
			timeout = K_NSEC((sco_trans_interval - delta_time) * 1000);
		}

		k_mutex_lock(&voice_tx_mutex, K_FOREVER);
		len = ring_buf_size_get(&raw_tx_ringbuf);
		if (len < pcm_len) {
			is_silence_data = true;
		}

		if (!is_silence_data) {
			len = ring_buf_get_ptr(&raw_tx_ringbuf, &data, 0);
			if (len < pcm_len) {
				uint32_t offset = len;

				memcpy(&temp_buffer[0], data, len);
				len = ring_buf_get_ptr(&raw_tx_ringbuf, &data, offset);
				memcpy(&temp_buffer[offset], data, pcm_len - offset);
				data = temp_buffer;
			}
		} else {
			memset(temp_buffer, 0, pcm_len);
			data = temp_buffer;
		}
		k_mutex_unlock(&voice_tx_mutex);

		if (selected_codec_id == BT_HFP_HF_CODEC_CVSD) {
			cvsd_encode(tx_buf, data, pcm_len);
		} else if (IS_ENABLED(CONFIG_LIBSBC) && selected_codec_id == BT_HFP_HF_CODEC_MSBC) {
			msbc_encode(tx_buf, data, pcm_len);
		}

		if (!is_silence_data) {
			k_mutex_lock(&voice_tx_mutex, K_FOREVER);
			ring_buf_consume(&raw_tx_ringbuf, pcm_len);
			k_mutex_unlock(&voice_tx_mutex);
		}

		err = bt_sco_stream_send(&voice_stream, tx_buf);
		if (err < 0) {
			net_buf_unref(tx_buf);
			printk("Failed to send SCO data: %d\n", err);
		} else {
			last_sent_time += sco_trans_interval;
		}
		tx_buf = NULL;
		is_silence_data = false;

		k_yield();
	}
}

static void voice_rx_cb(struct bt_sco_stream *stream, uint8_t flag, struct net_buf *buf)
{
	uint32_t added_len;

	if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
		return;
	}

	k_mutex_lock(&voice_rx_mutex, K_FOREVER);
	if (ring_buf_space_get(&sco_rx_ringbuf) < buf->len) {
		k_mutex_unlock(&voice_rx_mutex);
		printk("sco_rx_ringbuf is full, dropping data\n");
		return;
	}
	added_len = ring_buf_put(&sco_rx_ringbuf, buf->data, buf->len);
	k_mutex_unlock(&voice_rx_mutex);
	if (added_len != buf->len) {
		printk("sco_rx_ringbuf overflow\n");
	}
	k_sem_give(&voice_rx_sem);
}

static void voice_tx_cb(struct bt_sco_stream *stream)
{
}

#if defined(CONFIG_LIBSBC)
#define MSBC_BIT_RATE 61
#define MSBC_SUBBANDS 8
#define MSBC_BLOCK_LEN 15
#define MSBC_BITPOOL 26
#define MSBC_ALLOC_MTHD SBC_ALLOC_MTHD_LOUDNESS
#define MSBC_CHANNEL_MODE SBC_CH_MODE_MONO
#define MSBC_CHANNEL_NUM 1

static int msbc_init(void)
{
	struct sbc_encoder_init_param param;
	int err;

	param.bit_rate = MSBC_BIT_RATE;
	param.samp_freq = 16000;
	param.blk_len = MSBC_BLOCK_LEN;
	param.subband = MSBC_SUBBANDS;
	param.alloc_mthd = MSBC_ALLOC_MTHD;
	param.ch_mode = MSBC_CHANNEL_MODE;
	param.ch_num = MSBC_CHANNEL_NUM;
	param.min_bitpool = MSBC_BITPOOL;
	param.max_bitpool = MSBC_BITPOOL;

	err = sbc_setup_encoder(&encoder, &param);
	if (err != 0) {
		printk("Failed to setup SBC encoder: %d\n", err);
		return err;
	}

	err = sbc_setup_decoder(&decoder);
	if (err != 0) {
		printk("Failed to setup SBC decoder: %d\n", err);
		return err;
	}
	return 0;
}
#endif /* CONFIG_LIBSBC */

int voice_init(struct bt_conn *sco_conn, uint8_t air_mode, uint8_t codec_id)
{
	struct bt_conn_info info;
	uint16_t frequency;
	int err;

	static struct bt_sco_stream_ops ops = {
		.recv = voice_rx_cb,
		.sent = voice_tx_cb,
	};
	static struct k_thread voice_tx_thread;
	static k_tid_t voice_tx_thread_id;
	static struct k_thread voice_rx_thread;
	static k_tid_t voice_rx_thread_id;

	if (air_mode == BT_HCI_CODING_FORMAT_CVSD) {
		frequency = 8; /* KHz*/
	} else if (IS_ENABLED(CONFIG_LIBSBC) && air_mode == BT_HCI_CODING_FORMAT_TRANSPARENT) {
		frequency = 16; /* KHz*/
	} else {
		return -EINVAL;
	}

	err = bt_conn_get_info(sco_conn, &info);
	if (err != 0) {
		printk("Failed to get sco conn %p info\n", sco_conn);
		return err;
	}

	sco_mtu = info.sco.mtu;
	if (info.sco.link_type == BT_HCI_SCO) {
		pcm_samples = 6 * 625 * frequency / 1000;
		sco_trans_interval = 6 * 625;
	} else {
		pcm_samples = info.sco.interval * 625 * frequency / 1000;
		sco_trans_interval = info.sco.interval * 625;
	}
	pcm_len = pcm_samples * MAX_SAMPLE_BIT_WIDTH / 8;

	selected_codec_id = codec_id;

	k_mutex_lock(&voice_tx_mutex, K_FOREVER);
	ring_buf_reset(&raw_tx_ringbuf);
	k_mutex_unlock(&voice_tx_mutex);

	k_mutex_lock(&voice_rx_mutex, K_FOREVER);
	ring_buf_reset(&sco_rx_ringbuf);
	k_mutex_unlock(&voice_rx_mutex);

#if defined(CONFIG_LIBSBC)
	err = msbc_init();
	if (err != 0) {
		printk("Failed to initialize mSBC: %d\n", err);
		return err;
	}
#endif /* CONFIG_LIBSBC */

	err = bt_sco_stream_cb_register(&voice_stream, &ops);
	if (err != 0) {
		printk("Failed to register SCO stream callbacks: %d\n", err);
		return err;
	}

	err = bt_sco_stream_connect(sco_conn, &voice_stream);
	if (err != 0) {
		printk("Failed to connect SCO stream: %d\n", err);
		return err;
	}

	if (voice_rx_thread_id == NULL) {
		voice_rx_thread_id = k_thread_create(&voice_rx_thread, voice_rx_thread_stack,
						   K_KERNEL_STACK_SIZEOF(voice_rx_thread_stack),
						   voice_rx_task, NULL, NULL, NULL,
						   CONFIG_VOICE_RX_THREAD_PRIO, 0,
						   K_NO_WAIT);
		k_thread_name_set(voice_rx_thread_id, "HFP VOICE RX");
	}

	if (voice_tx_thread_id == NULL) {
		voice_tx_thread_id = k_thread_create(&voice_tx_thread, voice_tx_thread_stack,
						   K_KERNEL_STACK_SIZEOF(voice_tx_thread_stack),
						   voice_tx_task, NULL, NULL, NULL,
						   CONFIG_VOICE_TX_THREAD_PRIO, 0,
						   K_NO_WAIT);
		k_thread_name_set(voice_tx_thread_id, "HFP VOICE TX");
	}

	atomic_set_bit(&flags, VOICE_OVER_HCI_STARTED);
	return 0;
}

int voice_tx(const uint8_t *data, uint32_t len)
{
	static uint8_t mono_pcm_buf[BLOCK_SIZE];
	uint32_t added_len;

	if (len != (BLOCK_SIZE * 2)) {
		printk("Invalid data len %u != %u\n", len, BLOCK_SIZE * 2);
		return -EINVAL;
	}

	if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
		printk("Stream has been stopped\n");
		return -ENOTCONN;
	}

	/* Covert stereo to mono PCM data */
	for (uint32_t i = 0; i < BLOCK_SIZE; i += 2) {
		uint16_t *dst;
		uint16_t *src;

		dst = (uint16_t *)&mono_pcm_buf[i];
		src = (uint16_t *)&data[i * 2 + 2];

		*dst = (uint16_t)(*src);
	}

	k_mutex_lock(&voice_tx_mutex, K_FOREVER);
	if (ring_buf_space_get(&raw_tx_ringbuf) < BLOCK_SIZE) {
		k_mutex_unlock(&voice_tx_mutex);
		printk("raw_tx_ringbuf full, dropping data\n");
		return 0;
	}

	added_len = ring_buf_put(&raw_tx_ringbuf, mono_pcm_buf, BLOCK_SIZE);
	k_mutex_unlock(&voice_tx_mutex);
	if (added_len != BLOCK_SIZE) {
		printk("raw_tx_ringbuf overflow\n");
	}

	k_sem_give(&voice_tx_sem);
	return 0;
}

int voice_rx_start(voice_rx_cb_t cb)
{
	rx_cb = cb;
	return 0;
}

int voice_rx_stop(void)
{
	return 0;
}

int voice_deinit(void)
{
	int err;

	atomic_clear_bit(&flags, VOICE_OVER_HCI_STARTED);

	err = bt_sco_stream_disconnect(&voice_stream);
	if (err != 0) {
		printk("Failed to disconnect SCO stream: %d\n", err);
		return err;
	}

	err = bt_sco_stream_cb_unregister(&voice_stream);
	if (err != 0) {
		printk("Failed to register SCO stream callbacks: %d\n", err);
		return err;
	}

	return 0;
}
