/** @file
 *  @brief Bluetooth Basic Audio Profile shell
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/gmap.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "common/bt_shell_private.h"
#include "host/shell/bt.h"
#include "audio.h"

/* Determines if we can initiate streaming */
#define IS_BAP_INITIATOR                                                                           \
	(IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SOURCE) || IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT))

#define GENERATE_SINE_SUPPORTED (IS_ENABLED(CONFIG_LIBLC3) && !IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS))

#if defined(CONFIG_BT_BAP_UNICAST)

struct shell_stream unicast_streams[CONFIG_BT_MAX_CONN *
				    MAX(UNICAST_SERVER_STREAM_COUNT, UNICAST_CLIENT_STREAM_COUNT)];

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
struct bt_bap_unicast_group *default_unicast_group;
static struct bt_bap_unicast_client_cb unicast_client_cbs;
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
struct bt_bap_ep *snks[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
struct bt_bap_ep *srcs[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#endif /* CONFIG_BT_BAP_UNICAST */

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
struct shell_stream broadcast_source_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
struct broadcast_source default_source;
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
static struct shell_stream broadcast_sink_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct broadcast_sink default_broadcast_sink;
#endif /* CONFIG_BT_BAP_BROADCAST_SINK */

#if defined(CONFIG_BT_BAP_UNICAST) || defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
static struct bt_bap_stream *default_stream;
#endif /* CONFIG_BT_BAP_UNICAST || CONFIG_BT_BAP_BROADCAST_SOURCE */

#if IS_BAP_INITIATOR
/* Default to 16_2_1 */
struct named_lc3_preset default_sink_preset = {"16_2_1",
					       BT_BAP_LC3_UNICAST_PRESET_16_2_1(LOCATION, CONTEXT)};
struct named_lc3_preset default_source_preset = {
	"16_2_1", BT_BAP_LC3_UNICAST_PRESET_16_2_1(LOCATION, CONTEXT)};
struct named_lc3_preset default_broadcast_source_preset = {
	"16_2_1", BT_BAP_LC3_BROADCAST_PRESET_16_2_1(LOCATION, CONTEXT)};
#endif /* IS_BAP_INITIATOR */

static const struct named_lc3_preset lc3_unicast_presets[] = {
	{"8_1_1", BT_BAP_LC3_UNICAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1", BT_BAP_LC3_UNICAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1", BT_BAP_LC3_UNICAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1", BT_BAP_LC3_UNICAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1", BT_BAP_LC3_UNICAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1", BT_BAP_LC3_UNICAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1", BT_BAP_LC3_UNICAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1", BT_BAP_LC3_UNICAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_BAP_LC3_UNICAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_BAP_LC3_UNICAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1", BT_BAP_LC3_UNICAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1", BT_BAP_LC3_UNICAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1", BT_BAP_LC3_UNICAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1", BT_BAP_LC3_UNICAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1", BT_BAP_LC3_UNICAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1", BT_BAP_LC3_UNICAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2", BT_BAP_LC3_UNICAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2", BT_BAP_LC3_UNICAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2", BT_BAP_LC3_UNICAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2", BT_BAP_LC3_UNICAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2", BT_BAP_LC3_UNICAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2", BT_BAP_LC3_UNICAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2", BT_BAP_LC3_UNICAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2", BT_BAP_LC3_UNICAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_BAP_LC3_UNICAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_BAP_LC3_UNICAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2", BT_BAP_LC3_UNICAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2", BT_BAP_LC3_UNICAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2", BT_BAP_LC3_UNICAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2", BT_BAP_LC3_UNICAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2", BT_BAP_LC3_UNICAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2", BT_BAP_LC3_UNICAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};

static const struct named_lc3_preset lc3_broadcast_presets[] = {
	{"8_1_1", BT_BAP_LC3_BROADCAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1", BT_BAP_LC3_BROADCAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1", BT_BAP_LC3_BROADCAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1", BT_BAP_LC3_BROADCAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1", BT_BAP_LC3_BROADCAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1", BT_BAP_LC3_BROADCAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1", BT_BAP_LC3_BROADCAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1", BT_BAP_LC3_BROADCAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_BAP_LC3_BROADCAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_BAP_LC3_BROADCAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1", BT_BAP_LC3_BROADCAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1", BT_BAP_LC3_BROADCAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1", BT_BAP_LC3_BROADCAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1", BT_BAP_LC3_BROADCAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1", BT_BAP_LC3_BROADCAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1", BT_BAP_LC3_BROADCAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2", BT_BAP_LC3_BROADCAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2", BT_BAP_LC3_BROADCAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2", BT_BAP_LC3_BROADCAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2", BT_BAP_LC3_BROADCAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2", BT_BAP_LC3_BROADCAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2", BT_BAP_LC3_BROADCAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2", BT_BAP_LC3_BROADCAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2", BT_BAP_LC3_BROADCAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_BAP_LC3_BROADCAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_BAP_LC3_BROADCAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2", BT_BAP_LC3_BROADCAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2", BT_BAP_LC3_BROADCAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2", BT_BAP_LC3_BROADCAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2", BT_BAP_LC3_BROADCAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2", BT_BAP_LC3_BROADCAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2", BT_BAP_LC3_BROADCAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};

static bool initialized;
static unsigned long bap_stats_interval = 1000U;

struct shell_stream *shell_stream_from_bap_stream(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream =
		CONTAINER_OF(bap_stream, struct bt_cap_stream, bap_stream);
	struct shell_stream *sh_stream = CONTAINER_OF(cap_stream, struct shell_stream, stream);

	return sh_stream;
}

struct bt_bap_stream *bap_stream_from_shell_stream(struct shell_stream *sh_stream)
{
	return &sh_stream->stream.bap_stream;
}

unsigned long bap_get_stats_interval(void)
{
	return bap_stats_interval;
}

void bap_foreach_stream(void (*func)(struct shell_stream *sh_stream, void *data), void *data)
{
#if defined(CONFIG_BT_BAP_UNICAST)
	for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
		func(&unicast_streams[i], data);
	}
#endif /* CONFIG_BT_BAP_UNICAST */

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		func(&broadcast_source_streams[i], data);
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		func(&broadcast_sink_streams[i], data);
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SINK */
}

#if defined(CONFIG_LIBLC3)
#include <lc3.h>

static int get_lc3_chan_alloc_from_index(const struct shell_stream *sh_stream, uint8_t index,
					 enum bt_audio_location *chan_alloc)
{
	const bool has_left = (sh_stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_LEFT) != 0;
	const bool has_right =
		(sh_stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_RIGHT) != 0;
	const bool is_mono = sh_stream->lc3_chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO;
	const bool is_left = index == 0 && has_left;
	const bool is_right = has_right && (index == 0U || (index == 1U && has_left));

	/* LC3 is always Left before Right, so we can use the index and the stream channel
	 * allocation to determine if index 0 is left or right.
	 */
	if (is_left) {
		*chan_alloc = BT_AUDIO_LOCATION_FRONT_LEFT;
	} else if (is_right) {
		*chan_alloc = BT_AUDIO_LOCATION_FRONT_RIGHT;
	} else if (is_mono) {
		*chan_alloc = BT_AUDIO_LOCATION_MONO_AUDIO;
	} else {
		/* Not suitable for USB */
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_LIBLC3 */

#if defined(CONFIG_BT_AUDIO_TX)
static size_t tx_streaming_cnt;

size_t bap_get_tx_streaming_cnt(void)
{
	return tx_streaming_cnt;
}

uint16_t get_next_seq_num(struct bt_bap_stream *bap_stream)
{
	struct shell_stream *sh_stream = shell_stream_from_bap_stream(bap_stream);
	const uint32_t interval_us = bap_stream->qos->interval;
	int64_t uptime_ticks;
	int64_t delta_ticks;
	uint64_t delta_us;
	uint16_t seq_num;

	if (!sh_stream->is_tx) {
		return 0;
	}

	/* Note: This does not handle wrapping of ticks when they go above 2^(62-1) */
	uptime_ticks = k_uptime_ticks();
	delta_ticks = uptime_ticks - sh_stream->tx.connected_at_ticks;

	delta_us = k_ticks_to_us_near64((uint64_t)delta_ticks);
	/* Calculate the sequence number by dividing the stream uptime by the SDU interval */
	seq_num = (uint16_t)(delta_us / interval_us);

	return seq_num;
}
#endif /* CONFIG_BT_AUDIO_TX */

#if defined(CONFIG_LIBLC3) && defined(CONFIG_BT_AUDIO_TX)
/* For the first call-back we push multiple audio frames to the buffer to use the
 * controller ISO buffer to handle jitter.
 */
#define PRIME_COUNT 2U
#define SINE_TX_POOL_SIZE (BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU))
NET_BUF_POOL_FIXED_DEFINE(sine_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT, SINE_TX_POOL_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#include "math.h"

#define AUDIO_VOLUME            (INT16_MAX - 3000) /* codec does clipping above INT16_MAX - 3000 */
#define AUDIO_TONE_FREQUENCY_HZ   400

static int16_t lc3_tx_buf[LC3_MAX_NUM_SAMPLES_MONO];

static int init_lc3_encoder(struct shell_stream *sh_stream)
{
	if (sh_stream == NULL) {
		bt_shell_error("NULL stream to init LC3");
		return -EINVAL;
	}

	if (!sh_stream->is_tx) {
		bt_shell_error("Invalid stream to init LC3 encoder");
		return -EINVAL;
	}

	if (sh_stream->tx.lc3_encoder != NULL) {
		bt_shell_error("Already initialized");
		return -EALREADY;
	}

	if (sh_stream->lc3_freq_hz == 0 || sh_stream->lc3_frame_duration_us == 0) {
		bt_shell_error("Invalid freq (%u) or frame duration (%u)",
			       sh_stream->lc3_freq_hz, sh_stream->lc3_frame_duration_us);

		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
		const size_t frame_size = bap_usb_get_frame_size(sh_stream);

		if (frame_size > sizeof(lc3_tx_buf)) {
			bt_shell_error("Cannot put %u octets in lc3_tx_buf of size %zu",
				       frame_size, sizeof(lc3_tx_buf));

			return -EINVAL;
		}
	}

	bt_shell_print(
		"Initializing LC3 encoder for BAP stream %p with %u us duration and %u Hz "
		"frequency",
		bap_stream_from_shell_stream(sh_stream), sh_stream->lc3_frame_duration_us,
		sh_stream->lc3_freq_hz);

	sh_stream->tx.lc3_encoder =
		lc3_setup_encoder(sh_stream->lc3_frame_duration_us, sh_stream->lc3_freq_hz,
				  IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ? USB_SAMPLE_RATE : 0,
				  &sh_stream->tx.lc3_encoder_mem);
	if (sh_stream->tx.lc3_encoder == NULL) {
		bt_shell_error("Failed to setup LC3 encoder - wrong parameters?\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * Use the math lib to generate a sine-wave using 16 bit samples into a buffer.
 *
 * @param buf Destination buffer
 * @param length_us Length of the buffer in microseconds
 * @param frequency_hz frequency in Hz
 * @param sample_rate_hz sample-rate in Hz.
 */
static void fill_lc3_tx_buf_sin(int16_t *buf, int length_us, int frequency_hz, int sample_rate_hz)
{
	const uint32_t sine_period_samples = sample_rate_hz / frequency_hz;
	const size_t num_samples = (length_us * sample_rate_hz) / USEC_PER_SEC;
	const float step = 2 * 3.1415 / sine_period_samples;

	for (size_t i = 0; i < num_samples; i++) {
		const float sample = sinf(i * step);

		buf[i] = (int16_t)(AUDIO_VOLUME * sample);
	}
}

static bool encode_frame(struct shell_stream *sh_stream, uint8_t index, size_t frame_cnt,
			 struct net_buf *out_buf)
{
	const size_t total_frames = sh_stream->lc3_chan_cnt * sh_stream->lc3_frame_blocks_per_sdu;
	const uint16_t octets_per_frame = sh_stream->lc3_octets_per_frame;
	int lc3_ret;

	if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
		enum bt_audio_location chan_alloc;
		int err;

		err = get_lc3_chan_alloc_from_index(sh_stream, index, &chan_alloc);
		if (err != 0) {
			/* Not suitable for USB */
			false;
		}

		/* TODO: Move the following to a function in bap_usb.c*/
		bap_usb_get_frame(sh_stream, chan_alloc, lc3_tx_buf);
	} else {
		/* Generate sine wave */
		fill_lc3_tx_buf_sin(lc3_tx_buf, sh_stream->lc3_frame_duration_us,
				    AUDIO_TONE_FREQUENCY_HZ, sh_stream->lc3_freq_hz);
	}

	if ((sh_stream->tx.encoded_cnt % bap_stats_interval) == 0) {
		bt_shell_print("[%zu]: Encoding frame of size %u (%u/%u)",
			       sh_stream->tx.encoded_cnt, octets_per_frame, frame_cnt + 1,
			       total_frames);
	}

	lc3_ret = lc3_encode(sh_stream->tx.lc3_encoder, LC3_PCM_FORMAT_S16, lc3_tx_buf, 1,
			     octets_per_frame, net_buf_tail(out_buf));
	if (lc3_ret == -1) {
		bt_shell_error("LC3 encoder failed - wrong parameters?: %d", lc3_ret);

		return false;
	}

	out_buf->len += octets_per_frame;

	return true;
}

static size_t encode_frame_block(struct shell_stream *sh_stream, size_t frame_cnt,
				 struct net_buf *out_buf)
{
	const uint8_t chan_cnt = sh_stream->lc3_chan_cnt;
	size_t encoded_frames = 0U;

	for (uint8_t i = 0U; i < chan_cnt; i++) {
		/* We provide the total number of decoded frames to `decode_frame` for logging
		 * purposes
		 */
		if (encode_frame(sh_stream, i, frame_cnt, out_buf)) {
			encoded_frames++;
		}
	}

	return encoded_frames;
}

static void do_lc3_encode(struct shell_stream *sh_stream, struct net_buf *out_buf)
{
	if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) && !bap_usb_can_get_full_sdu(sh_stream)) {
		/* No op - Will just send empty SDU */
	} else {
		size_t frame_cnt = 0U;

		for (uint8_t i = 0U; i < sh_stream->lc3_frame_blocks_per_sdu; i++) {
			frame_cnt += encode_frame_block(sh_stream, frame_cnt, out_buf);

			sh_stream->tx.encoded_cnt++;
		}
	}
}

static void lc3_audio_send_data(struct shell_stream *sh_stream)
{
	struct bt_bap_stream *bap_stream = bap_stream_from_shell_stream(sh_stream);
	const uint16_t tx_sdu_len = sh_stream->lc3_frame_blocks_per_sdu * sh_stream->lc3_chan_cnt *
				    sh_stream->lc3_octets_per_frame;
	struct net_buf *buf;
	int err;

	if (!sh_stream->is_tx || !sh_stream->tx.active) {
		/* TX has been aborted */
		return;
	}

	if (sh_stream->tx.lc3_encoder == NULL) {
		bt_shell_error("LC3 encoder not setup, cannot encode data");
		return;
	}

	if (bap_stream == NULL || bap_stream->qos == NULL) {
		bt_shell_error("invalid stream, aborting");
		return;
	}

	if (tx_sdu_len == 0U || tx_sdu_len > SINE_TX_POOL_SIZE) {
		bt_shell_error(
			"Cannot send %u length SDU (from frame blocks per sdu %u, channel "
			"count %u and %u octets per frame) for pool size %d",
			tx_sdu_len, sh_stream->lc3_frame_blocks_per_sdu,
			sh_stream->lc3_chan_cnt, sh_stream->lc3_octets_per_frame,
			SINE_TX_POOL_SIZE);
		return;
	}

	if (atomic_get(&sh_stream->tx.lc3_enqueue_cnt) == 0U) {
		/* no op */
		return;
	}

	buf = net_buf_alloc(&sine_tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	do_lc3_encode(sh_stream, buf);

	err = bt_bap_stream_send(bap_stream, buf, sh_stream->tx.seq_num);
	if (err < 0) {
		bt_shell_error("Failed to send LC3 audio data (%d)", err);
		net_buf_unref(buf);

		return;
	}

	if ((sh_stream->tx.lc3_sdu_cnt % bap_stats_interval) == 0U) {
		bt_shell_info("[%zu]: stream %p : TX LC3: %zu (seq_num %u)",
			      sh_stream->tx.lc3_sdu_cnt, bap_stream, tx_sdu_len,
			      sh_stream->tx.seq_num);
	}

	sh_stream->tx.lc3_sdu_cnt++;
	sh_stream->tx.seq_num++;
	atomic_dec(&sh_stream->tx.lc3_enqueue_cnt);
}

static void lc3_sent_cb(struct bt_bap_stream *bap_stream)
{
	struct shell_stream *sh_stream = shell_stream_from_bap_stream(bap_stream);

	if (!sh_stream->is_tx) {
		return;
	}

	atomic_inc(&sh_stream->tx.lc3_enqueue_cnt);
}

static void encode_and_send_cb(struct shell_stream *sh_stream, void *user_data)
{
	if (sh_stream->is_tx) {
		lc3_audio_send_data(sh_stream);
	}
}

static void lc3_encoder_thread_func(void *arg1, void *arg2, void *arg3)
{
	/* This will attempt to send on all TX streams.
	 * If there are no buffers available or the stream already have PRIME_COUNT outstanding SDUs
	 * the stream will not send anything.
	 *
	 * If USB is enabled it will attempt to read from buffered USB audio data.
	 * If there is no data available it will send empty SDUs
	 */
	while (true) {
		bap_foreach_stream(encode_and_send_cb, NULL);
		k_sleep(K_MSEC(1));
	}
}
#endif /* CONFIG_LIBLC3 && CONFIG_BT_AUDIO_TX */

const struct named_lc3_preset *bap_get_named_preset(bool is_unicast, enum bt_audio_dir dir,
						    const char *preset_arg)
{
	if (is_unicast) {
		for (size_t i = 0U; i < ARRAY_SIZE(lc3_unicast_presets); i++) {
			if (!strcmp(preset_arg, lc3_unicast_presets[i].name)) {
				return &lc3_unicast_presets[i];
			}
		}
	} else {
		for (size_t i = 0U; i < ARRAY_SIZE(lc3_broadcast_presets); i++) {
			if (!strcmp(preset_arg, lc3_broadcast_presets[i].name)) {
				return &lc3_broadcast_presets[i];
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_GMAP)) {
		return gmap_get_named_preset(is_unicast, dir, preset_arg);
	}

	return NULL;
}

#if defined(CONFIG_BT_PACS)
static const struct bt_audio_codec_cap lc3_codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_ANY, BT_AUDIO_CODEC_CAP_DURATION_ANY,
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2), 30, 240, MAX_CODEC_FRAMES_PER_SDU, CONTEXT);

#if defined(CONFIG_BT_PAC_SNK)
static struct bt_pacs_cap cap_sink = {
	.codec_cap = &lc3_codec_cap,
};
#endif /* CONFIG_BT_PAC_SNK */

#if defined(CONFIG_BT_PAC_SRC)
static struct bt_pacs_cap cap_source = {
	.codec_cap = &lc3_codec_cap,
};
#endif /* CONFIG_BT_PAC_SRC */
#endif /* CONFIG_BT_PACS */

#if defined(CONFIG_BT_BAP_UNICAST)
static void set_unicast_stream(struct bt_bap_stream *stream)
{
	default_stream = stream;

	for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
		if (stream == bap_stream_from_shell_stream(&unicast_streams[i])) {
			bt_shell_print("Default stream: %u", i + 1);
		}
	}
}

static int cmd_select_unicast(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_bap_stream *stream;
	unsigned long index;
	int err = 0;

	index = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse index: %d", err);

		return -ENOEXEC;
	}

	if (index > ARRAY_SIZE(unicast_streams)) {
		shell_error(sh, "Invalid index: %lu", index);

		return -ENOEXEC;
	}

	stream = bap_stream_from_shell_stream(&unicast_streams[index]);

	set_unicast_stream(stream);

	return 0;
}

#if defined(CONFIG_BT_BAP_UNICAST_SERVER)
static const struct bt_bap_qos_cfg_pref qos_pref =
	BT_BAP_QOS_CFG_PREF(true, BT_GAP_LE_PHY_2M, 0u, 60u, 10000u, 60000u, 10000u, 60000u);

static struct bt_bap_stream *stream_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		struct bt_bap_stream *stream = bap_stream_from_shell_stream(&unicast_streams[i]);

		if (!stream->conn) {
			return stream;
		}
	}

	return NULL;
}

static int lc3_config(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
		      const struct bt_audio_codec_cfg *codec_cfg, struct bt_bap_stream **stream,
		      struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("ASE Codec Config: conn %p ep %p dir %u", conn, ep, dir);

	print_codec_cfg(0, codec_cfg);

	*stream = stream_alloc();
	if (*stream == NULL) {
		bt_shell_print("No unicast_streams available");

		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);

		return -ENOMEM;
	}

	bt_shell_print("ASE Codec Config stream %p", *stream);

	set_unicast_stream(*stream);

	*pref = qos_pref;

	return 0;
}

static int lc3_reconfig(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			const struct bt_audio_codec_cfg *codec_cfg,
			struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("ASE Codec Reconfig: stream %p", stream);

	print_codec_cfg(0, codec_cfg);

	if (default_stream == NULL) {
		set_unicast_stream(stream);
	}

	*pref = qos_pref;

	return 0;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg *qos,
		   struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("QoS: stream %p %p", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
		      struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("Enable: stream %p meta_len %zu", stream, meta_len);

	return 0;
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("Start: stream %p", stream);

	return 0;
}

static bool meta_data_func_cb(struct bt_data *data, void *user_data)
{
	struct bt_bap_ascs_rsp *rsp = (struct bt_bap_ascs_rsp *)user_data;

	if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(data->type)) {
		printk("Invalid metadata type %u or length %u\n", data->type, data->data_len);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED, data->type);
		return false;
	}

	return true;
}

static int lc3_metadata(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
			struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("Metadata: stream %p meta_len %zu", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, meta_data_func_cb, rsp);
}

static int lc3_disable(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("Disable: stream %p", stream);

	return 0;
}

static int lc3_stop(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("Stop: stream %p", stream);

	return 0;
}

static int lc3_release(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	bt_shell_print("Release: stream %p", stream);

	if (stream == default_stream) {
		default_stream = NULL;
	}

	return 0;
}

static const struct bt_bap_unicast_server_cb unicast_server_cb = {
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
#endif /* CONFIG_BT_BAP_UNICAST_SERVER */

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

static int set_metadata(struct bt_audio_codec_cfg *codec_cfg, const char *meta_str)
{
	uint16_t context;

	context = strmeta(meta_str);
	if (context == 0) {
		return -ENOEXEC;
	}

	/* TODO: Check the type and only overwrite the streaming context */
	sys_put_le16(context, codec_cfg->meta);

	return 0;
}

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
int bap_ac_create_unicast_group(const struct bap_unicast_ac_param *param,
				struct shell_stream *snk_uni_streams[], size_t snk_cnt,
				struct shell_stream *src_uni_streams[], size_t src_cnt)
{
	struct bt_bap_unicast_group_stream_param snk_group_stream_params[BAP_UNICAST_AC_MAX_SNK] = {
		0};
	struct bt_bap_unicast_group_stream_param src_group_stream_params[BAP_UNICAST_AC_MAX_SRC] = {
		0};
	struct bt_bap_unicast_group_stream_pair_param pair_params[BAP_UNICAST_AC_MAX_PAIR] = {0};
	struct bt_bap_unicast_group_param group_param = {0};
	struct bt_bap_qos_cfg *snk_qos[BAP_UNICAST_AC_MAX_SNK];
	struct bt_bap_qos_cfg *src_qos[BAP_UNICAST_AC_MAX_SRC];
	size_t snk_stream_cnt = 0U;
	size_t src_stream_cnt = 0U;
	size_t pair_cnt = 0U;

	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_qos[i] = &snk_uni_streams[i]->qos;
	}

	for (size_t i = 0U; i < src_cnt; i++) {
		src_qos[i] = &src_uni_streams[i]->qos;
	}

	/* Create Group
	 *
	 * First setup the individual stream parameters and then match them in pairs by connection
	 * and direction
	 */
	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_group_stream_params[i].qos = snk_qos[i];
		snk_group_stream_params[i].stream =
			bap_stream_from_shell_stream(snk_uni_streams[i]);
	}
	for (size_t i = 0U; i < src_cnt; i++) {
		src_group_stream_params[i].qos = src_qos[i];
		src_group_stream_params[i].stream =
			bap_stream_from_shell_stream(src_uni_streams[i]);
	}

	for (size_t i = 0U; i < param->conn_cnt; i++) {
		for (size_t j = 0; j < MAX(param->snk_cnt[i], param->src_cnt[i]); j++) {
			if (param->snk_cnt[i] > j) {
				pair_params[pair_cnt].tx_param =
					&snk_group_stream_params[snk_stream_cnt++];
			} else {
				pair_params[pair_cnt].tx_param = NULL;
			}

			if (param->src_cnt[i] > j) {
				pair_params[pair_cnt].rx_param =
					&src_group_stream_params[src_stream_cnt++];
			} else {
				pair_params[pair_cnt].rx_param = NULL;
			}

			pair_cnt++;
		}
	}

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params = pair_params;
	group_param.params_count = pair_cnt;

	return bt_bap_unicast_group_create(&group_param, &default_unicast_group);
}

static uint8_t stream_dir(const struct bt_bap_stream *stream)
{
	if (stream->conn) {
		uint8_t conn_index = bt_conn_index(stream->conn);

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
		for (size_t i = 0; i < ARRAY_SIZE(snks[conn_index]); i++) {
			const struct bt_bap_ep *snk_ep = snks[conn_index][i];

			if (snk_ep != NULL && stream->ep == snk_ep) {
				return BT_AUDIO_DIR_SINK;
			}
		}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
		for (size_t i = 0; i < ARRAY_SIZE(srcs[conn_index]); i++) {
			const struct bt_bap_ep *src_ep = srcs[conn_index][i];

			if (src_ep != NULL && stream->ep == src_ep) {
				return BT_AUDIO_DIR_SOURCE;
			}
		}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
	}

	__ASSERT(false, "Invalid stream");
	return 0;
}

static void print_remote_codec_cap(const struct bt_conn *conn,
				   const struct bt_audio_codec_cap *codec_cap,
				   enum bt_audio_dir dir)
{
	bt_shell_print("conn %p: codec_cap %p dir 0x%02x", conn, codec_cap, dir);

	print_codec_cap(0, codec_cap);
}

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
static void add_sink(const struct bt_conn *conn, struct bt_bap_ep *ep)
{
	const uint8_t conn_index = bt_conn_index(conn);

	for (size_t i = 0U; i < ARRAY_SIZE(snks[conn_index]); i++) {
		if (snks[conn_index][i] == NULL) {
			bt_shell_print("Conn: %p, Sink #%zu: ep %p", conn, i, ep);

			snks[conn_index][i] = ep;
			return;
		}
	}

	bt_shell_error("Could not add more sink endpoints");
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
static void add_source(const struct bt_conn *conn, struct bt_bap_ep *ep)
{
	const uint8_t conn_index = bt_conn_index(conn);

	for (size_t i = 0U; i < ARRAY_SIZE(srcs[conn_index]); i++) {
		if (srcs[conn_index][i] == NULL) {
			bt_shell_print("Conn: %p, Source #%zu: ep %p", conn, i, ep);

			srcs[conn_index][i] = ep;
			return;
		}
	}

	bt_shell_error("Could not add more sink endpoints");
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec_cap)
{
	print_remote_codec_cap(conn, codec_cap, dir);
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	if (dir == BT_AUDIO_DIR_SINK) {
		add_sink(conn, ep);
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	if (dir == BT_AUDIO_DIR_SOURCE) {
		add_source(conn, ep);
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0*/
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	bt_shell_print("Discover complete: err %d", err);
}

static void discover_all(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	/* Sinks discovery complete, now discover sources */
	if (dir == BT_AUDIO_DIR_SINK) {
		dir = BT_AUDIO_DIR_SOURCE;
		unicast_client_cbs.discover = discover_cb;

		err = bt_bap_unicast_client_discover(default_conn, dir);
		if (err) {
			bt_shell_error("bt_bap_unicast_client_discover err %d", err);
		}
	}
}

static void unicast_client_location_cb(struct bt_conn *conn,
				       enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	bt_shell_print("dir %u loc %X\n", dir, loc);
}

static void available_contexts_cb(struct bt_conn *conn,
				  enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	bt_shell_print("snk ctx %u src ctx %u\n", snk_ctx, src_ctx);
}

static void config_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		      enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p config operation rsp_code %u reason %u",
		       stream, rsp_code, reason);

	if (default_stream == NULL) {
		default_stream = stream;
	}
}

static void qos_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		   enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p qos operation rsp_code %u reason %u",
		       stream, rsp_code, reason);
}

static void enable_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		      enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p enable operation rsp_code %u reason %u",
		       stream, rsp_code, reason);
}

static void start_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		     enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p start operation rsp_code %u reason %u",
		       stream, rsp_code, reason);
}

static void stop_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		    enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p stop operation rsp_code %u reason %u",
		       stream, rsp_code, reason);
}

static void disable_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p disable operation rsp_code %u reason %u",
		       stream, rsp_code, reason);
}

static void metadata_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
			enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p metadata operation rsp_code %u reason %u",
		       stream, rsp_code, reason);
}

static void release_cb(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason)
{
	bt_shell_print("stream %p release operation rsp_code %u reason %u",
		       stream, rsp_code, reason);
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
	.config = config_cb,
	.qos = qos_cb,
	.enable = enable_cb,
	.start = start_cb,
	.stop = stop_cb,
	.disable = disable_cb,
	.metadata = metadata_cb,
	.release = release_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
};

static int cmd_discover(const struct shell *sh, size_t argc, char *argv[])
{
	static bool cbs_registered;
	enum bt_audio_dir dir;
	uint8_t conn_index;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!initialized) {
		shell_error(sh, "Not initialized");
		return -ENOEXEC;
	}

	if (!cbs_registered) {
		err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);

		if (err != 0) {
			shell_error(sh, "Failed to register unicast client callbacks: %d", err);
			return err;
		}

		cbs_registered = true;
	}

	unicast_client_cbs.discover = discover_all;
	dir = BT_AUDIO_DIR_SINK;

	if (argc > 1) {
		if (!strcmp(argv[1], "sink")) {
			unicast_client_cbs.discover = discover_cb;
		} else if (!strcmp(argv[1], "source")) {
			unicast_client_cbs.discover = discover_cb;
			dir = BT_AUDIO_DIR_SOURCE;
		} else {
			shell_error(sh, "Unsupported dir: %s", argv[1]);
			return -ENOEXEC;
		}
	}

	err = bt_bap_unicast_client_discover(default_conn, dir);
	if (err != 0) {
		return -ENOEXEC;
	}

	conn_index = bt_conn_index(default_conn);
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	memset(srcs[conn_index], 0, sizeof(srcs[conn_index]));
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	memset(snks[conn_index], 0, sizeof(snks[conn_index]));
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

	return 0;
}

static int cmd_config(const struct shell *sh, size_t argc, char *argv[])
{
	enum bt_audio_location location = BT_AUDIO_LOCATION_MONO_AUDIO;
	const struct named_lc3_preset *named_preset;
	struct shell_stream *uni_stream;
	struct bt_bap_stream *bap_stream;
	struct bt_bap_ep *ep = NULL;
	enum bt_audio_dir dir;
	unsigned long index;
	uint8_t conn_index;
	int err = 0;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}
	conn_index = bt_conn_index(default_conn);

	if (default_stream == NULL) {
		bap_stream = bap_stream_from_shell_stream(&unicast_streams[0]);
	} else {
		bap_stream = default_stream;
	}

	index = shell_strtoul(argv[2], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse index: %d", err);

		return -ENOEXEC;
	}

	if (index > ARRAY_SIZE(unicast_streams)) {
		shell_error(sh, "Invalid index: %lu", index);

		return -ENOEXEC;
	}

	if (false) {

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	} else if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
		ep = snks[conn_index][index];

		named_preset = &default_sink_preset;
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
		ep = srcs[conn_index][index];

		named_preset = &default_source_preset;
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	if (!ep) {
		shell_error(sh, "Unable to find endpoint");
		return -ENOEXEC;
	}

	for (size_t i = 3U; i < argc; i++) {
		const char *arg = argv[i];

		/* argc needs to be larger than `i` to parse the argument value */
		if (argc <= i) {
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}

		if (strcmp(arg, "loc") == 0) {
			unsigned long loc_bits;

			arg = argv[++i];
			loc_bits = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Could not parse loc_bits: %d", err);

				return -ENOEXEC;
			}

			if (loc_bits > BT_AUDIO_LOCATION_ANY) {
				shell_error(sh, "Invalid loc_bits: %lu", loc_bits);

				return -ENOEXEC;
			}

			location = (enum bt_audio_location)loc_bits;
		} else if (strcmp(arg, "preset") == 0) {
			if (argc > i) {
				arg = argv[++i];

				named_preset = bap_get_named_preset(true, dir, arg);
				if (named_preset == NULL) {
					shell_error(sh, "Unable to parse named_preset %s", arg);
					return -ENOEXEC;
				}
			} else {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}
		} else {
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}
	}

	uni_stream = shell_stream_from_bap_stream(bap_stream);
	copy_unicast_stream_preset(uni_stream, named_preset);

	/* If location has been modified, we update the location in the codec configuration */
	struct bt_audio_codec_cfg *codec_cfg = &uni_stream->codec_cfg;

	for (size_t i = 0U; i < codec_cfg->data_len;) {
		const uint8_t len = codec_cfg->data[i++];
		uint8_t *value;
		uint8_t data_len;
		uint8_t type;

		if (len == 0 || len > codec_cfg->data_len - i) {
			/* Invalid len field */
			return false;
		}

		type = codec_cfg->data[i++];
		value = &codec_cfg->data[i];

		if (type == BT_AUDIO_CODEC_CFG_CHAN_ALLOC) {
			const uint32_t loc_32 = location;

			sys_put_le32(loc_32, value);

			shell_print(sh, "Setting location to 0x%08X", location);
			break;
		}

		data_len = len - sizeof(type);

		/* Since we are incrementing i by the value_len, we don't need to increment
		 * it further in the `for` statement
		 */
		i += data_len;
	}

	if (bap_stream->ep == ep) {
		err = bt_bap_stream_reconfig(bap_stream, &uni_stream->codec_cfg);
		if (err != 0) {
			shell_error(sh, "Unable reconfig stream: %d", err);
			return -ENOEXEC;
		}
	} else {
		err = bt_bap_stream_config(default_conn, bap_stream, ep,
					   &uni_stream->codec_cfg);
		if (err != 0) {
			shell_error(sh, "Unable to config stream: %d", err);
			return err;
		}
	}

	shell_print(sh, "ASE config: preset %s", named_preset->name);

	return 0;
}

static int cmd_stream_qos(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_bap_qos_cfg *qos;
	unsigned long interval;
	int err = 0;

	if (default_stream == NULL) {
		shell_print(sh, "No stream selected");
		return -ENOEXEC;
	}

	qos = default_stream->qos;

	if (qos == NULL) {
		shell_print(sh, "Stream not configured");
		return -ENOEXEC;
	}

	interval = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		return -ENOEXEC;
	}

	if (!IN_RANGE(interval, BT_ISO_SDU_INTERVAL_MIN, BT_ISO_SDU_INTERVAL_MAX)) {
		return -ENOEXEC;
	}

	qos->interval = interval;

	if (argc > 2) {
		unsigned long framing;

		framing = shell_strtoul(argv[2], 0, &err);
		if (err != 0) {
			return -ENOEXEC;
		}

		if (framing != BT_ISO_FRAMING_UNFRAMED && framing != BT_ISO_FRAMING_FRAMED) {
			return -ENOEXEC;
		}

		qos->framing = framing;
	}

	if (argc > 3) {
		unsigned long latency;

		latency = shell_strtoul(argv[3], 0, &err);
		if (err != 0) {
			return -ENOEXEC;
		}

		if (!IN_RANGE(latency, BT_ISO_LATENCY_MIN, BT_ISO_LATENCY_MAX)) {
			return -ENOEXEC;
		}

		qos->latency = latency;
	}

	if (argc > 4) {
		unsigned long pd;

		pd = shell_strtoul(argv[4], 0, &err);
		if (err != 0) {
			return -ENOEXEC;
		}

		if (pd > BT_AUDIO_PD_MAX) {
			return -ENOEXEC;
		}

		qos->pd = pd;
	}

	if (argc > 5) {
		unsigned long sdu;

		sdu = shell_strtoul(argv[5], 0, &err);
		if (err != 0) {
			return -ENOEXEC;
		}

		if (sdu > BT_ISO_MAX_SDU) {
			return -ENOEXEC;
		}

		qos->sdu = sdu;
	}

	if (argc > 6) {
		unsigned long phy;

		phy = shell_strtoul(argv[6], 0, &err);
		if (err != 0) {
			return -ENOEXEC;
		}

		if (phy != BT_GAP_LE_PHY_1M && phy != BT_GAP_LE_PHY_2M &&
		    phy != BT_GAP_LE_PHY_CODED) {
			return -ENOEXEC;
		}

		qos->phy = phy;
	}

	if (argc > 7) {
		unsigned long rtn;

		rtn = shell_strtoul(argv[7], 0, &err);
		if (err != 0) {
			return -ENOEXEC;
		}

		if (rtn > BT_ISO_CONNECTED_RTN_MAX) {
			return -ENOEXEC;
		}

		qos->rtn = rtn;
	}

	return 0;
}

static int set_group_param(
	const struct shell *sh, struct bt_bap_unicast_group_param *group_param,
	struct bt_bap_unicast_group_stream_pair_param pair_param[ARRAY_SIZE(unicast_streams)],
	struct bt_bap_unicast_group_stream_param stream_params[ARRAY_SIZE(unicast_streams)])
{
	size_t source_cnt = 0;
	size_t sink_cnt = 0;
	size_t cnt = 0;

	for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
		struct bt_bap_stream *stream = bap_stream_from_shell_stream(&unicast_streams[i]);
		struct shell_stream *uni_stream = &unicast_streams[i];

		if (stream->ep != NULL) {
			struct bt_bap_unicast_group_stream_param *stream_param;

			stream_param = &stream_params[cnt];

			stream_param->stream = stream;
			if (stream_dir(stream) == BT_AUDIO_DIR_SINK) {
				stream_param->qos = &uni_stream->qos;
				pair_param[sink_cnt++].tx_param = stream_param;
			} else {
				stream_param->qos = &uni_stream->qos;
				pair_param[source_cnt++].rx_param = stream_param;
			}

			cnt++;
		}
	}

	if (cnt == 0U) {
		shell_error(sh, "Stream cnt is 0");

		return -ENOEXEC;
	}

	group_param->packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param->params = pair_param;
	group_param->params_count = MAX(source_cnt, sink_cnt);

	return 0;
}

static int create_unicast_group(const struct shell *sh)
{
	struct bt_bap_unicast_group_stream_pair_param pair_param[ARRAY_SIZE(unicast_streams)] = {0};
	struct bt_bap_unicast_group_stream_param stream_params[ARRAY_SIZE(unicast_streams)] = {0};
	struct bt_bap_unicast_group_param group_param = {0};
	int err;

	err = set_group_param(sh, &group_param, pair_param, stream_params);
	if (err != 0) {
		return err;
	}

	err = bt_bap_unicast_group_create(&group_param, &default_unicast_group);
	if (err != 0) {
		shell_error(sh, "Unable to create default unicast group: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int reconfig_unicast_group(const struct shell *sh)
{
	struct bt_bap_unicast_group_stream_pair_param pair_param[ARRAY_SIZE(unicast_streams)] = {0};
	struct bt_bap_unicast_group_stream_param stream_params[ARRAY_SIZE(unicast_streams)] = {0};
	struct bt_bap_unicast_group_param group_param = {0};
	int err;

	err = set_group_param(sh, &group_param, pair_param, stream_params);
	if (err != 0) {
		return err;
	}

	err = bt_bap_unicast_group_reconfig(default_unicast_group, &group_param);
	if (err != 0) {
		shell_error(sh, "Unable to create default unicast group: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_qos(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_print(sh, "No stream selected");
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (default_unicast_group == NULL) {
		err = create_unicast_group(sh);
		if (err != 0) {
			return err;
		}
	} else {
		err = reconfig_unicast_group(sh);
		if (err != 0) {
			return err;
		}
	}

	err = bt_bap_stream_qos(default_conn, default_unicast_group);
	if (err) {
		shell_error(sh, "Unable to setup QoS: %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_enable(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_codec_cfg *codec_cfg;
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	codec_cfg = default_stream->codec_cfg;

	if (argc > 1) {
		err = set_metadata(codec_cfg, argv[1]);
		if (err != 0) {
			shell_error(sh, "Unable to handle metadata update: %d", err);
			return err;
		}
	}

	err = bt_bap_stream_enable(default_stream, codec_cfg->meta, codec_cfg->meta_len);
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

	err = bt_bap_stream_stop(default_stream);
	if (err) {
		shell_error(sh, "Unable to stop Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	err = bt_bap_stream_connect(default_stream);
	if (err) {
		shell_error(sh, "Unable to connect stream");
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

static int cmd_metadata(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_codec_cfg *codec_cfg;
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "No stream selected");
		return -ENOEXEC;
	}

	codec_cfg = default_stream->codec_cfg;

	if (argc > 1) {
		err = set_metadata(codec_cfg, argv[1]);
		if (err != 0) {
			shell_error(sh, "Unable to handle metadata update: %d", err);
			return err;
		}
	}

	err = bt_bap_stream_metadata(default_stream, codec_cfg->meta, codec_cfg->meta_len);
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

	err = bt_bap_stream_start(default_stream);
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

	err = bt_bap_stream_disable(default_stream);
	if (err) {
		shell_error(sh, "Unable to disable Channel");
		return -ENOEXEC;
	}

	return 0;
}

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
static void conn_list_eps(struct bt_conn *conn, void *data)
{
	const struct shell *sh = (const struct shell *)data;
	uint8_t conn_index = bt_conn_index(conn);

	shell_print(sh, "Conn: %p", conn);
	shell_print(sh, "  Sinks:");

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(snks[conn_index]); i++) {
		const struct bt_bap_ep *ep = snks[conn_index][i];

		if (ep != NULL) {
			struct bt_bap_ep_info ep_info;
			int err;

			err = bt_bap_ep_get_info(ep, &ep_info);
			if (err == 0) {
				shell_print(sh, "    #%u: ep %p (state: %d)", i, ep, ep_info.state);
			}
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	shell_print(sh, "  Sources:");

	for (size_t i = 0U; i < ARRAY_SIZE(srcs[conn_index]); i++) {
		const struct bt_bap_ep *ep = srcs[conn_index][i];

		if (ep != NULL) {
			struct bt_bap_ep_info ep_info;
			int err;

			err = bt_bap_ep_get_info(ep, &ep_info);
			if (err == 0) {
				shell_print(sh, "    #%u: ep %p (state: %d)", i, ep, ep_info.state);
			}
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
static int cmd_list(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "Configured Channels:");

	for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
		struct bt_bap_stream *stream = &unicast_streams[i].stream.bap_stream;

		if (stream != NULL && stream->conn != NULL) {
			shell_print(sh, "  %s#%u: stream %p dir 0x%02x group %p",
				    stream == default_stream ? "*" : " ", i, stream,
				    stream_dir(stream), stream->group);
		}
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, conn_list_eps, (void *)sh);

	return 0;
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

static int cmd_release(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_print(sh, "No stream selected");
		return -ENOEXEC;
	}

	err = bt_bap_stream_release(default_stream);
	if (err) {
		shell_error(sh, "Unable to release Channel");
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_BAP_UNICAST */

#if IS_BAP_INITIATOR
static ssize_t parse_config_data_args(const struct shell *sh, size_t argn, size_t argc,
				      char *argv[], struct bt_audio_codec_cfg *codec_cfg)
{
	for (; argn < argc; argn++) {
		const char *arg = argv[argn];
		unsigned long val;
		int err = 0;

		if (strcmp(arg, "freq") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse freq from %s: %d", arg, err);

				return -1;
			}

			if (val > UINT8_MAX) {
				shell_error(sh, "Invalid freq value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_set_freq(codec_cfg,
							  (enum bt_audio_codec_cfg_freq)val);
			if (err < 0) {
				shell_error(sh, "Failed to set freq with value %lu: %d", val, err);

				return -1;
			}
		} else if (strcmp(arg, "dur") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse dur from %s: %d", arg, err);

				return -1;
			}

			if (val > UINT8_MAX) {
				shell_error(sh, "Invalid dur value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_set_frame_dur(
				codec_cfg, (enum bt_audio_codec_cfg_frame_dur)val);
			if (err < 0) {
				shell_error(sh, "Failed to set dur with value %lu: %d", val, err);

				return -1;
			}
		} else if (strcmp(arg, "chan_alloc") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse chan alloc from %s: %d", arg, err);

				return -1;
			}

			if (val > UINT32_MAX) {
				shell_error(sh, "Invalid chan alloc value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_set_chan_allocation(codec_cfg,
								     (enum bt_audio_location)val);
			if (err < 0) {
				shell_error(sh, "Failed to set chan alloc with value %lu: %d", val,
					    err);

				return -1;
			}
		} else if (strcmp(arg, "frame_len") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to frame len from %s: %d", arg, err);

				return -1;
			}

			if (val > UINT16_MAX) {
				shell_error(sh, "Invalid frame len value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_set_octets_per_frame(codec_cfg, (uint16_t)val);
			if (err < 0) {
				shell_error(sh, "Failed to set frame len with value %lu: %d", val,
					    err);

				return -1;
			}
		} else if (strcmp(arg, "frame_blks") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse frame blks from %s: %d", arg, err);

				return -1;
			}

			if (val > UINT8_MAX) {
				shell_error(sh, "Invalid frame blks value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_set_frame_blocks_per_sdu(codec_cfg, (uint8_t)val);
			if (err < 0) {
				shell_error(sh, "Failed to set frame blks with value %lu: %d", val,
					    err);

				return -1;
			}
		} else { /* we are no longer parsing codec config values */
			/* Decrement to return taken argument */
			argn--;
			break;
		}
	}

	return argn;
}

static ssize_t parse_config_meta_args(const struct shell *sh, size_t argn, size_t argc,
				      char *argv[], struct bt_audio_codec_cfg *codec_cfg)
{
	for (; argn < argc; argn++) {
		const char *arg = argv[argn];
		unsigned long val;
		int err = 0;

		if (strcmp(arg, "pref_ctx") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse pref ctx from %s: %d", arg, err);

				return -1;
			}

			if (val > UINT16_MAX) {
				shell_error(sh, "Invalid pref ctx value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_pref_context(codec_cfg,
								       (enum bt_audio_context)val);
			if (err < 0) {
				shell_error(sh, "Failed to set pref ctx with value %lu: %d", val,
					    err);

				return -1;
			}
		} else if (strcmp(arg, "stream_ctx") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse stream ctx from %s: %d", arg, err);

				return -1;
			}

			if (val > UINT16_MAX) {
				shell_error(sh, "Invalid stream ctx value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_stream_context(
				codec_cfg, (enum bt_audio_context)val);
			if (err < 0) {
				shell_error(sh, "Failed to set stream ctx with value %lu: %d", val,
					    err);

				return -1;
			}
		} else if (strcmp(arg, "program_info") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			err = bt_audio_codec_cfg_meta_set_program_info(codec_cfg, arg, strlen(arg));
			if (err != 0) {
				shell_error(sh, "Failed to set program info with value %s: %d", arg,
					    err);

				return -1;
			}
		} else if (strcmp(arg, "lang") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			if (strlen(arg) != BT_AUDIO_LANG_SIZE) {
				shell_error(sh, "Failed to parse lang from %s", arg);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_lang(codec_cfg, arg);
			if (err < 0) {
				shell_error(sh, "Failed to set lang with value %s: %d", arg, err);

				return -1;
			}
		} else if (strcmp(arg, "ccid_list") == 0) {
			uint8_t ccid_list[CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE];
			size_t ccid_list_len;

			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			ccid_list_len = hex2bin(arg, strlen(arg), ccid_list, sizeof(ccid_list));
			if (ccid_list_len == 0) {
				shell_error(sh, "Failed to parse ccid list from %s", arg);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_ccid_list(codec_cfg, ccid_list,
								    ccid_list_len);
			if (err < 0) {
				shell_error(sh, "Failed to set ccid list with value %s: %d", arg,
					    err);

				return -1;
			}
		} else if (strcmp(arg, "parental_rating") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse parental rating from %s: %d", arg,
					    err);

				return -1;
			}

			if (val > UINT8_MAX) {
				shell_error(sh, "Invalid parental rating value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_parental_rating(
				codec_cfg, (enum bt_audio_parental_rating)val);
			if (err < 0) {
				shell_error(sh, "Failed to set parental rating with value %lu: %d",
					    val, err);

				return -1;
			}
		} else if (strcmp(arg, "program_info_uri") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			err = bt_audio_codec_cfg_meta_set_program_info_uri(codec_cfg, arg,
									   strlen(arg));
			if (err < 0) {
				shell_error(sh, "Failed to set program info URI with value %s: %d",
					    arg, err);

				return -1;
			}
		} else if (strcmp(arg, "audio_active_state") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Failed to parse audio active state from %s: %d",
					    arg, err);

				return -1;
			}

			if (val > UINT8_MAX) {
				shell_error(sh, "Invalid audio active state value: %lu", val);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_audio_active_state(
				codec_cfg, (enum bt_audio_active_state)val);
			if (err < 0) {
				shell_error(sh,
					    "Failed to set audio active state with value %lu: %d",
					    val, err);

				return -1;
			}
		} else if (strcmp(arg, "bcast_flag") == 0) {
			err = bt_audio_codec_cfg_meta_set_bcast_audio_immediate_rend_flag(
				codec_cfg);
			if (err < 0) {
				shell_error(sh, "Failed to set audio active state: %d", err);

				return -1;
			}
		} else if (strcmp(arg, "extended") == 0) {
			uint8_t extended[CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE];
			size_t extended_len;

			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			extended_len = hex2bin(arg, strlen(arg), extended, sizeof(extended));
			if (extended_len == 0) {
				shell_error(sh, "Failed to parse extended meta from %s", arg);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_extended(codec_cfg, extended,
								   extended_len);
			if (err < 0) {
				shell_error(sh, "Failed to set extended meta with value %s: %d",
					    arg, err);

				return -1;
			}
		} else if (strcmp(arg, "vendor") == 0) {
			uint8_t vendor[CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE];
			size_t vendor_len;

			if (++argn == argc) {
				shell_help(sh);

				return -1;
			}

			arg = argv[argn];

			vendor_len = hex2bin(arg, strlen(arg), vendor, sizeof(vendor));
			if (vendor_len == 0) {
				shell_error(sh, "Failed to parse vendor meta from %s", arg);

				return -1;
			}

			err = bt_audio_codec_cfg_meta_set_vendor(codec_cfg, vendor, vendor_len);
			if (err < 0) {
				shell_error(sh, "Failed to set vendor meta with value %s: %d", arg,
					    err);

				return -1;
			}
		} else { /* we are no longer parsing codec config meta values */
			/* Decrement to return taken argument */
			argn--;
			break;
		}
	}

	return argn;
}

static int cmd_preset(const struct shell *sh, size_t argc, char *argv[])
{
	const struct named_lc3_preset *named_preset;
	enum bt_audio_dir dir;
	bool unicast = true;

	if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
		named_preset = &default_sink_preset;
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
		named_preset = &default_source_preset;
	} else if (!strcmp(argv[1], "broadcast")) {
		unicast = false;
		dir = BT_AUDIO_DIR_SOURCE;

		named_preset = &default_broadcast_source_preset;
	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	if (argc > 2) {
		struct bt_audio_codec_cfg *codec_cfg;

		named_preset = bap_get_named_preset(unicast, dir, argv[2]);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s", argv[2]);
			return -ENOEXEC;
		}

		if (!strcmp(argv[1], "sink")) {
			named_preset = memcpy(&default_sink_preset, named_preset,
					      sizeof(default_sink_preset));
			codec_cfg = &default_sink_preset.preset.codec_cfg;
		} else if (!strcmp(argv[1], "source")) {
			named_preset = memcpy(&default_source_preset, named_preset,
					      sizeof(default_sink_preset));
			codec_cfg = &default_source_preset.preset.codec_cfg;
		} else if (!strcmp(argv[1], "broadcast")) {
			named_preset = memcpy(&default_broadcast_source_preset, named_preset,
					      sizeof(default_sink_preset));
			codec_cfg = &default_broadcast_source_preset.preset.codec_cfg;
		} else {
			shell_error(sh, "Invalid dir: %s", argv[1]);

			return -ENOEXEC;
		}

		if (argc > 3) {
			struct bt_audio_codec_cfg codec_cfg_backup;

			memcpy(&codec_cfg_backup, codec_cfg, sizeof(codec_cfg_backup));

			for (size_t argn = 3; argn < argc; argn++) {
				const char *arg = argv[argn];

				if (strcmp(arg, "config") == 0) {
					ssize_t ret;

					if (++argn == argc) {
						shell_help(sh);

						memcpy(codec_cfg, &codec_cfg_backup,
						       sizeof(codec_cfg_backup));

						return SHELL_CMD_HELP_PRINTED;
					}

					ret = parse_config_data_args(sh, argn, argc, argv,
								     codec_cfg);
					if (ret < 0) {
						memcpy(codec_cfg, &codec_cfg_backup,
						       sizeof(codec_cfg_backup));

						return -ENOEXEC;
					}

					argn = ret;
				} else if (strcmp(arg, "meta") == 0) {
					ssize_t ret;

					if (++argn == argc) {
						shell_help(sh);

						memcpy(codec_cfg, &codec_cfg_backup,
						       sizeof(codec_cfg_backup));

						return SHELL_CMD_HELP_PRINTED;
					}

					ret = parse_config_meta_args(sh, argn, argc, argv,
								     codec_cfg);
					if (ret < 0) {
						memcpy(codec_cfg, &codec_cfg_backup,
						       sizeof(codec_cfg_backup));

						return -ENOEXEC;
					}

					argn = ret;
				} else {
					shell_error(sh, "Invalid argument: %s", arg);
					shell_help(sh);

					return -ENOEXEC;
				}
			}
		}
	}

	shell_print(sh, "%s", named_preset->name);

	print_codec_cfg(0, &named_preset->preset.codec_cfg);
	print_qos(&named_preset->preset.qos);

	return 0;
}
#endif /* IS_BAP_INITIATOR */

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20 /* Set the timeout relative to interval */
#define PA_SYNC_SKIP         5

struct bt_broadcast_info {
	uint32_t broadcast_id;
	char broadcast_name[BT_AUDIO_BROADCAST_NAME_LEN_MAX + 1];
};

static struct broadcast_sink_auto_scan {
	struct broadcast_sink *broadcast_sink;
	struct bt_broadcast_info broadcast_info;
	struct bt_le_per_adv_sync **out_sync;
} auto_scan = {
	.broadcast_info = {
		.broadcast_id = BT_BAP_INVALID_BROADCAST_ID,
	},
};

static void clear_auto_scan(void)
{
	memset(&auto_scan, 0, sizeof(auto_scan));
	auto_scan.broadcast_info.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
}

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint32_t interval_us;
	uint32_t timeout;

	/* Add retries and convert to unit in 10's of ms */
	interval_us = BT_GAP_PER_ADV_INTERVAL_TO_US(interval);
	timeout =
		BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(interval_us) * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);

	return (uint16_t)timeout;
}

static bool scan_check_and_get_broadcast_values(struct bt_data *data, void *user_data)
{
	struct bt_broadcast_info *sr_info = (struct bt_broadcast_info *)user_data;
	struct bt_uuid_16 adv_uuid;

	switch (data->type) {
	case BT_DATA_SVC_DATA16:
		if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
			return true;
		}

		if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
			return true;
		}

		if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO) != 0) {
			return true;
		}

		sr_info->broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);
		return true;
	case BT_DATA_BROADCAST_NAME:
		if (!IN_RANGE(data->data_len, BT_AUDIO_BROADCAST_NAME_LEN_MIN,
			      BT_AUDIO_BROADCAST_NAME_LEN_MAX)) {
			return false;
		}

		memcpy(sr_info->broadcast_name, data->data, data->data_len);
		sr_info->broadcast_name[data->data_len] = '\0';
		return true;
	default:
		return true;
	}
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	struct bt_broadcast_info sr_info = {0};
	char addr_str[BT_ADDR_LE_STR_LEN];
	bool identified_broadcast = false;

	sr_info.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;

	if (!passes_scan_filter(info, ad)) {
		return;
	}

	bt_data_parse(ad, scan_check_and_get_broadcast_values, (void *)&sr_info);

	/* Verify that it is a BAP broadcaster*/
	if (sr_info.broadcast_id == BT_BAP_INVALID_BROADCAST_ID) {
		return;
	}

	bt_addr_le_to_str(info->addr, addr_str, sizeof(addr_str));

	bt_shell_print("Found broadcaster with ID 0x%06X (%s) and addr %s and sid 0x%02X ",
		       sr_info.broadcast_id, sr_info.broadcast_name, addr_str, info->sid);

	if ((auto_scan.broadcast_info.broadcast_id == BT_BAP_INVALID_BROADCAST_ID) &&
	    (strlen(auto_scan.broadcast_info.broadcast_name) == 0U)) {
		/* no op */
		return;
	}

	if (sr_info.broadcast_id == auto_scan.broadcast_info.broadcast_id) {
		identified_broadcast = true;
	} else if ((strlen(auto_scan.broadcast_info.broadcast_name) != 0U) &&
		   is_substring(auto_scan.broadcast_info.broadcast_name, sr_info.broadcast_name)) {
		auto_scan.broadcast_info.broadcast_id = sr_info.broadcast_id;
		identified_broadcast = true;

		bt_shell_print("Found matched broadcast name '%s' with address %s",
			       sr_info.broadcast_name, addr_str);
	}

	if (identified_broadcast && (auto_scan.broadcast_sink != NULL) &&
	    (auto_scan.broadcast_sink->pa_sync == NULL)) {
		struct bt_le_per_adv_sync_param create_params = {0};
		int err;

		err = bt_le_scan_stop();
		if (err != 0) {
			bt_shell_error("Could not stop scan: %d", err);
		}

		bt_addr_le_copy(&create_params.addr, info->addr);
		create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
		create_params.sid = info->sid;
		create_params.skip = PA_SYNC_SKIP;
		create_params.timeout = interval_to_sync_timeout(info->interval);

		bt_shell_print("Attempting to PA sync to the broadcaster");
		err = bt_le_per_adv_sync_create(&create_params, auto_scan.out_sync);
		if (err != 0) {
			bt_shell_error("Could not create Broadcast PA sync: %d", err);
		} else {
			auto_scan.broadcast_sink->pa_sync = *auto_scan.out_sync;
		}
	}
}

static void base_recv(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
		      size_t base_size)
{
	/* Don't print duplicates */
	if (base_size != default_broadcast_sink.base_size ||
	    memcmp(base, &default_broadcast_sink.received_base, base_size) != 0) {
		bt_shell_print("Received BASE from sink %p:", sink);
		(void)memcpy(&default_broadcast_sink.received_base, base, base_size);
		default_broadcast_sink.base_size = base_size;

		print_base(base);
	}
}

static void syncable(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	if (default_broadcast_sink.bap_sink == sink) {
		if (default_broadcast_sink.syncable) {
			return;
		}

		bt_shell_print("Sink %p is ready to sync %s encryption", sink,
			       biginfo->encryption ? "with" : "without");
		default_broadcast_sink.syncable = true;
	}
}

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (auto_scan.broadcast_sink != NULL && auto_scan.out_sync != NULL &&
	    sync == *auto_scan.out_sync) {
		bt_shell_print("PA synced to broadcast with broadcast ID 0x%06x",
			       auto_scan.broadcast_info.broadcast_id);

		if (auto_scan.broadcast_sink->bap_sink == NULL) {
			bt_shell_print("Attempting to create the sink");
			int err;

			err = bt_bap_broadcast_sink_create(sync,
							   auto_scan.broadcast_info.broadcast_id,
							   &auto_scan.broadcast_sink->bap_sink);
			if (err != 0) {
				bt_shell_error("Could not create broadcast sink: %d", err);
			}
		} else {
			bt_shell_print("Sink is already created");
		}
	}

	clear_auto_scan();
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (default_broadcast_sink.pa_sync == sync) {
		default_broadcast_sink.syncable = false;
		(void)memset(&default_broadcast_sink.received_base, 0,
			     sizeof(default_broadcast_sink.received_base));
	}

	clear_auto_scan();
}

static void broadcast_scan_timeout_cb(void)
{
	bt_shell_print("Scan timeout");

	clear_auto_scan();
}

static struct bt_bap_broadcast_sink_cb sink_cbs = {
	.base_recv = base_recv,
	.syncable = syncable,
};

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
};

static struct bt_le_scan_cb bap_scan_cb = {
	.timeout = broadcast_scan_timeout_cb,
	.recv = broadcast_scan_recv,
};
#endif /* CONFIG_BT_BAP_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_RX)
static size_t rx_streaming_cnt;

size_t bap_get_rx_streaming_cnt(void)
{
	return rx_streaming_cnt;
}

#if defined(CONFIG_LIBLC3)
struct lc3_data {
	void *fifo_reserved; /* 1st word reserved for use by FIFO */
	struct net_buf *buf;
	struct shell_stream *sh_stream;
	uint32_t ts;
	bool do_plc;
};

K_MEM_SLAB_DEFINE(lc3_data_slab, sizeof(struct lc3_data), CONFIG_BT_ISO_RX_BUF_COUNT,
		  __alignof__(struct lc3_data));

static int16_t lc3_rx_buf[LC3_MAX_NUM_SAMPLES_MONO];
static K_FIFO_DEFINE(lc3_in_fifo);

/* We only want to send USB to left/right from a single stream. If we have 2 left streams, the
 * outgoing audio is going to be terrible.
 * Since a stream can contain stereo data, both of these may be the same stream.
 */
static struct shell_stream *usb_left_stream;
static struct shell_stream *usb_right_stream;

static int init_lc3_decoder(struct shell_stream *sh_stream)
{
	if (sh_stream == NULL) {
		bt_shell_error("NULL stream to init LC3 decoder");
		return -EINVAL;
	}

	if (!sh_stream->is_rx) {
		bt_shell_error("Invalid stream to init LC3 decoder");
		return -EINVAL;
	}

	if (sh_stream->rx.lc3_decoder != NULL) {
		bt_shell_error("Already initialized");
		return -EALREADY;
	}

	if (sh_stream->lc3_freq_hz == 0 || sh_stream->lc3_frame_duration_us == 0) {
		bt_shell_error("Invalid freq (%u) or frame duration (%u)",
			       sh_stream->lc3_freq_hz, sh_stream->lc3_frame_duration_us);

		return -EINVAL;
	}

	bt_shell_print("Initializing the LC3 decoder with %u us duration and %u Hz frequency",
		       sh_stream->lc3_frame_duration_us, sh_stream->lc3_freq_hz);
	/* Create the decoder instance. This shall complete before stream_started() is called. */
	sh_stream->rx.lc3_decoder =
		lc3_setup_decoder(sh_stream->lc3_frame_duration_us, sh_stream->lc3_freq_hz,
				  IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ? USB_SAMPLE_RATE : 0,
				  &sh_stream->rx.lc3_decoder_mem);
	if (sh_stream->rx.lc3_decoder == NULL) {
		bt_shell_error("Failed to setup LC3 decoder - wrong parameters?\n");
		return -EINVAL;
	}

	return 0;
}

static bool decode_frame(struct lc3_data *data, size_t frame_cnt)
{
	const struct shell_stream *sh_stream = data->sh_stream;
	const size_t total_frames = sh_stream->lc3_chan_cnt * sh_stream->lc3_frame_blocks_per_sdu;
	const uint16_t octets_per_frame = sh_stream->lc3_octets_per_frame;
	struct net_buf *buf = data->buf;
	void *iso_data;
	int err;

	if (data->do_plc) {
		iso_data = NULL; /* perform PLC */

		if ((sh_stream->rx.decoded_cnt % bap_stats_interval) == 0) {
			bt_shell_print("[%zu]: Performing PLC", sh_stream->rx.decoded_cnt);
		}

		data->do_plc = false; /* clear flag */
	} else {
		iso_data = net_buf_pull_mem(data->buf, octets_per_frame);

		if ((sh_stream->rx.decoded_cnt % bap_stats_interval) == 0) {
			bt_shell_print("[%zu]: Decoding frame of size %u (%u/%u)",
				       sh_stream->rx.decoded_cnt, octets_per_frame, frame_cnt + 1,
				       total_frames);
		}
	}

	err = lc3_decode(sh_stream->rx.lc3_decoder, iso_data, octets_per_frame, LC3_PCM_FORMAT_S16,
			 lc3_rx_buf, 1);
	if (err < 0) {
		bt_shell_error("Failed to decode LC3 data (%u/%u - %u/%u)", frame_cnt + 1,
			       total_frames, octets_per_frame * frame_cnt, buf->len);
		return false;
	}

	return true;
}

static size_t decode_frame_block(struct lc3_data *data, size_t frame_cnt)
{
	const struct shell_stream *sh_stream = data->sh_stream;
	const uint8_t chan_cnt = sh_stream->lc3_chan_cnt;
	size_t decoded_frames = 0U;

	for (uint8_t i = 0U; i < chan_cnt; i++) {
		/* We provide the total number of decoded frames to `decode_frame` for logging
		 * purposes
		 */
		if (decode_frame(data, frame_cnt + decoded_frames)) {
			decoded_frames++;

			if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
				enum bt_audio_location chan_alloc;
				int err;

				err = get_lc3_chan_alloc_from_index(sh_stream, i, &chan_alloc);
				if (err != 0) {
					/* Not suitable for USB */
					continue;
				}

				/* We only want to left or right from one stream to USB */
				if ((chan_alloc == BT_AUDIO_LOCATION_FRONT_LEFT &&
				     sh_stream != usb_left_stream) ||
				    (chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT &&
				     sh_stream != usb_right_stream)) {
					continue;
				}

				err = bap_usb_add_frame_to_usb(chan_alloc, lc3_rx_buf,
							       sizeof(lc3_rx_buf), data->ts);
				if (err == -EINVAL) {
					continue;
				}
			}
		} else {
			/* If decoding failed, we clear the data to USB as it would contain
			 * invalid data
			 */
			if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
				bap_usb_clear_frames_to_usb();
			}

			break;
		}
	}

	return decoded_frames;
}

static void do_lc3_decode(struct lc3_data *data)
{
	struct shell_stream *sh_stream = data->sh_stream;

	if (sh_stream->is_rx && sh_stream->rx.lc3_decoder != NULL) {
		const uint8_t frame_blocks_per_sdu = sh_stream->lc3_frame_blocks_per_sdu;
		size_t frame_cnt;

		frame_cnt = 0;
		for (uint8_t i = 0U; i < frame_blocks_per_sdu; i++) {
			const size_t decoded_frames = decode_frame_block(data, frame_cnt);

			if (decoded_frames == 0) {
				break;
			}

			frame_cnt += decoded_frames;
		}

		sh_stream->rx.decoded_cnt++;
	}

	net_buf_unref(data->buf);
}

static void lc3_decoder_thread_func(void *arg1, void *arg2, void *arg3)
{
	while (true) {
		struct lc3_data *data = k_fifo_get(&lc3_in_fifo, K_FOREVER);
		struct shell_stream *sh_stream = data->sh_stream;

		if (sh_stream->is_rx && sh_stream->rx.lc3_decoder == NULL) {
			bt_shell_warn("Decoder is NULL, discarding data from FIFO");
			k_mem_slab_free(&lc3_data_slab, (void *)data);
			continue; /* Wait for new data */
		}

		do_lc3_decode(data);

		k_mem_slab_free(&lc3_data_slab, (void *)data);
	}
}

#endif /* CONFIG_LIBLC3*/

static void audio_recv(struct bt_bap_stream *stream,
		       const struct bt_iso_recv_info *info,
		       struct net_buf *buf)
{
	struct shell_stream *sh_stream = shell_stream_from_bap_stream(stream);

	if (!sh_stream->is_rx) {
		return;
	}

	sh_stream->rx.rx_cnt++;

	if (info->ts == sh_stream->rx.last_info.ts) {
		sh_stream->rx.dup_ts++;
	}

	if (info->seq_num == sh_stream->rx.last_info.seq_num) {
		sh_stream->rx.dup_psn++;
	}

	if ((info->flags & BT_ISO_FLAGS_VALID) != 0) {
		if (buf->len == 0U) {
			sh_stream->rx.empty_sdu_pkts++;
		} else {
			sh_stream->rx.valid_sdu_pkts++;
		}
	}

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		sh_stream->rx.err_pkts++;
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		sh_stream->rx.lost_pkts++;
	}

	if ((sh_stream->rx.rx_cnt % bap_stats_interval) == 0) {
		bt_shell_print(
			"[%zu]: Incoming audio on stream %p len %u ts %u seq_num %u flags %u "
			"(valid %zu, dup ts %zu, dup psn %zu, err_pkts %zu, lost_pkts %zu, "
			"empty SDUs %zu)",
			sh_stream->rx.rx_cnt, stream, buf->len, info->ts, info->seq_num,
			info->flags, sh_stream->rx.valid_sdu_pkts, sh_stream->rx.dup_ts,
			sh_stream->rx.dup_psn, sh_stream->rx.err_pkts, sh_stream->rx.lost_pkts,
			sh_stream->rx.empty_sdu_pkts);
	}

	(void)memcpy(&sh_stream->rx.last_info, info, sizeof(sh_stream->rx.last_info));

#if defined(CONFIG_LIBLC3)
	if (sh_stream->rx.lc3_decoder != NULL) {
		const uint8_t frame_blocks_per_sdu = sh_stream->lc3_frame_blocks_per_sdu;
		const uint16_t octets_per_frame = sh_stream->lc3_octets_per_frame;
		const uint8_t chan_cnt = sh_stream->lc3_chan_cnt;
		struct lc3_data *data;

		/* Allocate a context that holds both the buffer and the stream so that we can
		 * send both of these values to the LC3 decoder thread as a single struct
		 * in a FIFO
		 */
		if (k_mem_slab_alloc(&lc3_data_slab, (void **)&data, K_NO_WAIT)) {
			bt_shell_warn("Could not allocate LC3 data item");

			return;
		}

		if ((info->flags & BT_ISO_FLAGS_VALID) == 0) {
			data->do_plc = true;
		} else if (buf->len != (octets_per_frame * chan_cnt * frame_blocks_per_sdu)) {
			if (buf->len != 0U) {
				bt_shell_error(
					"Expected %u frame blocks with %u channels of size %u, but "
					"length is %u",
					frame_blocks_per_sdu, chan_cnt, octets_per_frame, buf->len);
			}

			data->do_plc = true;
		}

		data->buf = net_buf_ref(buf);
		data->sh_stream = sh_stream;
		if (info->flags & BT_ISO_FLAGS_TS) {
			data->ts = info->ts;
		} else {
			data->ts = 0U;
		}

		k_fifo_put(&lc3_in_fifo, data);
	}
#endif /* CONFIG_LIBLC3 */
}
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_BAP_UNICAST)
static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	bt_shell_print("Stream %p enabled", stream);

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER)) {
		struct bt_bap_ep_info ep_info;
		struct bt_conn_info conn_info;
		int err;

		err = bt_conn_get_info(stream->conn, &conn_info);
		if (err != 0) {
			bt_shell_error("Failed to get conn info: %d", err);
			return;
		}

		if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
			return; /* We also want to autonomously start the stream as the server */
		}

		err = bt_bap_ep_get_info(stream->ep, &ep_info);
		if (err != 0) {
			bt_shell_error("Failed to get ep info: %d", err);
			return;
		}

		if (ep_info.dir == BT_AUDIO_DIR_SINK) {
			/* Automatically do the receiver start ready operation */
			err = bt_bap_stream_start(stream);

			if (err != 0) {
				bt_shell_error("Failed to start stream: %d", err);
				return;
			}
		}
	}
}
#endif /* CONFIG_BT_BAP_UNICAST */

static void stream_started_cb(struct bt_bap_stream *bap_stream)
{
	struct shell_stream *sh_stream = shell_stream_from_bap_stream(bap_stream);
	struct bt_bap_ep_info info = {0};
	int ret;

	printk("Stream %p started\n", bap_stream);

	ret = bt_bap_ep_get_info(bap_stream->ep, &info);
	if (ret != 0) {
		bt_shell_error("Failed to get EP info: %d", ret);
		return;
	}

	sh_stream->is_rx = info.can_recv;
	sh_stream->is_tx = info.can_send;

#if defined(CONFIG_LIBLC3)
	const struct bt_audio_codec_cfg *codec_cfg = bap_stream->codec_cfg;

	if (codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		if (sh_stream->is_tx) {
			atomic_set(&sh_stream->tx.lc3_enqueue_cnt, PRIME_COUNT);
			sh_stream->tx.lc3_sdu_cnt = 0U;
		}

		ret = bt_audio_codec_cfg_get_freq(codec_cfg);
		if (ret >= 0) {
			ret = bt_audio_codec_cfg_freq_to_freq_hz(ret);

			if (ret > 0) {
				if (ret == 8000 || ret == 16000 || ret == 24000 || ret == 32000 ||
				    ret == 48000) {
					sh_stream->lc3_freq_hz = (uint32_t)ret;
				} else {
					bt_shell_error("Unsupported frequency for LC3: %d", ret);
					sh_stream->lc3_freq_hz = 0U;
				}
			} else {
				bt_shell_error("Invalid frequency: %d", ret);
				sh_stream->lc3_freq_hz = 0U;
			}
		} else {
			bt_shell_error("Could not get frequency: %d", ret);
			sh_stream->lc3_freq_hz = 0U;
		}

		ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
		if (ret >= 0) {
			ret = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
			if (ret > 0) {
				sh_stream->lc3_frame_duration_us = (uint32_t)ret;
			} else {
				bt_shell_error("Invalid frame duration: %d", ret);
				sh_stream->lc3_frame_duration_us = 0U;
			}
		} else {
			bt_shell_error("Could not get frame duration: %d", ret);
			sh_stream->lc3_frame_duration_us = 0U;
		}

		ret = bt_audio_codec_cfg_get_chan_allocation(
			codec_cfg, &sh_stream->lc3_chan_allocation, false);
		if (ret == 0) {
			sh_stream->lc3_chan_cnt =
				bt_audio_get_chan_count(sh_stream->lc3_chan_allocation);
		} else {
			bt_shell_error("Could not get channel allocation: %d", ret);
			sh_stream->lc3_chan_allocation = BT_AUDIO_LOCATION_MONO_AUDIO;
			sh_stream->lc3_chan_cnt = 1U;
		}

		ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);
		if (ret >= 0) {
			sh_stream->lc3_frame_blocks_per_sdu = (uint8_t)ret;
		} else {
			bt_shell_error("Could not get frame blocks per SDU: %d", ret);
			sh_stream->lc3_frame_blocks_per_sdu = 0U;
		}

		ret = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
		if (ret >= 0) {
			sh_stream->lc3_octets_per_frame = (uint16_t)ret;
		} else {
			bt_shell_error("Could not get octets per frame: %d", ret);
			sh_stream->lc3_octets_per_frame = 0U;
		}

#if defined(CONFIG_BT_AUDIO_TX)
		if (sh_stream->is_tx && sh_stream->tx.lc3_encoder == NULL) {
			const int err = init_lc3_encoder(sh_stream);

			if (err != 0) {
				bt_shell_error("Failed to init LC3 encoder: %d", err);

				return;
			}

			if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
				/* Always mark as active when using USB */
				sh_stream->tx.active = true;
			}
		}
#endif /* CONFIG_BT_AUDIO_TX */

#if defined(CONFIG_BT_AUDIO_RX)
		if (sh_stream->is_rx) {
			if (sh_stream->rx.lc3_decoder == NULL) {
				const int err = init_lc3_decoder(sh_stream);

				if (err != 0) {
					bt_shell_error("Failed to init LC3 decoder: %d", err);

					return;
				}
			}

			sh_stream->rx.decoded_cnt = 0U;

			if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
				if ((sh_stream->lc3_chan_allocation &
				     BT_AUDIO_LOCATION_FRONT_LEFT) != 0) {
					if (usb_left_stream == NULL) {
						bt_shell_info("Setting USB left stream to %p",
							      sh_stream);
						usb_left_stream = sh_stream;
					} else {
						bt_shell_warn("Multiple left streams started");
					}
				}

				if ((sh_stream->lc3_chan_allocation &
				     BT_AUDIO_LOCATION_FRONT_RIGHT) != 0) {
					if (usb_right_stream == NULL) {
						bt_shell_info("Setting USB right stream to %p",
							      sh_stream);
						usb_right_stream = sh_stream;
					} else {
						bt_shell_warn("Multiple right streams started");
					}
				}
			}
		}
#endif /* CONFIG_BT_AUDIO_RX */
	}
#endif /* CONFIG_LIBLC3 */

#if defined(CONFIG_BT_AUDIO_TX)
	if (sh_stream->is_tx) {
		sh_stream->tx.connected_at_ticks = k_uptime_ticks();
	}
#endif /* CONFIG_BT_AUDIO_TX */

#if defined(CONFIG_BT_AUDIO_RX)
	if (sh_stream->is_rx) {
		sh_stream->rx.empty_sdu_pkts = 0U;
		sh_stream->rx.valid_sdu_pkts = 0U;
		sh_stream->rx.lost_pkts = 0U;
		sh_stream->rx.err_pkts = 0U;
		sh_stream->rx.dup_psn = 0U;
		sh_stream->rx.rx_cnt = 0U;
		sh_stream->rx.dup_ts = 0U;

		rx_streaming_cnt++;
	}
#endif
}

#if defined(CONFIG_LIBLC3)
static void update_usb_streams_cb(struct shell_stream *sh_stream, void *user_data)
{
	if (sh_stream->is_rx) {
		if (usb_left_stream == NULL &&
		    (sh_stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_LEFT) != 0) {
			bt_shell_info("Setting new USB left stream to %p", sh_stream);
			usb_left_stream = sh_stream;
		}

		if (usb_right_stream == NULL &&
		    (sh_stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_RIGHT) != 0) {
			bt_shell_info("Setting new USB right stream to %p", sh_stream);
			usb_right_stream = sh_stream;
		}
	}
}

static void update_usb_streams(struct shell_stream *sh_stream)
{
	if (sh_stream->is_rx) {
		if (sh_stream == usb_left_stream) {
			bt_shell_info("Clearing USB left stream (%p)", usb_left_stream);
			usb_left_stream = NULL;
		}

		if (sh_stream == usb_right_stream) {
			bt_shell_info("Clearing USB right stream (%p)", usb_right_stream);
			usb_right_stream = NULL;
		}

		bap_foreach_stream(update_usb_streams_cb, NULL);
	}
}
#endif /* CONFIG_LIBLC3 */

static void clear_stream_data(struct shell_stream *sh_stream)
{
#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
	if (IS_ARRAY_ELEMENT(broadcast_sink_streams, sh_stream)) {
		if (default_broadcast_sink.stream_cnt != 0) {
			default_broadcast_sink.stream_cnt--;
		}

		if (default_broadcast_sink.stream_cnt == 0) {
			/* All streams in the broadcast sink has been terminated */
			memset(&default_broadcast_sink.received_base, 0,
			       sizeof(default_broadcast_sink.received_base));
			default_broadcast_sink.broadcast_id = 0;
			default_broadcast_sink.syncable = false;
		}
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_RX)
	if (sh_stream->is_rx) {
		rx_streaming_cnt--;
		memset(&sh_stream->rx, 0, sizeof(sh_stream->rx));
	}
#endif

#if defined(CONFIG_BT_AUDIO_TX)
	if (sh_stream->is_tx) {
		memset(&sh_stream->tx, 0, sizeof(sh_stream->tx));
	}
#endif

	sh_stream->is_rx = sh_stream->is_tx = false;

#if defined(CONFIG_LIBLC3)
	if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS)) {
		update_usb_streams(sh_stream);
	}
#endif /* CONFIG_LIBLC3 */
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	struct shell_stream *sh_stream = shell_stream_from_bap_stream(stream);

	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);

	clear_stream_data(sh_stream);
}

#if defined(CONFIG_BT_BAP_UNICAST)
static void stream_configured_cb(struct bt_bap_stream *stream,
				 const struct bt_bap_qos_cfg_pref *pref)
{
	bt_shell_print("Stream %p configured\n", stream);
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	struct shell_stream *sh_stream = shell_stream_from_bap_stream(stream);

	bt_shell_print("Stream %p released\n", stream);

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	if (default_unicast_group != NULL) {
		bool group_can_be_deleted = true;

		for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
			const struct bt_bap_stream *bap_stream =
				bap_stream_from_shell_stream(&unicast_streams[i]);

			if (bap_stream->ep != NULL) {
				struct bt_bap_ep_info ep_info;
				int err;

				err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
				if (err == 0 && ep_info.state != BT_BAP_EP_STATE_CODEC_CONFIGURED &&
				    ep_info.state != BT_BAP_EP_STATE_IDLE) {
					group_can_be_deleted = false;
					break;
				}
			}
		}

		if (group_can_be_deleted) {
			int err;

			bt_shell_print("All streams released, deleting group\n");

			err = bt_bap_unicast_group_delete(default_unicast_group);

			if (err != 0) {
				bt_shell_error("Failed to delete unicast group: %d", err);
			} else {
				default_unicast_group = NULL;
			}
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

	clear_stream_data(sh_stream);
}
#endif /* CONFIG_BT_BAP_UNICAST */

static struct bt_bap_stream_ops stream_ops = {
#if defined(CONFIG_BT_AUDIO_RX)
	.recv = audio_recv,
#endif /* CONFIG_BT_AUDIO_RX */
#if defined(CONFIG_BT_BAP_UNICAST)
	.configured = stream_configured_cb,
	.released = stream_released_cb,
	.enabled = stream_enabled_cb,
#endif /* CONFIG_BT_BAP_UNICAST */
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
#if defined(CONFIG_LIBLC3) && defined(CONFIG_BT_AUDIO_TX)
	.sent = lc3_sent_cb,
#endif
};

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
static int cmd_select_broadcast_source(const struct shell *sh, size_t argc,
				       char *argv[])
{
	unsigned long index;
	int err = 0;

	index = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse index: %d", err);

		return -ENOEXEC;
	}

	if (index > ARRAY_SIZE(broadcast_source_streams)) {
		shell_error(sh, "Invalid index: %lu", index);

		return -ENOEXEC;
	}

	default_stream = bap_stream_from_shell_stream(&broadcast_source_streams[index]);

	return 0;
}

static int cmd_create_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	struct bt_bap_broadcast_source_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_bap_broadcast_source_subgroup_param subgroup_param;
	struct bt_bap_broadcast_source_param create_param = {0};
	const struct named_lc3_preset *named_preset;
	uint32_t broadcast_id = 0U;
	int err;

	if (default_source.bap_source != NULL) {
		shell_info(sh, "Broadcast source already created");
		return -ENOEXEC;
	}

	named_preset = &default_broadcast_source_preset;

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

				named_preset = bap_get_named_preset(false, BT_AUDIO_DIR_SOURCE,
								    arg);
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

	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err != 0) {
		bt_shell_error("Unable to generate broadcast ID: %d\n", err);

		return -ENOEXEC;
	}

	shell_print(sh, "Generated broadcast_id 0x%06X", broadcast_id);

	copy_broadcast_source_preset(&default_source, named_preset);

	(void)memset(stream_params, 0, sizeof(stream_params));
	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream =
			bap_stream_from_shell_stream(&broadcast_source_streams[i]);
	}
	subgroup_param.params_count = ARRAY_SIZE(stream_params);
	subgroup_param.params = stream_params;
	subgroup_param.codec_cfg = &default_source.codec_cfg;
	create_param.params_count = 1U;
	create_param.params = &subgroup_param;
	create_param.qos = &default_source.qos;

	err = bt_bap_broadcast_source_create(&create_param, &default_source.bap_source);
	if (err != 0) {
		shell_error(sh, "Unable to create broadcast source: %d", err);

		default_source.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
		return err;
	}

	shell_print(sh, "Broadcast source created: preset %s",
		    named_preset->name);
	default_source.broadcast_id = broadcast_id;

	if (default_stream == NULL) {
		default_stream = bap_stream_from_shell_stream(&broadcast_source_streams[0]);
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

	if (default_source.bap_source == NULL || default_source.is_cap) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_source_start(default_source.bap_source, adv_sets[selected_adv]);
	if (err != 0) {
		shell_error(sh, "Unable to start broadcast source: %d", err);
		return err;
	}

	return 0;
}

static int cmd_stop_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_source.bap_source == NULL || default_source.is_cap) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_source_stop(default_source.bap_source);
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

	if (default_source.bap_source == NULL || default_source.is_cap) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_source_delete(default_source.bap_source);
	if (err != 0) {
		shell_error(sh, "Unable to delete broadcast source: %d", err);
		return err;
	}
	default_source.bap_source = NULL;

	return 0;
}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
static int cmd_create_broadcast_sink(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_per_adv_sync *per_adv_sync = per_adv_syncs[selected_per_adv_sync];
	unsigned long broadcast_id;
	int err = 0;

	broadcast_id = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse broadcast_id: %d", err);

		return -ENOEXEC;
	}

	if (broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		shell_error(sh, "Invalid broadcast_id: %lu", broadcast_id);

		return -ENOEXEC;
	}

	if (per_adv_sync == NULL) {
		const struct bt_le_scan_param param = {
			.type = BT_LE_SCAN_TYPE_PASSIVE,
			.options = BT_LE_SCAN_OPT_NONE,
			.interval = BT_GAP_SCAN_FAST_INTERVAL,
			.window = BT_GAP_SCAN_FAST_WINDOW,
			.timeout = 1000, /* 10ms units -> 10 second timeout */
		};

		shell_print(sh, "No PA sync available, starting scanning for broadcast_id");

		err = bt_le_scan_start(&param, NULL);
		if (err) {
			shell_print(sh, "Fail to start scanning: %d", err);

			return -ENOEXEC;
		}

		auto_scan.broadcast_sink = &default_broadcast_sink;
		auto_scan.broadcast_info.broadcast_id = broadcast_id;
		auto_scan.out_sync = &per_adv_syncs[selected_per_adv_sync];
	} else {
		shell_print(sh, "Creating broadcast sink with broadcast ID 0x%06X",
			    (uint32_t)broadcast_id);

		err = bt_bap_broadcast_sink_create(per_adv_sync, (uint32_t)broadcast_id,
						   &default_broadcast_sink.bap_sink);

		if (err != 0) {
			shell_error(sh, "Failed to create broadcast sink: %d", err);

			return -ENOEXEC;
		}
	}

	return 0;
}

static int cmd_create_sink_by_name(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_le_scan_param param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
		.timeout = 1000, /* 10ms units -> 10 second timeout */
	};
	char *broadcast_name;
	int err = 0;

	broadcast_name = argv[1];
	if (!IN_RANGE(strlen(broadcast_name), BT_AUDIO_BROADCAST_NAME_LEN_MIN,
		      BT_AUDIO_BROADCAST_NAME_LEN_MAX)) {
		shell_error(sh, "Broadcast name should be minimum %d and maximum %d characters",
			    BT_AUDIO_BROADCAST_NAME_LEN_MIN, BT_AUDIO_BROADCAST_NAME_LEN_MAX);

		return -ENOEXEC;
	}

	shell_print(sh, "Starting scanning for broadcast_name");

	err = bt_le_scan_start(&param, NULL);
	if (err) {
		shell_print(sh, "Fail to start scanning: %d", err);

		return -ENOEXEC;
	}

	memcpy(auto_scan.broadcast_info.broadcast_name, broadcast_name, strlen(broadcast_name));
	auto_scan.broadcast_info.broadcast_name[strlen(broadcast_name)] = '\0';

	auto_scan.broadcast_info.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
	auto_scan.broadcast_sink = &default_broadcast_sink;
	auto_scan.out_sync = &per_adv_syncs[selected_per_adv_sync];

	return 0;
}

static int cmd_sync_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_bap_stream *streams[ARRAY_SIZE(broadcast_sink_streams)];
	uint8_t bcode[BT_ISO_BROADCAST_CODE_SIZE] = {0};
	bool bcode_set = false;
	uint32_t bis_bitfield;
	size_t stream_cnt;
	int err = 0;

	bis_bitfield = 0;
	stream_cnt = 0U;
	for (size_t argn = 1U; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(argv[argn], "bcode") == 0) {
			size_t len;

			if (++argn == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			arg = argv[argn];

			len = hex2bin(arg, strlen(arg), bcode, sizeof(bcode));
			if (len == 0) {
				shell_print(sh, "Invalid broadcast code: %s", arg);

				return -ENOEXEC;
			}

			bcode_set = true;
		} else if (strcmp(argv[argn], "bcode_str") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			arg = argv[argn];

			if (strlen(arg) == 0U || strlen(arg) > sizeof(bcode)) {
				shell_print(sh, "Invalid broadcast code: %s", arg);

				return -ENOEXEC;
			}

			memcpy(bcode, arg, strlen(arg));
			bcode_set = true;
		} else {
			unsigned long val;

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Could not parse BIS index val: %d", err);

				return -ENOEXEC;
			}

			if (!IN_RANGE(val, BT_ISO_BIS_INDEX_MIN, BT_ISO_BIS_INDEX_MAX)) {
				shell_error(sh, "Invalid index: %lu", val);

				return -ENOEXEC;
			}

			bis_bitfield |= BT_ISO_BIS_INDEX_BIT(val);
			stream_cnt++;
		}
	}

	if (default_broadcast_sink.bap_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	(void)memset(streams, 0, sizeof(streams));
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i] = bap_stream_from_shell_stream(&broadcast_sink_streams[i]);
	}

	err = bt_bap_broadcast_sink_sync(default_broadcast_sink.bap_sink, bis_bitfield, streams,
					 bcode_set ? bcode : NULL);
	if (err != 0) {
		shell_error(sh, "Failed to sync to broadcast: %d", err);
		return err;
	}

	default_broadcast_sink.stream_cnt = stream_cnt;

	return 0;
}

static int cmd_stop_broadcast_sink(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int err;

	if (default_broadcast_sink.bap_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_sink_stop(default_broadcast_sink.bap_sink);
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

	if (default_broadcast_sink.bap_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_sink_delete(default_broadcast_sink.bap_sink);
	if (err != 0) {
		shell_error(sh, "Failed to term sink: %d", err);
		return err;
	}

	default_broadcast_sink.bap_sink = NULL;
	default_broadcast_sink.syncable = false;

	return err;
}
#endif /* CONFIG_BT_BAP_BROADCAST_SINK */

static int cmd_set_loc(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_audio_dir dir;
	enum bt_audio_location loc;
	unsigned long loc_val;

	if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	loc_val = shell_strtoul(argv[2], 16, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse loc_val: %d", err);

		return -ENOEXEC;
	}

	if (loc_val > BT_AUDIO_LOCATION_ANY) {
		shell_error(sh, "Invalid location: %lu", loc_val);

		return -ENOEXEC;
	}

	loc = loc_val;

	err = bt_pacs_set_location(dir, loc);
	if (err) {
		shell_error(sh, "Set available contexts err %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_context(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_audio_dir dir;
	enum bt_audio_context ctx;
	unsigned long ctx_val;

	if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	ctx_val = shell_strtoul(argv[2], 16, &err);
	if (err) {
		shell_error(sh, "Could not parse context: %d", err);

		return err;
	}

	if (ctx_val > BT_AUDIO_CONTEXT_TYPE_ANY) {
		shell_error(sh, "Invalid context: %lu", ctx_val);

		return -ENOEXEC;
	}

	ctx = ctx_val;

	if (!strcmp(argv[3], "supported")) {
		if (ctx_val == BT_AUDIO_CONTEXT_TYPE_NONE) {
			shell_error(sh, "Invalid context: %lu", ctx_val);

			return -ENOEXEC;
		}

		err = bt_pacs_set_supported_contexts(dir, ctx);
		if (err) {
			shell_error(sh, "Set supported contexts err %d", err);
			return err;
		}
	} else if (!strcmp(argv[3], "available")) {
		err = bt_pacs_set_available_contexts(dir, ctx);
		if (err) {
			shell_error(sh, "Set available contexts err %d", err);
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

	if (initialized) {
		shell_print(sh, "Already initialized");
		return -ENOEXEC;
	}

	if (argc != 1 && (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) && argc != 3)) {
		shell_error(sh, "Invalid argument count");
		shell_help(sh);

		return SHELL_CMD_HELP_PRINTED;
	}

#if defined(CONFIG_BT_BAP_UNICAST_SERVER)
	unsigned long snk_cnt, src_cnt;
	struct bt_bap_unicast_server_register_param unicast_server_param = {
		CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
	};
	const struct bt_pacs_register_param pacs_param = {
#if defined(CONFIG_BT_PAC_SNK)
		.snk_pac = true,
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SNK_LOC)
		.snk_loc = true,
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC)
		.src_pac = true,
#endif /* CONFIG_BT_PAC_SRC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
		.src_loc = true,
#endif /* CONFIG_BT_PAC_SRC_LOC */
	};

	if (argc == 3) {
		snk_cnt = shell_strtoul(argv[1], 0, &err);
		if (snk_cnt > CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT) {
			shell_error(sh, "Invalid Sink ASE count: %lu. Valid interval: [0, %u]",
				    snk_cnt, CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT);

			return -ENOEXEC;
		}

		unicast_server_param.snk_cnt = snk_cnt;

		src_cnt = shell_strtoul(argv[2], 0, &err);
		if (src_cnt > CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT) {
			shell_error(sh, "Invalid Source ASE count: %lu. Valid interval: [0, %u]",
				    src_cnt, CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT);

			return -ENOEXEC;
		}

		unicast_server_param.src_cnt = src_cnt;
	} else {
		snk_cnt = CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT;
		src_cnt = CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT;
	}

	err = bt_pacs_register(&pacs_param);
	__ASSERT(err == 0, "Failed to register PACS: %d", err);

	err = bt_bap_unicast_server_register(&unicast_server_param);
	__ASSERT(err == 0, "Failed to register Unicast Server: %d", err);

	err = bt_bap_unicast_server_register_cb(&unicast_server_cb);
	__ASSERT(err == 0, "Failed to register Unicast Server Callbacks: %d", err);
#endif /* CONFIG_BT_BAP_UNICAST_SERVER */

#if defined(CONFIG_BT_PAC_SNK)
	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap_sink);
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SRC)
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap_source);
#endif /* CONFIG_BT_PAC_SNK */

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

#if defined(CONFIG_BT_BAP_UNICAST)
	for (i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		bt_bap_stream_cb_register(bap_stream_from_shell_stream(&unicast_streams[i]),
					  &stream_ops);

		if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT) &&
		    IS_ENABLED(CONFIG_BT_CAP_INITIATOR)) {
			/* If we use the cap initiator, we need to register the callbacks for CAP
			 * as well, as CAP will override and use the BAP callbacks if doing a CAP
			 * procedure
			 */
			bt_cap_stream_ops_register(&unicast_streams[i].stream, &stream_ops);
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST */

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
	bt_bap_broadcast_sink_register_cb(&sink_cbs);
	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	bt_le_scan_cb_register(&bap_scan_cb);

	for (i = 0; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		bt_bap_stream_cb_register(bap_stream_from_shell_stream(&broadcast_sink_streams[i]),
					  &stream_ops);
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	for (i = 0; i < ARRAY_SIZE(broadcast_source_streams); i++) {
		bt_bap_stream_cb_register(
			bap_stream_from_shell_stream(&broadcast_source_streams[i]), &stream_ops);
	}

	default_source.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

#if defined(CONFIG_LIBLC3)
#if defined(CONFIG_BT_AUDIO_RX)
	static K_KERNEL_STACK_DEFINE(lc3_decoder_thread_stack, 4096);
	const int lc3_decoder_thread_prio = K_PRIO_PREEMPT(5);
	static struct k_thread lc3_decoder_thread;

	k_thread_create(&lc3_decoder_thread, lc3_decoder_thread_stack,
			K_KERNEL_STACK_SIZEOF(lc3_decoder_thread_stack), lc3_decoder_thread_func,
			NULL, NULL, NULL, lc3_decoder_thread_prio, 0, K_NO_WAIT);
	k_thread_name_set(&lc3_decoder_thread, "LC3 Decoder");
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
	static K_KERNEL_STACK_DEFINE(lc3_encoder_thread_stack, 4096);
	const int lc3_encoder_thread_prio = K_PRIO_PREEMPT(5);
	static struct k_thread lc3_encoder_thread;

	k_thread_create(&lc3_encoder_thread, lc3_encoder_thread_stack,
			K_KERNEL_STACK_SIZEOF(lc3_encoder_thread_stack), lc3_encoder_thread_func,
			NULL, NULL, NULL, lc3_encoder_thread_prio, 0, K_NO_WAIT);
	k_thread_name_set(&lc3_encoder_thread, "LC3 Encoder");

#endif /* CONFIG_BT_AUDIO_TX */

	if (IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) &&
	    (IS_ENABLED(CONFIG_BT_AUDIO_RX) || IS_ENABLED(CONFIG_BT_AUDIO_TX))) {
		err = bap_usb_init();
		__ASSERT(err == 0, "Failed to enable USB: %d", err);
	}
#endif /* CONFIG_LIBLC3 */

	initialized = true;

	return 0;
}

#if defined(CONFIG_BT_AUDIO_TX)

#define DATA_MTU CONFIG_BT_ISO_TX_MTU
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, DATA_MTU, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	static uint8_t data[DATA_MTU - BT_ISO_CHAN_SEND_RESERVE];
	int ret, len;
	struct net_buf *buf;

	if (default_stream == NULL) {
		shell_error(sh, "Invalid (NULL) stream");

		return -ENOEXEC;
	}

	if (default_stream->qos == NULL) {
		shell_error(sh, "NULL stream QoS");

		return -ENOEXEC;
	}

	if (argc > 1) {
		len = hex2bin(argv[1], strlen(argv[1]), data, sizeof(data));
		if (len > default_stream->qos->sdu) {
			shell_print(sh, "Unable to send: len %d > %u MTU",
				    len, default_stream->qos->sdu);

			return -ENOEXEC;
		}
	} else {
		len = MIN(default_stream->qos->sdu, sizeof(data));
		memset(data, 0xff, len);
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, data, len);

	ret = bt_bap_stream_send(default_stream, buf, get_next_seq_num(default_stream));
	if (ret < 0) {
		shell_print(sh, "Unable to send: %d", -ret);
		net_buf_unref(buf);

		return -ENOEXEC;
	}

	shell_print(sh, "Sending:");
	shell_hexdump(sh, data, len);

	return 0;
}

#if GENERATE_SINE_SUPPORTED
static void start_sine_stream_cb(struct shell_stream *sh_stream, void *user_data)
{
	if (sh_stream->is_tx) {
		struct bt_bap_stream *bap_stream = bap_stream_from_shell_stream(sh_stream);
		const struct shell *sh = user_data;
		int err;

		err = init_lc3_encoder(sh_stream);
		if (err != 0) {
			shell_error(sh, "Failed to init LC3 %d for stream %p", err, bap_stream);

			return;
		}

		sh_stream->tx.active = true;
		sh_stream->tx.seq_num = get_next_seq_num(bap_stream_from_shell_stream(sh_stream));

		shell_print(sh, "Started transmitting sine on stream %p", bap_stream);
	}
}

static int cmd_start_sine(const struct shell *sh, size_t argc, char *argv[])
{
	bool start_all = false;

	if (argc > 1) {
		if (strcmp(argv[1], "all") == 0) {
			start_all = true;
		} else {
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if (start_all) {
		bap_foreach_stream(start_sine_stream_cb, (void *)sh);
	} else {
		struct shell_stream *sh_stream = shell_stream_from_bap_stream(default_stream);

		start_sine_stream_cb(sh_stream, (void *)sh);
	}

	return 0;
}

static void stop_sine_stream_cb(struct shell_stream *sh_stream, void *user_data)
{
	if (sh_stream->is_tx) {
		struct bt_bap_stream *bap_stream = bap_stream_from_shell_stream(sh_stream);
		const struct shell *sh = user_data;

		shell_print(sh, "Stopped transmitting on stream %p", bap_stream);

		sh_stream->tx.active = false;
	}
}

static int cmd_stop_sine(const struct shell *sh, size_t argc, char *argv[])
{
	bool stop_all = false;

	if (argc > 1) {
		if (strcmp(argv[1], "all") == 0) {
			stop_all = true;
		} else {
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if (stop_all) {
		bap_foreach_stream(stop_sine_stream_cb, (void *)sh);
	} else {
		struct shell_stream *sh_stream = shell_stream_from_bap_stream(default_stream);

		stop_sine_stream_cb(sh_stream, (void *)sh);
	}

	return 0;
}
#endif /* GENERATE_SINE_SUPPORTED */
#endif /* CONFIG_BT_AUDIO_TX */

static int cmd_bap_stats(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_info(sh, "Current stats interval: %lu", bap_stats_interval);
	} else {
		int err = 0;
		unsigned long interval;

		interval = shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "Could not parse interval: %d", err);

			return -ENOEXEC;
		}

		if (interval == 0U) {
			shell_error(sh, "Interval cannot be 0");

			return -ENOEXEC;
		}

		bap_stats_interval = interval;
	}

	return 0;
}

#if defined(CONFIG_BT_BAP_UNICAST_SERVER)
static void print_ase_info(struct bt_bap_ep *ep, void *user_data)
{
	struct bt_bap_ep_info info;
	int err;

	err = bt_bap_ep_get_info(ep, &info);
	if (err == 0) {
		printk("ASE info: id %u state %u dir %u\n", info.id, info.state, info.dir);
	}
}

static int cmd_print_ase_info(const struct shell *sh, size_t argc, char *argv[])
{
	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	bt_bap_unicast_server_foreach_ep(default_conn, print_ase_info, NULL);

	return 0;
}
#endif /* CONFIG_BT_BAP_UNICAST_SERVER */

/* 31 is a unit separator - without t the tab is seemingly ignored*/
#define HELP_SEP "\n\31\t"

#define HELP_CFG_DATA                                                                              \
	"\n[config" HELP_SEP "[freq <frequency>]" HELP_SEP "[dur <duration>]" HELP_SEP             \
	"[chan_alloc <location>]" HELP_SEP "[frame_len <frame length>]" HELP_SEP                   \
	"[frame_blks <frame blocks>]]"

#define HELP_CFG_META                                                                              \
	"\n[meta" HELP_SEP "[pref_ctx <context>]" HELP_SEP "[stream_ctx <context>]" HELP_SEP       \
	"[program_info <program info>]" HELP_SEP "[lang <ISO 639-3 lang>]" HELP_SEP                \
	"[ccid_list <ccids>]" HELP_SEP "[parental_rating <rating>]" HELP_SEP                       \
	"[program_info_uri <URI>]" HELP_SEP "[audio_active_state <state>]" HELP_SEP                \
	"[bcast_flag]" HELP_SEP "[extended <meta>]" HELP_SEP "[vendor <meta>]]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	bap_cmds,
	SHELL_CMD_ARG(init, NULL, NULL, cmd_init, 1,
		      IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) ? 2 : 0),
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	SHELL_CMD_ARG(select_broadcast, NULL, "<stream>", cmd_select_broadcast_source, 2, 0),
	SHELL_CMD_ARG(create_broadcast, NULL, "[preset <preset_name>] [enc <broadcast_code>]",
		      cmd_create_broadcast, 1, 2),
	SHELL_CMD_ARG(start_broadcast, NULL, "", cmd_start_broadcast, 1, 0),
	SHELL_CMD_ARG(stop_broadcast, NULL, "", cmd_stop_broadcast, 1, 0),
	SHELL_CMD_ARG(delete_broadcast, NULL, "", cmd_delete_broadcast, 1, 0),
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
	SHELL_CMD_ARG(create_broadcast_sink, NULL, "0x<broadcast_id>", cmd_create_broadcast_sink, 2,
		      0),
	SHELL_CMD_ARG(create_sink_by_name, NULL, "<broadcast_name>",
		      cmd_create_sink_by_name, 2, 0),
	SHELL_CMD_ARG(sync_broadcast, NULL,
		      "0x<bis_index> [[[0x<bis_index>] 0x<bis_index>] ...] "
		      "[bcode <broadcast code> || bcode_str <broadcast code as string>]",
		      cmd_sync_broadcast, 2, ARRAY_SIZE(broadcast_sink_streams) + 1),
	SHELL_CMD_ARG(stop_broadcast_sink, NULL, "Stops broadcast sink", cmd_stop_broadcast_sink, 1,
		      0),
	SHELL_CMD_ARG(term_broadcast_sink, NULL, "", cmd_term_broadcast_sink, 1, 0),
#endif /* CONFIG_BT_BAP_BROADCAST_SINK */
#if defined(CONFIG_BT_BAP_UNICAST)
#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	SHELL_CMD_ARG(discover, NULL, "[dir: sink, source]", cmd_discover, 1, 1),
	SHELL_CMD_ARG(config, NULL,
		      "<direction: sink, source> <index> [loc <loc_bits>] [preset <preset_name>]",
		      cmd_config, 3, 4),
	SHELL_CMD_ARG(stream_qos, NULL, "interval [framing] [latency] [pd] [sdu] [phy] [rtn]",
		      cmd_stream_qos, 2, 6),
	SHELL_CMD_ARG(connect, NULL, "Connect the CIS of the stream", cmd_connect, 1, 0),
	SHELL_CMD_ARG(qos, NULL, "Send QoS configure for Unicast Group", cmd_qos, 1, 0),
	SHELL_CMD_ARG(enable, NULL, "[context]", cmd_enable, 1, 1),
	SHELL_CMD_ARG(stop, NULL, NULL, cmd_stop, 1, 0),
	SHELL_CMD_ARG(list, NULL, NULL, cmd_list, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#if defined(CONFIG_BT_BAP_UNICAST_SERVER)
	SHELL_CMD_ARG(print_ase_info, NULL, "Print ASE info for default connection",
		      cmd_print_ase_info, 0, 0),
#endif /* CONFIG_BT_BAP_UNICAST_SERVER */
	SHELL_CMD_ARG(metadata, NULL, "[context]", cmd_metadata, 1, 1),
	SHELL_CMD_ARG(start, NULL, NULL, cmd_start, 1, 0),
	SHELL_CMD_ARG(disable, NULL, NULL, cmd_disable, 1, 0),
	SHELL_CMD_ARG(release, NULL, NULL, cmd_release, 1, 0),
	SHELL_CMD_ARG(select_unicast, NULL, "<stream>", cmd_select_unicast, 2, 0),
#endif /* CONFIG_BT_BAP_UNICAST */
#if IS_BAP_INITIATOR
	SHELL_CMD_ARG(preset, NULL,
		      "<sink, source, broadcast> [preset] " HELP_CFG_DATA " " HELP_CFG_META,
		      cmd_preset, 2, 34),
#endif /* IS_BAP_INITIATOR */
#if defined(CONFIG_BT_AUDIO_TX)
	SHELL_CMD_ARG(send, NULL, "Send to Audio Stream [data]", cmd_send, 1, 1),
#if GENERATE_SINE_SUPPORTED
	SHELL_CMD_ARG(start_sine, NULL, "Start sending a LC3 encoded sine wave [all]",
		      cmd_start_sine, 1, 1),
	SHELL_CMD_ARG(stop_sine, NULL, "Stop sending a LC3 encoded sine wave [all]", cmd_stop_sine,
		      1, 1),
#endif /* GENERATE_SINE_SUPPORTED */
#endif /* CONFIG_BT_AUDIO_TX */
	SHELL_CMD_ARG(bap_stats, NULL,
		      "Sets or gets the statistics reporting interval in # of packets",
		      cmd_bap_stats, 1, 1),
	SHELL_COND_CMD_ARG(CONFIG_BT_PACS, set_location, NULL,
			   "<direction: sink, source> <location bitmask>", cmd_set_loc, 3, 0),
	SHELL_COND_CMD_ARG(CONFIG_BT_PACS, set_context, NULL,
			   "<direction: sink, source>"
			   "<context bitmask> <type: supported, available>",
			   cmd_context, 4, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_bap(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(bap, &bap_cmds, "Bluetooth BAP shell commands", cmd_bap, 1, 1);

static size_t connectable_ad_data_add(struct bt_data *data_array, size_t data_array_size)
{
	static const uint8_t ad_ext_uuid16[] = {
		IF_ENABLED(CONFIG_BT_MICP_MIC_DEV, (BT_UUID_16_ENCODE(BT_UUID_MICS_VAL),))
		IF_ENABLED(CONFIG_BT_ASCS, (BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),))
		IF_ENABLED(CONFIG_BT_BAP_SCAN_DELEGATOR, (BT_UUID_16_ENCODE(BT_UUID_BASS_VAL),))
		IF_ENABLED(CONFIG_BT_PACS, (BT_UUID_16_ENCODE(BT_UUID_PACS_VAL),))
		IF_ENABLED(CONFIG_BT_TBS, (BT_UUID_16_ENCODE(BT_UUID_GTBS_VAL),))
		IF_ENABLED(CONFIG_BT_TBS_BEARER_COUNT, (BT_UUID_16_ENCODE(BT_UUID_TBS_VAL),))
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
		sys_put_le16(src_context, &ad_bap_announcement[5]);

		/* Metadata length */
		ad_bap_announcement[7] = 0x00;

		__ASSERT(data_array_size > ad_len, "No space for AD_BAP_ANNOUNCEMENT");
		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_bap_announcement);
		data_array[ad_len].data = &ad_bap_announcement[0];
		ad_len++;
	}

	if (IS_ENABLED(CONFIG_BT_BAP_SCAN_DELEGATOR)) {
		ad_len += bap_scan_delegator_ad_data_add(&data_array[ad_len],
							 data_array_size - ad_len);
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR)) {
		ad_len += cap_acceptor_ad_data_add(&data_array[ad_len], data_array_size - ad_len,
						   true);
	}

	if (IS_ENABLED(CONFIG_BT_GMAP)) {
		ad_len += gmap_ad_data_add(&data_array[ad_len], data_array_size - ad_len);
	}

	if (ARRAY_SIZE(ad_ext_uuid16) > 0) {
		size_t uuid16_size;

		if (data_array_size <= ad_len) {
			bt_shell_warn("No space for AD_UUID16");
			return ad_len;
		}

		data_array[ad_len].type = BT_DATA_UUID16_SOME;

		if (IS_ENABLED(CONFIG_BT_HAS) && IS_ENABLED(CONFIG_BT_PRIVACY)) {
			/* If the HA is in one of the GAP connectable modes and is using a
			 * resolvable private address, the HA shall not include the Hearing Access
			 * Service UUID in the Service UUID AD type field of the advertising data
			 * or scan response.
			 */
			uuid16_size = ARRAY_SIZE(ad_ext_uuid16) - BT_UUID_SIZE_16;
		} else {
			uuid16_size = ARRAY_SIZE(ad_ext_uuid16);
		}

		/* We can maximum advertise 127 16-bit UUIDs = 254 octets */
		data_array[ad_len].data_len = MIN(uuid16_size, 254);

		data_array[ad_len].data = &ad_ext_uuid16[0];
		ad_len++;
	}

	return ad_len;
}

static size_t nonconnectable_ad_data_add(struct bt_data *data_array, const size_t data_array_size)
{
	static const uint8_t ad_ext_uuid16[] = {
		IF_ENABLED(CONFIG_BT_PACS, (BT_UUID_16_ENCODE(BT_UUID_PACS_VAL),))
		IF_ENABLED(CONFIG_BT_CAP_ACCEPTOR, (BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),))
	};
	size_t ad_len = 0;

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	if (default_source.bap_source != NULL) {
		static uint8_t ad_bap_broadcast_announcement[5] = {
			BT_UUID_16_ENCODE(BT_UUID_BROADCAST_AUDIO_VAL),
		};

		if (data_array_size <= ad_len) {
			bt_shell_warn("No space for BT_UUID_BROADCAST_AUDIO_VAL");
			return ad_len;
		}

		sys_put_le24(default_source.broadcast_id, &ad_bap_broadcast_announcement[2]);
		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_bap_broadcast_announcement);
		data_array[ad_len].data = ad_bap_broadcast_announcement;
		ad_len++;
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

	if (ARRAY_SIZE(ad_ext_uuid16) > 0) {
		if (data_array_size <= ad_len) {
			bt_shell_warn("No space for AD_UUID16");
			return ad_len;
		}

		data_array[ad_len].type = BT_DATA_UUID16_SOME;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_ext_uuid16);
		data_array[ad_len].data = &ad_ext_uuid16[0];
		ad_len++;
	}

	return ad_len;
}

size_t audio_ad_data_add(struct bt_data *data_array, const size_t data_array_size,
			 const bool discoverable, const bool connectable)
{
	size_t ad_len = 0;

	if (!discoverable) {
		return 0;
	}

	if (connectable) {
		ad_len += connectable_ad_data_add(data_array, data_array_size);
	} else {
		ad_len += nonconnectable_ad_data_add(data_array, data_array_size);
	}

	return ad_len;
}

size_t audio_pa_data_add(struct bt_data *data_array, const size_t data_array_size)
{
	size_t ad_len = 0;

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	if (default_source.bap_source != NULL && !default_source.is_cap) {
		/* Required size of the buffer depends on what has been
		 * configured. We just use the maximum size possible.
		 */
		NET_BUF_SIMPLE_DEFINE_STATIC(base_buf, UINT8_MAX);
		int err;

		net_buf_simple_reset(&base_buf);

		err = bt_bap_broadcast_source_get_base(default_source.bap_source, &base_buf);
		if (err != 0) {
			bt_shell_error("Unable to get BASE: %d\n", err);

			return 0;
		}

		data_array[ad_len].type = BT_DATA_SVC_DATA16;
		data_array[ad_len].data_len = base_buf.len;
		data_array[ad_len].data = base_buf.data;
		ad_len++;
	} else if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR)) {
		return cap_initiator_pa_data_add(data_array, data_array_size);
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

	return ad_len;
}
