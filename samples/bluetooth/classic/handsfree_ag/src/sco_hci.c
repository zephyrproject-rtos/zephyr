/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/classic/sco.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/ring_buffer.h>

#include "cvsd.h"
#include "msbc.h"
#include "sco_hci.h"

#if defined(CONFIG_HFP_AUDIO_PATH_SCO_HCI)

#define CVSD_SCO_PKT_LEN   60U
#define CVSD_PCM_SAMPLES   (CVSD_SCO_PKT_LEN * 8U)
#define CVSD_XFER_INTERVAL CONFIG_AUDIO_TRANSFER_INTERVAL

#define MAX_PCM_SAMPLES    CVSD_PCM_SAMPLES
#define MAX_PCM_BLOCK_BYTES (MAX_PCM_SAMPLES * sizeof(int16_t))
#define MAX_SCO_PKT_LEN    CVSD_SCO_PKT_LEN
#define RING_BUF_SIZE      (MAX_PCM_BLOCK_BYTES * 4U)

static struct bt_conn *active_conn;
static struct cvsd_ctx cvsd_dec_ctx;
static struct cvsd_ctx cvsd_enc_ctx;
static struct k_work_delayable tx_work;
static bool tx_running;
static uint8_t air_mode;
static uint16_t sco_pkt_len;
static uint16_t pcm_samples;
static uint16_t xfer_interval_ms;

static int16_t pcm_block[MAX_PCM_SAMPLES];
static uint8_t tx_pkt[MAX_SCO_PKT_LEN];

#if DT_HAS_ALIAS(codec0)
static const struct device *codec_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
static uint8_t ring_buffer[RING_BUF_SIZE];
static struct ring_buf rb;
static bool codec_ready;

static void codec_tx_done(const struct device *dev, void *user_data)
{
	uint32_t pcm_block_bytes = pcm_samples * sizeof(int16_t);
	uint32_t avail;
	uint32_t read;

	ARG_UNUSED(user_data);

	avail = ring_buf_size_get(&rb);
	if (avail < pcm_block_bytes) {
		return;
	}

	read = ring_buf_get(&rb, (uint8_t *)pcm_block, pcm_block_bytes);
	if (read != pcm_block_bytes) {
		return;
	}

	(void)audio_codec_write(dev, (uint8_t *)pcm_block, pcm_block_bytes);
}

static int codec_configure(uint32_t sample_rate)
{
	uint32_t pcm_block_bytes = pcm_samples * sizeof(int16_t);
	audio_property_value_t vol = {.vol = 15};
	struct audio_codec_cfg cfg = {
		.dai_type = AUDIO_DAI_TYPE_PCM,
		.dai_cfg.pcm.dir = AUDIO_DAI_DIR_TX,
		.dai_cfg.pcm.pcm_width = AUDIO_PCM_WIDTH_16_BITS,
		.dai_cfg.pcm.channels = 1U,
		.dai_cfg.pcm.block_size = pcm_block_bytes,
		.dai_cfg.pcm.samplerate = sample_rate,
	};
	int err;

	if (!device_is_ready(codec_dev)) {
		printk("Codec device not ready\n");
		return -ENODEV;
	}

	ring_buf_init(&rb, RING_BUF_SIZE, ring_buffer);
	audio_codec_register_done_callback(codec_dev, codec_tx_done, NULL, NULL, NULL);

	err = audio_codec_configure(codec_dev, &cfg);
	if (err != 0) {
		return err;
	}

	err = audio_codec_start(codec_dev, AUDIO_DAI_DIR_TX);
	if (err != 0) {
		return err;
	}

	err = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, 0, vol);
	if (err != 0) {
		return err;
	}

	codec_ready = true;
	return 0;
}

static void play_pcm(const int16_t *samples, size_t count)
{
	uint32_t bytes = count * sizeof(int16_t);

	if (!codec_ready) {
		return;
	}

	if (ring_buf_put(&rb, (const uint8_t *)samples, bytes) != bytes) {
		printk("Audio ring buffer full\n");
	}
}
#else
static int codec_configure(uint32_t sample_rate)
{
	ARG_UNUSED(sample_rate);
	return 0;
}

static void play_pcm(const int16_t *samples, size_t count)
{
	ARG_UNUSED(samples);
	ARG_UNUSED(count);
}
#endif /* DT_HAS_ALIAS(codec0) */

static bool air_mode_is_msbc(uint8_t mode)
{
	return mode == BT_HCI_CODING_FORMAT_TRANSPARENT && IS_ENABLED(CONFIG_HFP_SCO_HCI_MSBC);
}

static int setup_air_mode(uint8_t mode)
{
	uint32_t sample_rate;
	int err;

	air_mode = mode;

	if (mode == BT_HCI_CODING_FORMAT_CVSD) {
		sco_pkt_len = CVSD_SCO_PKT_LEN;
		pcm_samples = CVSD_PCM_SAMPLES;
		xfer_interval_ms = CVSD_XFER_INTERVAL;
		sample_rate = 8000;
		cvsd_init(&cvsd_dec_ctx);
		cvsd_init(&cvsd_enc_ctx);
	} else if (air_mode_is_msbc(mode)) {
		sco_pkt_len = MSBC_SCO_PKT_LEN;
		pcm_samples = MSBC_PCM_SAMPLES;
		xfer_interval_ms = MSBC_XFER_INTERVAL_MS;
		sample_rate = 16000;
		err = msbc_init();
		if (err != 0) {
			printk("Failed to init mSBC: %d\n", err);
			return err;
		}
	} else {
		printk("Unsupported SCO air mode: %u\n", mode);
		return -ENOTSUP;
	}

	return codec_configure(sample_rate);
}

static void sco_recv(struct bt_conn *conn, struct net_buf *buf)
{
	int16_t pcm[MAX_PCM_SAMPLES];
	int decoded;
	uint16_t len = buf->len;

	if (conn != active_conn) {
		net_buf_unref(buf);
		return;
	}

	if (len > sco_pkt_len) {
		len = sco_pkt_len;
	}

	if (air_mode == BT_HCI_CODING_FORMAT_CVSD) {
		cvsd_decode(&cvsd_dec_ctx, buf->data, len, pcm);
		play_pcm(pcm, len * 8U);
	} else if (air_mode_is_msbc(air_mode)) {
		decoded = msbc_decode(buf->data, len, pcm, MAX_PCM_SAMPLES);
		if (decoded > 0) {
			play_pcm(pcm, decoded);
		}
	}

	net_buf_unref(buf);
}

static void tx_work_handler(struct k_work *work)
{
	struct net_buf *buf;
	int err;
	int pkt_len;

	ARG_UNUSED(work);

	if (!tx_running || active_conn == NULL) {
		return;
	}

	memset(pcm_block, 0, pcm_samples * sizeof(int16_t));

	if (air_mode == BT_HCI_CODING_FORMAT_CVSD) {
		cvsd_encode(&cvsd_enc_ctx, pcm_block, pcm_samples, tx_pkt);
		pkt_len = sco_pkt_len;
	} else if (air_mode_is_msbc(air_mode)) {
		pkt_len = msbc_encode(pcm_block, pcm_samples, tx_pkt, sizeof(tx_pkt));
		if (pkt_len < 0) {
			k_work_schedule(&tx_work, K_MSEC(xfer_interval_ms));
			return;
		}
	} else {
		return;
	}

	buf = bt_sco_buf_alloc(K_NO_WAIT);
	if (buf == NULL) {
		k_work_schedule(&tx_work, K_MSEC(xfer_interval_ms));
		return;
	}

	net_buf_add_mem(buf, tx_pkt, pkt_len);
	err = bt_sco_send(active_conn, buf);
	if (err != 0) {
		printk("Failed to send SCO data: %d\n", err);
	}

	k_work_schedule(&tx_work, K_MSEC(xfer_interval_ms));
}

int sco_hci_init(uint8_t mode)
{
	int err;

	err = setup_air_mode(mode);
	if (err != 0) {
		return err;
	}

	k_work_init_delayable(&tx_work, tx_work_handler);

	return 0;
}

int sco_hci_start(struct bt_conn *sco_conn)
{
	int err;

	err = bt_sco_recv_cb_set(sco_conn, sco_recv);
	if (err != 0) {
		return err;
	}

	active_conn = bt_conn_ref(sco_conn);
	tx_running = true;
	k_work_schedule(&tx_work, K_MSEC(xfer_interval_ms));
	return 0;
}

int sco_hci_stop(void)
{
	tx_running = false;
	k_work_cancel_delayable(&tx_work);

	if (active_conn != NULL) {
		(void)bt_sco_recv_cb_set(active_conn, NULL);
		bt_conn_unref(active_conn);
		active_conn = NULL;
	}

#if DT_HAS_ALIAS(codec0)
	if (codec_ready) {
		(void)audio_codec_stop(codec_dev, AUDIO_DAI_DIR_TX);
		codec_ready = false;
	}
#endif

	return 0;
}

#else

int sco_hci_init(uint8_t air_mode)
{
	ARG_UNUSED(air_mode);
	return -ENOTSUP;
}

int sco_hci_start(struct bt_conn *sco_conn)
{
	ARG_UNUSED(sco_conn);
	return -ENOTSUP;
}

int sco_hci_stop(void)
{
	return 0;
}

#endif /* CONFIG_HFP_AUDIO_PATH_SCO_HCI */
