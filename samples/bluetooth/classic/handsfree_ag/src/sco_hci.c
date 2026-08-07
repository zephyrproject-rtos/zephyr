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
#include <zephyr/sys/util.h>

#include "msbc.h"
#include "sco_hci.h"

#if defined(CONFIG_HFP_AUDIO_PATH_SCO_HCI)

/*
 * HFP programs Voice_Setting with linear 16-bit PCM for both CVSD and
 * transparent air coding. For CVSD air mode the controller runs CVSD on the air
 * interface, so HCI SCO payloads carry plain PCM. For transparent air mode the
 * host exchanges mSBC frames prefixed with an H2 header.
 *
 * Speaker data comes from the SCO link and is queued for the codec DAC, while
 * microphone data captured by the codec ADC is queued for the SCO link.
 *
 * The payload length of the (e)SCO link is negotiated by the controllers and is
 * not necessarily one mSBC frame, so mSBC frames are reassembled on receive and
 * sliced into payload sized chunks on transmit.
 */

/* Default CVSD/PCM SCO payload: 60 bytes = 30 samples @ 8 kHz (3.75 ms) */
#define CVSD_SCO_PKT_LEN_DEFAULT 60U
#define CVSD_SAMPLE_RATE         8000U
#define CVSD_CODEC_SAMPLES       160U /* 20 ms codec DMA block */

#define MSBC_SAMPLE_RATE 16000U

#define MAX_SCO_PKT_LEN     MAX(MSBC_SCO_PKT_LEN, CONFIG_BT_BUF_SCO_TX_SIZE)
#define MAX_PCM_SAMPLES     MAX(MAX(CVSD_CODEC_SAMPLES, MSBC_PCM_SAMPLES), MAX_SCO_PKT_LEN / 2U)
#define MAX_PCM_BLOCK_BYTES (MAX_PCM_SAMPLES * sizeof(int16_t))
#define RING_BUF_SIZE       (MAX_PCM_BLOCK_BYTES * 4U)

/* Report a silent link once instead of on every transmit interval */
#define RX_STALL_TIMEOUT_US 1000000U

static struct bt_conn *active_conn;
static struct k_work_delayable tx_work;
static bool tx_running;
static uint8_t air_mode;
static uint16_t sco_pkt_len;
static uint16_t codec_pcm_samples;
static uint32_t sample_rate;
static uint32_t xfer_interval_us;
static uint32_t rx_pkts;
static uint32_t tx_elapsed_us;
static bool rx_stall_reported;
static bool tx_err_reported;

static int16_t enc_block[MAX_PCM_SAMPLES];
static uint8_t tx_pkt[MAX_SCO_PKT_LEN];

/* mSBC frame being sliced into SCO payloads, and stream being reassembled */
static uint8_t tx_frame[MSBC_SCO_PKT_LEN];
static uint16_t tx_frame_len;
static uint16_t tx_frame_pos;
static uint8_t rx_stream[2U * MSBC_SCO_PKT_LEN];
static uint16_t rx_stream_len;

#if DT_HAS_ALIAS(codec0)
static const struct device *codec_dev = DEVICE_DT_GET(DT_ALIAS(codec0));
static uint8_t spk_buffer[RING_BUF_SIZE];
static uint8_t mic_buffer[RING_BUF_SIZE];
static struct ring_buf spk_rb;
static struct ring_buf mic_rb;
static int16_t dac_block[MAX_PCM_SAMPLES];
static bool codec_ready;
static bool spk_playing;

static void codec_tx_done(const struct device *dev, void *user_data)
{
	uint32_t block_bytes = codec_pcm_samples * sizeof(int16_t);
	uint32_t read;

	ARG_UNUSED(user_data);

	/* Build up a small backlog before playing to ride out SCO jitter */
	if (!spk_playing) {
		if (ring_buf_size_get(&spk_rb) < (block_bytes * 2U)) {
			memset(dac_block, 0, block_bytes);
			(void)audio_codec_write(dev, (uint8_t *)dac_block, block_bytes);
			return;
		}

		spk_playing = true;
	}

	read = ring_buf_get(&spk_rb, (uint8_t *)dac_block, block_bytes);
	if (read < block_bytes) {
		memset((uint8_t *)dac_block + read, 0, block_bytes - read);
		spk_playing = false;
	}

	(void)audio_codec_write(dev, (uint8_t *)dac_block, block_bytes);
}

static void codec_rx_done(const struct device *dev, uint8_t *buf, uint32_t len, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (ring_buf_put(&mic_rb, buf, len) != len) {
		/* SCO link cannot keep up with the microphone, restart buffering */
		ring_buf_reset(&mic_rb);
	}
}

static int codec_configure(void)
{
	uint32_t block_bytes = codec_pcm_samples * sizeof(int16_t);
	audio_property_value_t vol = {.vol = 15};
	struct audio_codec_cfg cfg = {
		.dai_type = AUDIO_DAI_TYPE_PCM,
		.dai_cfg.pcm.dir = AUDIO_DAI_DIR_TXRX,
		.dai_cfg.pcm.pcm_width = AUDIO_PCM_WIDTH_16_BITS,
		.dai_cfg.pcm.channels = 1U,
		.dai_cfg.pcm.block_size = block_bytes,
		.dai_cfg.pcm.samplerate = sample_rate,
	};
	int err;

	if (!device_is_ready(codec_dev)) {
		printk("Codec device not ready\n");
		return -ENODEV;
	}

	ring_buf_init(&spk_rb, RING_BUF_SIZE, spk_buffer);
	ring_buf_init(&mic_rb, RING_BUF_SIZE, mic_buffer);
	spk_playing = false;

	audio_codec_register_done_callback(codec_dev, codec_tx_done, NULL, codec_rx_done, NULL);

	err = audio_codec_configure(codec_dev, &cfg);
	if (err != 0) {
		return err;
	}

	err = audio_codec_start(codec_dev, AUDIO_DAI_DIR_TXRX);
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

static void codec_release(void)
{
	if (!codec_ready) {
		return;
	}

	(void)audio_codec_stop(codec_dev, AUDIO_DAI_DIR_TXRX);
	codec_ready = false;
	spk_playing = false;
}

static void play_pcm(const int16_t *samples, size_t count)
{
	uint32_t bytes = count * sizeof(int16_t);

	if (!codec_ready) {
		return;
	}

	if (ring_buf_put(&spk_rb, (const uint8_t *)samples, bytes) != bytes) {
		printk("Speaker ring buffer full\n");
	}
}

static void capture_pcm(int16_t *samples, size_t count)
{
	uint32_t bytes = count * sizeof(int16_t);
	uint32_t read = 0;

	if (codec_ready) {
		read = ring_buf_get(&mic_rb, (uint8_t *)samples, bytes);
	}

	if (read < bytes) {
		memset((uint8_t *)samples + read, 0, bytes - read);
	}
}
#else
static int codec_configure(void)
{
	return 0;
}

static void codec_release(void)
{
}

static void play_pcm(const int16_t *samples, size_t count)
{
	ARG_UNUSED(samples);
	ARG_UNUSED(count);
}

static void capture_pcm(int16_t *samples, size_t count)
{
	memset(samples, 0, count * sizeof(int16_t));
}
#endif /* DT_HAS_ALIAS(codec0) */

static bool air_mode_is_msbc(uint8_t mode)
{
	return mode == BT_HCI_CODING_FORMAT_TRANSPARENT && IS_ENABLED(CONFIG_HFP_SCO_HCI_MSBC);
}

/* Adopt the payload length the controller actually uses for this link */
static void update_sco_pkt_len(uint16_t len)
{
	if (len < 2U || len > MAX_SCO_PKT_LEN || len == sco_pkt_len) {
		return;
	}

	if (air_mode == BT_HCI_CODING_FORMAT_CVSD) {
		if ((len & 1U) != 0U) {
			return;
		}

		xfer_interval_us = ((len / sizeof(int16_t)) * 1000000U) / sample_rate;
	} else {
		xfer_interval_us = (len * 1000000U) / MSBC_BYTE_RATE;
	}

	sco_pkt_len = len;
	printk("SCO payload %u bytes (%u us)\n", sco_pkt_len, xfer_interval_us);
}

static int setup_air_mode(uint8_t mode)
{
	int err;

	air_mode = mode;
	rx_pkts = 0U;
	rx_stream_len = 0U;
	tx_frame_len = 0U;
	tx_frame_pos = 0U;
	tx_elapsed_us = 0U;
	rx_stall_reported = false;
	tx_err_reported = false;

	if (mode == BT_HCI_CODING_FORMAT_CVSD) {
		sco_pkt_len = CVSD_SCO_PKT_LEN_DEFAULT;
		codec_pcm_samples = CVSD_CODEC_SAMPLES;
		sample_rate = CVSD_SAMPLE_RATE;
		xfer_interval_us = ((sco_pkt_len / sizeof(int16_t)) * 1000000U) / sample_rate;
	} else if (air_mode_is_msbc(mode)) {
		sco_pkt_len = MSBC_SCO_PKT_LEN;
		codec_pcm_samples = MSBC_PCM_SAMPLES;
		sample_rate = MSBC_SAMPLE_RATE;
		xfer_interval_us = MSBC_FRAME_DURATION_US;
		err = msbc_init();
		if (err != 0) {
			printk("Failed to init mSBC: %d\n", err);
			return err;
		}
	} else {
		printk("Unsupported SCO air mode: %u\n", mode);
		return -ENOTSUP;
	}

	return codec_configure();
}

/* Reassemble mSBC frames from the SCO byte stream and play what decodes */
static void msbc_stream_recv(const uint8_t *data, uint16_t len)
{
	int16_t pcm[MSBC_PCM_SAMPLES];
	uint16_t chunk;
	int decoded;
	int offset;

	while (len > 0U) {
		chunk = MIN(len, (uint16_t)(sizeof(rx_stream) - rx_stream_len));
		if (chunk == 0U) {
			rx_stream_len = 0U;
			continue;
		}

		memcpy(&rx_stream[rx_stream_len], data, chunk);
		rx_stream_len += chunk;
		data += chunk;
		len -= chunk;

		while (rx_stream_len > 0U) {
			offset = msbc_frame_sync(rx_stream, rx_stream_len);
			if (offset < 0) {
				/* Keep the last byte, it may start an H2 header */
				rx_stream[0] = rx_stream[rx_stream_len - 1U];
				rx_stream_len = 1U;
				break;
			}

			rx_stream_len -= (uint16_t)offset;
			memmove(rx_stream, &rx_stream[offset], rx_stream_len);

			if (rx_stream_len < MSBC_SCO_PKT_LEN) {
				break;
			}

			decoded = msbc_decode(rx_stream, MSBC_SCO_PKT_LEN, pcm, ARRAY_SIZE(pcm));
			rx_stream_len -= MSBC_SCO_PKT_LEN;
			memmove(rx_stream, &rx_stream[MSBC_SCO_PKT_LEN], rx_stream_len);

			if (decoded > 0) {
				play_pcm(pcm, (size_t)decoded);
			}
		}
	}
}

static void sco_recv(struct bt_conn *conn, struct net_buf *buf)
{
	uint16_t len = buf->len;

	if (conn != active_conn) {
		net_buf_unref(buf);
		return;
	}

	rx_pkts++;
	update_sco_pkt_len(len);

	if (air_mode == BT_HCI_CODING_FORMAT_CVSD) {
		if (len >= 2U && (len & 1U) == 0U) {
			play_pcm((const int16_t *)buf->data, len / sizeof(int16_t));
		}
	} else if (air_mode_is_msbc(air_mode)) {
		msbc_stream_recv(buf->data, len);
	}

	net_buf_unref(buf);
}

static void tx_work_handler(struct k_work *work)
{
	struct net_buf *buf;
	uint32_t interval_us = xfer_interval_us;
	int err;
	int pkt_len;

	ARG_UNUSED(work);

	if (!tx_running || active_conn == NULL) {
		return;
	}

	if (air_mode == BT_HCI_CODING_FORMAT_CVSD) {
		pkt_len = sco_pkt_len;
		capture_pcm(enc_block, sco_pkt_len / sizeof(int16_t));
		memcpy(tx_pkt, enc_block, sco_pkt_len);
	} else if (air_mode_is_msbc(air_mode)) {
		if (tx_frame_pos >= tx_frame_len) {
			capture_pcm(enc_block, MSBC_PCM_SAMPLES);
			err = msbc_encode(enc_block, MSBC_PCM_SAMPLES, tx_frame, sizeof(tx_frame));
			if (err < 0) {
				k_work_schedule(&tx_work, K_USEC(interval_us));
				return;
			}

			tx_frame_len = (uint16_t)err;
			tx_frame_pos = 0U;
		}

		pkt_len = MIN(sco_pkt_len, tx_frame_len - tx_frame_pos);
		memcpy(tx_pkt, &tx_frame[tx_frame_pos], pkt_len);
		interval_us = ((uint32_t)pkt_len * 1000000U) / MSBC_BYTE_RATE;
	} else {
		return;
	}

	buf = bt_sco_buf_alloc(K_NO_WAIT);
	if (buf == NULL) {
		k_work_schedule(&tx_work, K_USEC(interval_us));
		return;
	}

	if (air_mode_is_msbc(air_mode)) {
		tx_frame_pos += pkt_len;
	}

	net_buf_add_mem(buf, tx_pkt, pkt_len);
	err = bt_sco_send(active_conn, buf);
	if (err != 0 && !tx_err_reported) {
		tx_err_reported = true;
		printk("Failed to send SCO data: %d\n", err);
	}

	tx_elapsed_us += interval_us;
	if (rx_pkts == 0U && !rx_stall_reported && tx_elapsed_us > RX_STALL_TIMEOUT_US) {
		rx_stall_reported = true;
		printk("No SCO data received from the controller\n");
	}

	k_work_schedule(&tx_work, K_USEC(interval_us));
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
	k_work_schedule(&tx_work, K_USEC(xfer_interval_us));
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

	codec_release();

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
