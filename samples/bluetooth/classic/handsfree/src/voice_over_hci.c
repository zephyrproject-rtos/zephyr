/* voice_over_hci.c - voice implementation for Bluetooth Hands-Free Profile */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/toolchain.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sco.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/sbc.h>

#include "voice.h"

static struct bt_sco_stream voice_stream;
static uint8_t selected_codec_id;
static voice_rx_cb_t rx_cb;
static uint8_t sco_mtu;
static atomic_t flags;

#define VOICE_OVER_HCI_STARTED 1

static K_KERNEL_STACK_MEMBER(voice_tx_thread_stack, CONFIG_VOICE_TX_THREAD_STACK_SIZE);
static K_KERNEL_STACK_MEMBER(voice_rx_thread_stack, CONFIG_VOICE_RX_THREAD_STACK_SIZE);

#define MAX_SAMPLE_FREQ      16000
#define MAX_SAMPLE_BIT_WIDTH 16
#define MAX_CHANNELS         1

#define SAMPLES_PER_BLOCK ((MAX_SAMPLE_FREQ * CONFIG_AUDIO_TRANSFER_INTERVAL / 1000) * MAX_CHANNELS)
#define BLOCK_SIZE     (MAX_SAMPLE_BIT_WIDTH * SAMPLES_PER_BLOCK / 8)

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_VOICE_BUFFERS + 1, BT_SCO_SDU_SIZE(UINT8_MAX),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(rx_pool, CONFIG_BT_SCO_RX_BUF_COUNT + 1, CONFIG_BT_SCO_RX_BUF_SIZE,
			  0, NULL);

NET_BUF_POOL_FIXED_DEFINE(raw_rx_pool, CONFIG_BT_SCO_RX_BUF_COUNT + 1, BLOCK_SIZE, 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(raw_tx_pool, CONFIG_VOICE_BUFFERS + 1, BLOCK_SIZE, 0, NULL);

static struct k_queue voice_rx_queue;
static struct k_queue voice_tx_queue;

static struct sbc_encoder encoder;
static struct sbc_decoder decoder;

static void msbc_decode(struct net_buf *buf, struct net_buf *rx_buf)
{
	uint16_t len;

	/* mSBC decode here */
	len = buf->len;
	len = MIN(len, (BLOCK_SIZE - rx_buf->len));
	net_buf_add_mem(rx_buf, buf->data, len);
	net_buf_pull(buf, len);
}

static void cvsd_decode(struct net_buf *buf, struct net_buf *rx_buf)
{
	uint16_t len;

	/* CVSD decode here */
	len = buf->len;
	len = MIN(len, (BLOCK_SIZE - rx_buf->len));
	net_buf_add_mem(rx_buf, buf->data, len);
	net_buf_pull(buf, len);
}

static void voice_rx_task(void *p1, void *p2, void *p3)
{
	struct net_buf *buf = NULL;
	struct net_buf *rx_buf = NULL;

	while (true) {
		if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
			if (buf != NULL) {
				net_buf_unref(buf);
			}

			if (rx_buf != NULL) {
				net_buf_unref(rx_buf);
			}

			while ((buf = k_queue_get(&voice_rx_queue, K_NO_WAIT)) != NULL) {
				net_buf_unref(buf);
			}
			buf = NULL;
			rx_buf = NULL;
		}

		if (buf == NULL) {
			buf = k_queue_get(&voice_rx_queue, K_MSEC(100));
			if (buf == NULL) {
				continue;
			}
		}

		if (rx_buf == NULL) {
			rx_buf = net_buf_alloc(&raw_rx_pool, K_MSEC(100));
			if (rx_buf == NULL) {
				continue;
			}
		}

		if (selected_codec_id == BT_HFP_HF_CODEC_CVSD) {
			cvsd_decode(buf, rx_buf);
		} else if (selected_codec_id == BT_HFP_HF_CODEC_MSBC) {
			msbc_decode(buf, rx_buf);
		}

		if (rx_buf->len >= BLOCK_SIZE) {
			if (rx_cb != NULL) {
				rx_cb(rx_buf->data, rx_buf->len);
			}
			net_buf_unref(rx_buf);
			rx_buf = NULL;
		}

		if (buf->len == 0) {
			net_buf_unref(buf);
			buf = NULL;
		}

		k_yield();
	}
}

static void msbc_encode(struct net_buf *buf, struct net_buf *tx_buf)
{
	uint16_t len;

	/* mSBC encode here */
	len = buf->len;
	len = MIN(len, (sco_mtu - tx_buf->len));
	net_buf_add_mem(tx_buf, buf->data, len);
	net_buf_pull(buf, len);
}

static void cvsd_encode(struct net_buf *buf, struct net_buf *tx_buf)
{
	uint16_t len;

	/* CVSD encode here */
	len = buf->len;
	len = MIN(len, (sco_mtu - tx_buf->len));
	net_buf_add_mem(tx_buf, buf->data, len);
	net_buf_pull(buf, len);
}

static void voice_tx_task(void *p1, void *p2, void *p3)
{
	struct net_buf *buf = NULL;
	struct net_buf *tx_buf = NULL;
	int err;

	while (true) {
		if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
			if (buf != NULL) {
				net_buf_unref(buf);
			}

			if (tx_buf != NULL) {
				net_buf_unref(tx_buf);
			}

			while ((buf = k_queue_get(&voice_tx_queue, K_NO_WAIT)) != NULL) {
				net_buf_unref(buf);
			}
			buf = NULL;
			tx_buf = NULL;
		}

		if (buf == NULL) {
			buf = k_queue_get(&voice_tx_queue, K_MSEC(100));
			if (buf == NULL) {
				continue;
			}
		}

		if (tx_buf == NULL) {
			tx_buf = net_buf_alloc(&tx_pool, K_MSEC(100));
			if (tx_buf == NULL) {
				continue;
			}
			net_buf_reserve(tx_buf, BT_SCO_CHAN_SEND_RESERVE);
		}

		if (selected_codec_id == BT_HFP_HF_CODEC_CVSD) {
			cvsd_encode(buf, tx_buf);
		} else if (selected_codec_id == BT_HFP_HF_CODEC_MSBC) {
			msbc_encode(buf, tx_buf);
		}

		if (tx_buf->len >= sco_mtu) {
			err = bt_sco_stream_send(&voice_stream, tx_buf);
			if (err < 0) {
				net_buf_unref(tx_buf);
				printk("Failed to send SCO data: %d\n", err);
			}
			tx_buf = NULL;
		}

		if (buf->len == 0) {
			net_buf_unref(buf);
			buf = NULL;
		}

		k_yield();
	}
}

void voice_rx_cb(struct bt_sco_stream *stream, uint8_t flag, struct net_buf *buf)
{
	struct net_buf *rx_buf;

	if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
		return;
	}

	rx_buf = net_buf_alloc(&rx_pool, K_MSEC(100));
	if (rx_buf == NULL) {
		return;
	}

	net_buf_add_mem(rx_buf, buf->data, buf->len);

	k_queue_append(&voice_rx_queue, rx_buf);
}

void voice_tx_cb(struct bt_sco_stream *stream)
{
}

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

int voice_init(struct bt_conn *sco_conn, uint8_t air_mode, uint8_t codec_id)
{
	struct bt_conn_info info;
	int err;

	static struct bt_sco_stream_ops ops = {
		.recv = voice_rx_cb,
		.sent = voice_tx_cb,
	};
	static struct k_thread voice_tx_thread;
	static k_tid_t voice_tx_thread_id;
	static struct k_thread voice_rx_thread;
	static k_tid_t voice_rx_thread_id;

	err = bt_conn_get_info(sco_conn, &info);
	if (err != 0) {
		printk("Failed to get sco conn %p info\n", sco_conn);
		return err;
	}

	sco_mtu = info.sco.mtu;

	selected_codec_id = codec_id;

	err = msbc_init();
	if (err != 0) {
		printf("Failed to initialize mSBC: %d\n", err);
		return err;
	}

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
		k_queue_init(&voice_rx_queue);
		voice_rx_thread_id = k_thread_create(&voice_rx_thread, voice_rx_thread_stack,
						   K_KERNEL_STACK_SIZEOF(voice_rx_thread_stack),
						   voice_rx_task, NULL, NULL, NULL,
						   CONFIG_VOICE_RX_THREAD_PRIO, 0,
						   K_NO_WAIT);
		k_thread_name_set(voice_rx_thread_id, "HFP VOICE RX");
	}

	if (voice_tx_thread_id == NULL) {
		k_queue_init(&voice_tx_queue);
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
	struct net_buf *tx_raw_buf;

	if (len != (BLOCK_SIZE * 2)) {
		printk("Invalid data len %u != %u\n", len, BLOCK_SIZE * 2);
		return -EINVAL;
	}

	if (!atomic_test_bit(&flags, VOICE_OVER_HCI_STARTED)) {
		printk("Stream has been stopped\n");
		return -ENOTCONN;
	}

	tx_raw_buf = net_buf_alloc(&raw_tx_pool, K_NO_WAIT);

	if (tx_raw_buf == NULL) {
		printk("Failed to allocate TX buffer\n");
		return -ENOBUFS;
	}

	for (uint32_t i = 0; i < BLOCK_SIZE; i += 2) {
		uint16_t *dst;
		uint32_t *src;

		dst = (uint16_t *)&(tx_raw_buf->data)[i];
		src = (uint32_t *)&data[i * 2];

		*dst = (uint16_t)(*src);
	}

	net_buf_add(tx_raw_buf, BLOCK_SIZE);
	k_queue_append(&voice_tx_queue, tx_raw_buf);
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
