/*
 * Copyright (c) 2022-2024 Nordic Semiconductor ASA
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_LIBLC3)
#include "lc3.h"
#endif /* defined(CONFIG_LIBLC3) */
#if defined(CONFIG_USB_DEVICE_AUDIO)
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>
#include <zephyr/sys/ring_buffer.h>
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */


BUILD_ASSERT(IS_ENABLED(CONFIG_SCAN_SELF) || IS_ENABLED(CONFIG_SCAN_OFFLOAD),
	     "Either SCAN_SELF or SCAN_OFFLOAD must be enabled");

#define SEM_TIMEOUT                 K_SECONDS(60)
#define BROADCAST_ASSISTANT_TIMEOUT K_SECONDS(120) /* 2 minutes */

#define LOG_INTERVAL 1000U

#if defined(CONFIG_SCAN_SELF)
#define ADV_TIMEOUT K_SECONDS(CONFIG_SCAN_DELAY)
#else /* !CONFIG_SCAN_SELF */
#define ADV_TIMEOUT K_FOREVER
#endif /* CONFIG_SCAN_SELF */

#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 5 /* Set the timeout relative to interval */
#define PA_SYNC_SKIP                5
#define NAME_LEN                    sizeof(CONFIG_TARGET_BROADCAST_NAME) + 1
#define BROADCAST_DATA_ELEMENT_SIZE sizeof(int16_t)

#if defined(CONFIG_LIBLC3)
#define LC3_MAX_SAMPLE_RATE        48000U
#define LC3_MAX_FRAME_DURATION_US  10000U
#define LC3_MAX_NUM_SAMPLES_MONO   ((LC3_MAX_FRAME_DURATION_US * LC3_MAX_SAMPLE_RATE)              \
				    / USEC_PER_SEC)
#define LC3_MAX_NUM_SAMPLES_STEREO (LC3_MAX_NUM_SAMPLES_MONO * 2)

#define LC3_ENCODER_STACK_SIZE  4096
#define LC3_ENCODER_PRIORITY    5
#endif /* defined(CONFIG_LIBLC3) */

#if defined(CONFIG_USB_DEVICE_AUDIO)
#define USB_ENQUEUE_COUNT            10U
#define USB_SAMPLE_RATE	             48000U
#define USB_FRAME_DURATION_US        1000U
#define USB_MONO_SAMPLE_SIZE                                                                       \
	((USB_FRAME_DURATION_US * USB_SAMPLE_RATE * BROADCAST_DATA_ELEMENT_SIZE) / USEC_PER_SEC)
#define USB_STEREO_SAMPLE_SIZE       (USB_MONO_SAMPLE_SIZE * 2)
#define USB_RING_BUF_SIZE            (5 * LC3_MAX_NUM_SAMPLES_STEREO) /* 5 SDUs*/
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */

static K_SEM_DEFINE(sem_broadcast_sink_stopped, 0U, 1U);
static K_SEM_DEFINE(sem_connected, 0U, 1U);
static K_SEM_DEFINE(sem_disconnected, 0U, 1U);
static K_SEM_DEFINE(sem_broadcaster_found, 0U, 1U);
static K_SEM_DEFINE(sem_pa_synced, 0U, 1U);
static K_SEM_DEFINE(sem_base_received, 0U, 1U);
static K_SEM_DEFINE(sem_syncable, 0U, 1U);
static K_SEM_DEFINE(sem_pa_sync_lost, 0U, 1U);
static K_SEM_DEFINE(sem_broadcast_code_received, 0U, 1U);
static K_SEM_DEFINE(sem_pa_request, 0U, 1U);
static K_SEM_DEFINE(sem_past_request, 0U, 1U);
static K_SEM_DEFINE(sem_bis_sync_requested, 0U, 1U);
static K_SEM_DEFINE(sem_stream_connected, 0U, CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT);
static K_SEM_DEFINE(sem_stream_started, 0U, CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT);
static K_SEM_DEFINE(sem_big_synced, 0U, 1U);

/* Sample assumes that we only have a single Scan Delegator receive state */
static const struct bt_bap_scan_delegator_recv_state *req_recv_state;
static struct bt_bap_broadcast_sink *broadcast_sink;
static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static struct bt_le_per_adv_sync *pa_sync;
static uint32_t broadcaster_broadcast_id;
static struct broadcast_sink_stream {
	struct bt_bap_stream stream;
	size_t recv_cnt;
	size_t loss_cnt;
	size_t error_cnt;
	size_t valid_cnt;
#if defined(CONFIG_LIBLC3)
	struct net_buf *in_buf;
	struct k_work_delayable lc3_decode_work;

	/* LC3 config values */
	enum bt_audio_location chan_allocation;
	uint16_t lc3_octets_per_frame;
	uint8_t lc3_frames_blocks_per_sdu;

	/* Internal lock for protecting net_buf from multiple access */
	struct k_mutex lc3_decoder_mutex;
	lc3_decoder_t lc3_decoder;
	lc3_decoder_mem_48k_t lc3_decoder_mem;
#endif /* defined(CONFIG_LIBLC3) */
} streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];

static struct bt_bap_stream *streams_p[ARRAY_SIZE(streams)];
static volatile bool big_synced;
static volatile bool base_received;
static struct bt_conn *broadcast_assistant_conn;
static struct bt_le_ext_adv *ext_adv;
static volatile uint8_t stream_count;

static const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_16KHZ | BT_AUDIO_CODEC_CAP_FREQ_24KHZ,
	BT_AUDIO_CODEC_CAP_DURATION_10, BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40u, 60u,
	CONFIG_MAX_CODEC_FRAMES_PER_SDU,
	(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. Bit 0 is BIS index 1.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams));
static uint32_t requested_bis_sync;
static uint32_t bis_index_bitfield;
static uint8_t sink_broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];

uint64_t total_rx_iso_packet_count; /* This value is exposed to test code */

static int stop_adv(void);

#if defined(CONFIG_USB_DEVICE_AUDIO)
RING_BUF_DECLARE(usb_ring_buf, USB_RING_BUF_SIZE);
NET_BUF_POOL_DEFINE(usb_tx_buf_pool, USB_ENQUEUE_COUNT, USB_STEREO_SAMPLE_SIZE, 0, net_buf_destroy);

static void add_to_usb_ring_buf(const int16_t audio_buf[LC3_MAX_NUM_SAMPLES_STEREO]);
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */

#if defined(CONFIG_LIBLC3)
static K_SEM_DEFINE(lc3_decoder_sem, 0, 1);

static void do_lc3_decode(lc3_decoder_t decoder, const void *in_data, uint8_t octets_per_frame,
			  int16_t out_data[LC3_MAX_NUM_SAMPLES_MONO]);
static void lc3_decoder_thread(void *arg1, void *arg2, void *arg3);
K_THREAD_DEFINE(decoder_tid, LC3_ENCODER_STACK_SIZE, lc3_decoder_thread,
		NULL, NULL, NULL, LC3_ENCODER_PRIORITY, 0, -1);

/* Consumer thread of the decoded stream data */
static void lc3_decoder_thread(void *arg1, void *arg2, void *arg3)
{
	while (true) {
#if defined(CONFIG_USB_DEVICE_AUDIO)
		static int16_t right_frames[CONFIG_MAX_CODEC_FRAMES_PER_SDU]
					   [LC3_MAX_NUM_SAMPLES_MONO];
		static int16_t left_frames[CONFIG_MAX_CODEC_FRAMES_PER_SDU]
					  [LC3_MAX_NUM_SAMPLES_MONO];
		size_t right_frames_cnt = 0;
		size_t left_frames_cnt = 0;

		memset(right_frames, 0, sizeof(right_frames));
		memset(left_frames, 0, sizeof(left_frames));
#else
		static int16_t lc3_audio_buf[LC3_MAX_NUM_SAMPLES_MONO];
#endif /* CONFIG_USB_DEVICE_AUDIO */

		k_sem_take(&lc3_decoder_sem, K_FOREVER);

		for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
			struct broadcast_sink_stream *stream = &streams[i];
			const uint8_t frames_blocks_per_sdu = stream->lc3_frames_blocks_per_sdu;
			const uint16_t octets_per_frame = stream->lc3_octets_per_frame;
			uint16_t frames_per_block;
			struct net_buf *buf;

			k_mutex_lock(&stream->lc3_decoder_mutex, K_FOREVER);

			if (stream->in_buf == NULL) {
				k_mutex_unlock(&stream->lc3_decoder_mutex);

				continue;
			}

			buf = net_buf_ref(stream->in_buf);
			net_buf_unref(stream->in_buf);
			stream->in_buf = NULL;
			k_mutex_unlock(&stream->lc3_decoder_mutex);

			frames_per_block = bt_audio_get_chan_count(stream->chan_allocation);
			if (buf->len !=
			    (frames_per_block * octets_per_frame * frames_blocks_per_sdu)) {
				printk("Expected %u frame blocks with %u frames of size %u, but "
				       "length is %u\n",
				       frames_blocks_per_sdu, frames_per_block, octets_per_frame,
				       buf->len);

				net_buf_unref(buf);

				continue;
			}

#if defined(CONFIG_USB_DEVICE_AUDIO)
			const bool has_left =
				(stream->chan_allocation & BT_AUDIO_LOCATION_FRONT_LEFT) != 0;
			const bool has_right =
				(stream->chan_allocation & BT_AUDIO_LOCATION_FRONT_RIGHT) != 0;
			const bool is_mono =
				stream->chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO;

			/* Split the SDU into frames*/
			for (uint8_t i = 0U; i < frames_blocks_per_sdu; i++) {
				for (uint16_t j = 0U; j < frames_per_block; j++) {
					const bool is_left = j == 0 && has_left;
					const bool is_right =
						has_right && (j == 0 || (j == 1 && has_left));
					const void *data = net_buf_pull_mem(buf, octets_per_frame);
					int16_t *out_frame;

					if (is_left) {
						out_frame = left_frames[left_frames_cnt++];
					} else if (is_right) {
						out_frame = right_frames[right_frames_cnt++];
					} else if (is_mono) {
						/* Use left as mono*/
						out_frame = left_frames[left_frames_cnt++];
					} else {
						/* unused channel */
						break;
					}

					do_lc3_decode(stream->lc3_decoder, data, octets_per_frame,
						      out_frame);
				}
			}
#else
			/* Dummy behavior: Decode and discard data */
			for (uint8_t i = 0U; i < frames_blocks_per_sdu; i++) {
				for (uint16_t j = 0U; j < frames_per_block; j++) {
					const void *data = net_buf_pull_mem(buf, octets_per_frame);

					do_lc3_decode(stream->lc3_decoder, data, octets_per_frame,
						      lc3_audio_buf);
				}
			}
#endif /* CONFIG_USB_DEVICE_AUDIO */

			net_buf_unref(buf);
		}

#if defined(CONFIG_USB_DEVICE_AUDIO)
		const bool is_left_only = right_frames_cnt == 0U;
		const bool is_right_only = left_frames_cnt == 0U;

		if (!is_left_only && !is_right_only && left_frames_cnt != right_frames_cnt) {
			printk("Mismatch between number of left (%zu) and right (%zu) frames, "
			       "discard SDU",
			       left_frames_cnt, right_frames_cnt);
			continue;
		}

		/* Send frames to USB - If we only have a single channel we mix it to stereo */
		for (size_t i = 0U; i < MAX(left_frames_cnt, right_frames_cnt); i++) {
			const bool is_single_channel = is_left_only || is_right_only;
			static int16_t stereo_frame[LC3_MAX_NUM_SAMPLES_STEREO];
			int16_t *right_frame = right_frames[i];
			int16_t *left_frame = left_frames[i];

			/* Not enough space to store data */
			if (ring_buf_space_get(&usb_ring_buf) < sizeof(stereo_frame)) {
				break;
			}

			memset(stereo_frame, 0, sizeof(stereo_frame));

			/* Generate the stereo frame
			 *
			 * If we only have single channel then that is always stored in the
			 * left_frame, and we mix that to stereo
			 */
			for (int j = 0; j < LC3_MAX_NUM_SAMPLES_MONO; j++) {
				if (is_single_channel) {
					/* Mix to stereo */
					if (is_left_only) {
						stereo_frame[j * 2] = left_frame[j];
						stereo_frame[j * 2 + 1] = left_frame[j];
					} else if (is_right_only) {
						stereo_frame[j * 2] = right_frame[j];
						stereo_frame[j * 2 + 1] = right_frame[j];
					}
				} else {
					stereo_frame[j * 2] = left_frame[j];
					stereo_frame[j * 2 + 1] = right_frame[j];
				}
			}

			add_to_usb_ring_buf(stereo_frame);
		}
#endif /* CONFIG_USB_DEVICE_AUDIO */
	}
}

/** Decode LC3 data on a stream and returns true if successful */
static void do_lc3_decode(lc3_decoder_t decoder, const void *in_data, uint8_t octets_per_frame,
			  int16_t out_data[LC3_MAX_NUM_SAMPLES_MONO])
{
	int err;

	err = lc3_decode(decoder, in_data, octets_per_frame, LC3_PCM_FORMAT_S16, out_data, 1);
	if (err == 1) {
		printk("  decoder performed PLC\n");
	} else if (err < 0) {
		printk("  decoder failed - wrong parameters? (err = %d)\n", err);
	}
}

static int lc3_enable(struct broadcast_sink_stream *sink_stream)
{
	size_t chan_alloc_bit_cnt;
	size_t sdu_size_required;
	int frame_duration_us;
	int freq_hz;
	int ret;

	printk("Enable: stream with codec %p\n", sink_stream->stream.codec_cfg);

	ret = bt_audio_codec_cfg_get_freq(sink_stream->stream.codec_cfg);
	if (ret > 0) {
		freq_hz = bt_audio_codec_cfg_freq_to_freq_hz(ret);
	} else {
		printk("Error: Codec frequency not set, cannot start codec.");
		return -1;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(sink_stream->stream.codec_cfg);
	if (ret > 0) {
		frame_duration_us = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
	} else {
		printk("Error: Frame duration not set, cannot start codec.");
		return ret;
	}

	ret = bt_audio_codec_cfg_get_chan_allocation(sink_stream->stream.codec_cfg,
						     &sink_stream->chan_allocation, true);
	if (ret != 0) {
		printk("Error: Channel allocation not set, invalid configuration for LC3");
		return ret;
	}

	ret = bt_audio_codec_cfg_get_octets_per_frame(sink_stream->stream.codec_cfg);
	if (ret > 0) {
		sink_stream->lc3_octets_per_frame = (uint16_t)ret;
	} else {
		printk("Error: Octets per frame not set, invalid configuration for LC3");
		return ret;
	}

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(sink_stream->stream.codec_cfg, true);
	if (ret > 0) {
		sink_stream->lc3_frames_blocks_per_sdu = (uint8_t)ret;
	} else {
		printk("Error: Frame blocks per SDU not set, invalid configuration for LC3");
		return ret;
	}

	/* An SDU can consist of X frame blocks, each with Y frames (one per channel) of size Z in
	 * them. The minimum SDU size required for this is X * Y * Z.
	 */
	chan_alloc_bit_cnt = bt_audio_get_chan_count(sink_stream->chan_allocation);
	sdu_size_required = chan_alloc_bit_cnt * sink_stream->lc3_octets_per_frame *
			    sink_stream->lc3_frames_blocks_per_sdu;
	if (sdu_size_required > sink_stream->stream.qos->sdu) {
		printk("With %zu channels and %u octets per frame and %u frames per block, SDUs "
		       "shall be at minimum %zu, but the stream has been configured for %u",
		       chan_alloc_bit_cnt, sink_stream->lc3_octets_per_frame,
		       sink_stream->lc3_frames_blocks_per_sdu, sdu_size_required,
		       sink_stream->stream.qos->sdu);

		return -EINVAL;
	}

	printk("Enabling LC3 decoder with frame duration %uus, frequency %uHz and with channel "
	       "allocation 0x%08X, %u octets per frame and %u frame blocks per SDU\n",
	       frame_duration_us, freq_hz, sink_stream->chan_allocation,
	       sink_stream->lc3_octets_per_frame, sink_stream->lc3_frames_blocks_per_sdu);

#if defined(CONFIG_USB_DEVICE_AUDIO)
	sink_stream->lc3_decoder = lc3_setup_decoder(frame_duration_us, freq_hz, USB_SAMPLE_RATE,
						     &sink_stream->lc3_decoder_mem);
#else
	sink_stream->lc3_decoder = lc3_setup_decoder(frame_duration_us, freq_hz, 0,
						     &sink_stream->lc3_decoder_mem);
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */

	if (sink_stream->lc3_decoder == NULL) {
		printk("ERROR: Failed to setup LC3 decoder - wrong parameters?\n");
		return -1;
	}

	k_thread_start(decoder_tid);

	return 0;
}
#endif /* defined(CONFIG_LIBLC3) */

#if defined(CONFIG_USB_DEVICE_AUDIO)
/* Move the LC3 data to the USB ring buffer */
static void add_to_usb_ring_buf(const int16_t audio_buf[LC3_MAX_NUM_SAMPLES_STEREO])
{
	uint32_t size;

	size = ring_buf_put(&usb_ring_buf, (uint8_t *)audio_buf,
			    LC3_MAX_NUM_SAMPLES_STEREO * sizeof(int16_t));
	if (size != LC3_MAX_NUM_SAMPLES_STEREO) {
		static int rb_put_failures;

		rb_put_failures++;
		if (rb_put_failures == LOG_INTERVAL) {
			printk("%s: Failure to add to usb_ring_buf %d, %u\n", __func__,
			       rb_put_failures, size);
		}
	}
}

/* USB consumer callback, called every 1ms, consumes data from ring-buffer */
static void usb_data_request_cb(const struct device *dev)
{
	uint8_t usb_audio_data[USB_STEREO_SAMPLE_SIZE] = {0};
	static struct net_buf *pcm_buf;
	static size_t cnt;
	int err;

	ring_buf_get(&usb_ring_buf, (uint8_t *)usb_audio_data, sizeof(usb_audio_data));
	/* Ignore ring_buf_get() return value, if size is 0 we send empty PCM frames to
	 * not starve USB audio interface, if size is lower than USB_STEREO_SAMPLE_SIZE
	 * we send frames padded with 0's as usb_audio_data is 0-initialized
	 */

	pcm_buf = net_buf_alloc(&usb_tx_buf_pool, K_NO_WAIT);
	if (pcm_buf == NULL) {
		printk("Could not allocate pcm_buf\n");
		return;
	}

	net_buf_add_mem(pcm_buf, usb_audio_data, sizeof(usb_audio_data));

	if (cnt % LOG_INTERVAL == 0) {
		printk("Sending USB audio (count = %zu)\n", cnt);
	}

	err = usb_audio_send(dev, pcm_buf, USB_STEREO_SAMPLE_SIZE);
	if (err) {
		printk("Failed to send USB audio: %d\n", err);
		net_buf_unref(pcm_buf);
	}

	cnt++;
}

static void usb_data_written_cb(const struct device *dev, struct net_buf *buf, size_t size)
{
	/* Unreference the buffer now that the USB is done with it */
	net_buf_unref(buf);
}
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */

static void stream_connected_cb(struct bt_bap_stream *stream)
{
	printk("Stream %p connected\n", stream);

	k_sem_give(&sem_stream_connected);
}

static void stream_disconnected_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int err;

	printk("Stream %p disconnected with reason 0x%02X\n", stream, reason);

	err = k_sem_take(&sem_stream_connected, K_NO_WAIT);
	if (err != 0) {
		printk("Failed to take sem_stream_connected: %d\n", err);
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct broadcast_sink_stream *sink_stream =
		CONTAINER_OF(stream, struct broadcast_sink_stream, stream);

	printk("Stream %p started\n", stream);

	total_rx_iso_packet_count = 0U;
	sink_stream->recv_cnt = 0U;
	sink_stream->loss_cnt = 0U;
	sink_stream->valid_cnt = 0U;
	sink_stream->error_cnt = 0U;

#if defined(CONFIG_LIBLC3)
	int err;

	if (stream->codec_cfg != 0 && stream->codec_cfg->id != BT_HCI_CODING_FORMAT_LC3) {
		/* No subgroups with LC3 was found */
		printk("Did not parse an LC3 codec\n");
		return;
	}

	err = lc3_enable(sink_stream);
	if (err < 0) {
		printk("Error: cannot enable LC3 codec: %d", err);
		return;
	}
#endif /* CONFIG_LIBLC3 */

	k_sem_give(&sem_stream_started);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int err;

	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);

	err = k_sem_take(&sem_stream_started, K_NO_WAIT);
	if (err != 0) {
		printk("Failed to take sem_stream_started: %d\n", err);
	}
}

static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	struct broadcast_sink_stream *sink_stream =
		CONTAINER_OF(stream, struct broadcast_sink_stream, stream);

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		sink_stream->error_cnt++;
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		sink_stream->loss_cnt++;
	}

	if (info->flags & BT_ISO_FLAGS_VALID) {
		sink_stream->valid_cnt++;
#if defined(CONFIG_LIBLC3)
		k_mutex_lock(&sink_stream->lc3_decoder_mutex, K_FOREVER);
		if (sink_stream->in_buf != NULL) {
			net_buf_unref(sink_stream->in_buf);
			sink_stream->in_buf = NULL;
		}

		sink_stream->in_buf = net_buf_ref(buf);
		k_mutex_unlock(&sink_stream->lc3_decoder_mutex);
		k_sem_give(&lc3_decoder_sem);
#endif /* defined(CONFIG_LIBLC3) */
	}

	total_rx_iso_packet_count++;
	sink_stream->recv_cnt++;
	if ((sink_stream->recv_cnt % LOG_INTERVAL) == 0U) {
		printk("Stream %p: received %u total ISO packets: Valid %u | Error %u | Loss %u\n",
		       &sink_stream->stream, sink_stream->recv_cnt, sink_stream->valid_cnt,
		       sink_stream->error_cnt, sink_stream->loss_cnt);
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.connected = stream_connected_cb,
	.disconnected = stream_disconnected_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.recv = stream_recv_cb,
};

#if defined(CONFIG_TARGET_BROADCAST_CHANNEL)
struct bis_channel_allocation_data {
	struct {
		bool chan_allocation_available;
		uint8_t index;
		enum bt_audio_location chan_allocation;
	} bis[BT_ISO_BIS_INDEX_MAX];

	uint8_t cnt;
};

/**
 * This is called for each BIS in a subgroup
 *
 * Gets BIS channel allocation (if exists).
 * Always returns `true` to continue to next BIS
 */
static bool bis_get_channel_allocation_cb(const struct bt_bap_base_subgroup_bis *bis,
					  void *user_data)
{
	struct bis_channel_allocation_data *data = user_data;
	struct bt_audio_codec_cfg codec_cfg;
	int err, idx;

	idx = data->cnt++;
	data->bis[idx].index = bis->index;
	data->bis[idx].chan_allocation_available = false;

	err = bt_bap_base_subgroup_bis_codec_to_codec_cfg(bis, &codec_cfg);
	if (err != 0) {
		printk("Could not get codec configuration for BIS: %d\n", err);

		return true; /* continue to next BIS */
	}

	err = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &data->bis[idx].chan_allocation,
						     false);
	if (err != 0) {
		printk("Could not find channel allocation for BIS: %d\n", err);

		return true; /* continue to next BIS */
	}

	/* Channel allocation data available for this bis */
	data->bis[idx].chan_allocation_available = true;

	return true; /* continue to next BIS */
}

/**
 * Called for each subgroup in the BASE. Will populate the 32-bit bitfield of BIS indexes if the
 * subgroup contains it.
 *
 * The channel allocation may
 *  - Not exist at all, implicitly meaning BT_AUDIO_LOCATION_MONO_AUDIO
 *  - Exist only in the subgroup codec configuration
 *  - Exist only in the BIS codec configuration
 *  - Exist in both the subgroup and BIS codec configuration, in which case, the BIS codec
 *    configuration overwrites the subgroup values
 *
 * This function returns `true` if the subgroup does not support the channels in
 * CONFIG_TARGET_BROADCAST_CHANNEL which makes it iterate over the next subgroup, and
 * returns `false` if this subgroup satisfies our CONFIG_TARGET_BROADCAST_CHANNEL.
 */
static bool subgroup_get_valid_bis_indexes_cb(const struct bt_bap_base_subgroup *subgroup,
					      void *user_data)
{
	enum bt_audio_location subgroup_chan_allocation;
	enum bt_audio_location chan_allocation = BT_AUDIO_LOCATION_MONO_AUDIO;
	bool subgroup_chan_allocation_available = false;
	struct bt_audio_codec_cfg codec_cfg;
	struct bis_channel_allocation_data data = {
		.cnt = 0,
	};
	uint32_t bis_indexes = 0;
	int err;

	err = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &codec_cfg);
	if (err != 0) {
		printk("Could not get codec configuration: %d\n", err);

		return true; /* continue to next subgroup */
	}

	if (codec_cfg.id != BT_HCI_CODING_FORMAT_LC3) {
		/* Only LC3 codec supported */
		return false; /* abort */
	}

	/* Get channel allocation at subgroup level */
	err = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &subgroup_chan_allocation, true);
	if (err == 0) {
		printk("Channel allocation (subgroup level) 0x%x\n", subgroup_chan_allocation);
		subgroup_chan_allocation_available = true;
	} else {
		/* subgroup error */
		return false; /* abort */
	}

	/* Get channel allocation at BIS level */
	err = bt_bap_base_subgroup_foreach_bis(subgroup, bis_get_channel_allocation_cb, &data);
	if (err != 0) {
		printk("Get channel allocation error %d\n", err);

		return true; /* continue to next subgroup */
	}

	/* If no BIS channel allocation available use subgroup channel allocation instead if
	 * exists (otherwise mono assumed)
	 */
	for (uint8_t i = 0U; i < data.cnt; i++) {
		if (!data.bis[i].chan_allocation_available) {
			data.bis[i].chan_allocation = subgroup_chan_allocation_available
							      ? subgroup_chan_allocation
							      : BT_AUDIO_LOCATION_MONO_AUDIO;
		}
	}

	/* Get the BIS indexes */
	for (uint8_t i = 0U; i < data.cnt; i++) {
		if ((data.bis[i].chan_allocation == CONFIG_TARGET_BROADCAST_CHANNEL) ||
		    ((data.bis[i].chan_allocation & CONFIG_TARGET_BROADCAST_CHANNEL) ==
		     CONFIG_TARGET_BROADCAST_CHANNEL)) {
			/* Exact match */
			bis_indexes = BT_ISO_BIS_INDEX_BIT(data.bis[i].index);

			printk("Channel allocation match. BIS index bitfield 0x%x\n", bis_indexes);
			*(uint32_t *)user_data = bis_indexes;

			return false; /* bis index found */
		} else if ((data.bis[i].chan_allocation & CONFIG_TARGET_BROADCAST_CHANNEL) != 0) {
			/* Partial match */
			chan_allocation |= data.bis[i].chan_allocation;
			bis_indexes |= BT_ISO_BIS_INDEX_BIT(data.bis[i].index);

			if ((chan_allocation & CONFIG_TARGET_BROADCAST_CHANNEL) ==
			    CONFIG_TARGET_BROADCAST_CHANNEL) {
				printk("Channel allocation match. BIS index bitfield 0x%x\n",
				       bis_indexes);
				*(uint32_t *)user_data = bis_indexes;

				return false; /* bis indexes found */
			}
		}
	}
	return true; /* continue to next subgroup */
}
#endif /* CONFIG_TARGET_BROADCAST_CHANNEL */

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	uint32_t base_bis_index_bitfield = 0U;
	int err;

	if (base_received) {
		return;
	}

	printk("Received BASE with %d subgroups from broadcast sink %p\n",
	       bt_bap_base_get_subgroup_count(base), sink);

#if defined(CONFIG_TARGET_BROADCAST_CHANNEL)
	/**
	 * Get a 32-bit bitfield of BIS indexes that cover the channel allocation values in
	 * CONFIG_TARGET_BROADCAST_CHANNEL.
	 */
	printk("Target channel location: 0x%x\n", CONFIG_TARGET_BROADCAST_CHANNEL);
	err = bt_bap_base_foreach_subgroup(base, subgroup_get_valid_bis_indexes_cb,
					   &base_bis_index_bitfield);
	if ((err != 0 && err != -ECANCELED) ||
	    (err == -ECANCELED && base_bis_index_bitfield == 0)) {
		printk("Failed to get valid BIS indexes: %d\n", err);

		return;
	}
#else
	err = bt_bap_base_get_bis_indexes(base, &base_bis_index_bitfield);
	if (err != 0) {
		printk("Failed to get BIS indexes: %d\n", err);

		return;
	}

#endif /* CONFIG_TARGET_BROADCAST_CHANNEL */

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

	printk("bis_index_bitfield = 0x%08x\n", bis_index_bitfield);

	if (broadcast_assistant_conn == NULL) {
		/* No broadcast assistant requesting anything */
		requested_bis_sync = BT_BAP_BIS_SYNC_NO_PREF;
		k_sem_give(&sem_bis_sync_requested);
	}

	base_received = true;
	k_sem_give(&sem_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	printk("Broadcast sink (%p) is syncable, BIG %s\n", (void *)sink,
	       biginfo->encryption ? "encrypted" : "not encrypted");

	k_sem_give(&sem_syncable);

	if (!biginfo->encryption) {
		k_sem_give(&sem_broadcast_code_received);
	}
}

static void broadcast_sink_started_cb(struct bt_bap_broadcast_sink *sink)
{
	printk("Broadcast sink %p started\n", sink);

	big_synced = true;
	k_sem_give(&sem_big_synced);
}

static void broadcast_sink_stopped_cb(struct bt_bap_broadcast_sink *sink, uint8_t reason)
{
	printk("Broadcast sink %p stopped with reason 0x%02X\n", sink, reason);

	big_synced = false;
	k_sem_give(&sem_broadcast_sink_stopped);
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
	.started = broadcast_sink_started_cb,
	.stopped = broadcast_sink_stopped_cb,
};

static void pa_timer_handler(struct k_work *work)
{
	if (req_recv_state != NULL) {
		enum bt_bap_pa_state pa_state;

		if (req_recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
			pa_state = BT_BAP_PA_STATE_NO_PAST;
		} else {
			pa_state = BT_BAP_PA_STATE_FAILED;
		}

		bt_bap_scan_delegator_set_pa_state(req_recv_state->src_id,
						   pa_state);
	}

	printk("PA timeout\n");
}

static K_WORK_DELAYABLE_DEFINE(pa_timer, pa_timer_handler);

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t pa_timeout;

	if (pa_interval == BT_BAP_PA_INTERVAL_UNKNOWN) {
		/* Use maximum value to maximize chance of success */
		pa_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	} else {
		uint32_t interval_us;
		uint32_t timeout;

		/* Add retries and convert to unit in 10's of ms */
		interval_us = BT_GAP_PER_ADV_INTERVAL_TO_US(pa_interval);
		timeout = BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(interval_us) *
			  PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO;

		/* Enforce restraints */
		pa_timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);
	}

	return pa_timeout;
}

static int pa_sync_past(struct bt_conn *conn, uint16_t pa_interval)
{
	struct bt_le_per_adv_sync_transfer_param param = { 0 };
	int err;

	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(pa_interval);

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);
	if (err != 0) {
		printk("Could not do PAST subscribe: %d\n", err);
	} else {
		printk("Syncing with PAST\n");
		(void)k_work_reschedule(&pa_timer, K_MSEC(param.timeout * 10));
	}

	return err;
}

static void recv_state_updated_cb(struct bt_conn *conn,
				  const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	printk("Receive state updated, pa sync state: %u, encrypt_state %u\n",
	       recv_state->pa_sync_state, recv_state->encrypt_state);

	for (uint8_t i = 0; i < recv_state->num_subgroups; i++) {
		printk("subgroup %d bis_sync: 0x%08x\n", i, recv_state->subgroups[i].bis_sync);
	}

	req_recv_state = recv_state;
}

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{

	printk("Received request to sync to PA (PAST %savailble): %u\n", past_avail ? "" : "not ",
	       recv_state->pa_sync_state);

	req_recv_state = recv_state;

	if (recv_state->pa_sync_state == BT_BAP_PA_STATE_SYNCED ||
	    recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		/* Already syncing */
		/* TODO: Terminate existing sync and then sync to new?*/
		return -1;
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER) && past_avail) {
		int err;

		err = pa_sync_past(conn, pa_interval);
		if (err != 0) {
			printk("Failed to subscribe to PAST: %d\n", err);

			return err;
		}

		k_sem_give(&sem_past_request);

		err = bt_bap_scan_delegator_set_pa_state(recv_state->src_id,
							 BT_BAP_PA_STATE_INFO_REQ);
		if (err != 0) {
			printk("Failed to set PA state to BT_BAP_PA_STATE_INFO_REQ: %d\n", err);

			return err;
		}
	}

	k_sem_give(&sem_pa_request);

	return 0;
}

static int pa_sync_term_req_cb(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	int err;

	printk("PA sync termination req, pa sync state: %u\n", recv_state->pa_sync_state);

	for (uint8_t i = 0; i < recv_state->num_subgroups; i++) {
		printk("subgroup %d bis_sync: 0x%08x\n", i, recv_state->subgroups[i].bis_sync);
	}

	req_recv_state = recv_state;

	printk("Delete periodic advertising sync\n");
	err = bt_le_per_adv_sync_delete(pa_sync);
	if (err != 0) {
		printk("Could not delete per adv sync: %d\n", err);

		return err;
	}

	return 0;
}

static void broadcast_code_cb(struct bt_conn *conn,
			      const struct bt_bap_scan_delegator_recv_state *recv_state,
			      const uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	printk("Broadcast code received for %p\n", recv_state);

	req_recv_state = recv_state;

	(void)memcpy(sink_broadcast_code, broadcast_code, BT_ISO_BROADCAST_CODE_SIZE);

	k_sem_give(&sem_broadcast_code_received);
}

static int bis_sync_req_cb(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   const uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	/* Bit field indicating from which subgroup(s) BIS sync is requested */
	uint32_t requested_subgroup_sync = 0; /* currently only used for printout */

	requested_bis_sync = 0;

	for (uint8_t subgroup = 0; subgroup < recv_state->num_subgroups; subgroup++) {
		if (bis_sync_req[subgroup] != 0) {
			if (requested_bis_sync == 0) {
				requested_bis_sync = bis_sync_req[subgroup];
			} else {
				if (requested_bis_sync != BT_BAP_BIS_SYNC_NO_PREF &&
				    bis_sync_req[subgroup] != BT_BAP_BIS_SYNC_NO_PREF) {
					/* Spec a little bit unclear. Here we choose to say that
					 * BIS sync request from more than 1 subgroup is not
					 * possible unless sync value is 0 or
					 * BT_BAP_BIS_SYNC_NO_PREF
					 */
					printk("Unsupported BIS sync request from more than 1 "
					       "subgroup\n");
					return -EINVAL;
				}
			}
			requested_subgroup_sync |= BIT(subgroup);
		}
	}

	printk("BIS sync req for %p: BIS indexes 0x%08x (subgroup indexes 0x%08x), "
	       "broadcast id: 0x%06x, (%s)\n",
	       recv_state, requested_bis_sync, requested_subgroup_sync, recv_state->broadcast_id,
	       big_synced ? "BIG synced" : "BIG not synced");

	if (big_synced && requested_bis_sync == 0) {
		int err;

		/* The stream stopped callback will be called as part of this,
		 * and we do not need to wait for any events from the
		 * controller. Thus, when this returns, the `big_synced`
		 * is back to false.
		 */
		err = bt_bap_broadcast_sink_stop(broadcast_sink);
		if (err != 0) {
			printk("Failed to stop Broadcast Sink: %d\n", err);

			return err;
		}
	}

	broadcaster_broadcast_id = recv_state->broadcast_id;
	if (requested_bis_sync != 0) {
		k_sem_give(&sem_bis_sync_requested);
	}

	return 0;
}

static struct bt_bap_scan_delegator_cb scan_delegator_cbs = {
	.recv_state_updated = recv_state_updated_cb,
	.pa_sync_req = pa_sync_req_cb,
	.pa_sync_term_req = pa_sync_term_req_cb,
	.broadcast_code = broadcast_code_cb,
	.bis_sync_req = bis_sync_req_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0U) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		broadcast_assistant_conn = NULL;
		return;
	}

	printk("Connected: %s\n", addr);
	broadcast_assistant_conn = bt_conn_ref(conn);

	k_sem_give(&sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != broadcast_assistant_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(broadcast_assistant_conn);
	broadcast_assistant_conn = NULL;

	k_sem_give(&sem_disconnected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_pacs_cap cap = {
	.codec_cap = &codec_cap,
};

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("Found broadcaster with ID 0x%06X and addr %s and sid 0x%02X\n", broadcast_id,
	       le_addr, info->sid);

	if (broadcast_assistant_conn == NULL /* Not requested by Broadcast Assistant */ ||
	    (req_recv_state != NULL && bt_addr_le_eq(info->addr, &req_recv_state->addr) &&
	     info->sid == req_recv_state->adv_sid &&
	     broadcast_id == req_recv_state->broadcast_id)) {

		/* Store info for PA sync parameters */
		memcpy(&broadcaster_info, info, sizeof(broadcaster_info));
		bt_addr_le_copy(&broadcaster_addr, info->addr);
		broadcaster_broadcast_id = broadcast_id;
		printk("broadcaster_broadcast_id = 0x%06X\n", broadcaster_broadcast_id);
		k_sem_give(&sem_broadcaster_found);
	}

	/* Stop parsing */
	return false;
}

static bool is_substring(const char *substr, const char *str)
{
	const size_t str_len = strlen(str);
	const size_t sub_str_len = strlen(substr);

	if (sub_str_len > str_len) {
		return false;
	}

	for (size_t pos = 0; pos < str_len; pos++) {
		if (pos + sub_str_len > str_len) {
			return false;
		}

		if (strncasecmp(substr, &str[pos], sub_str_len) == 0) {
			return true;
		}
	}

	return false;
}

static bool data_cb(struct bt_data *data, void *user_data)
{
	bool *device_found = user_data;
	char name[NAME_LEN] = {0};

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
	case BT_DATA_BROADCAST_NAME:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));

		if (is_substring(CONFIG_TARGET_BROADCAST_NAME, name)) {
			/* Device found */
			*device_found = true;
			return false;
		}
		return true;
	default:
		return true;
	}
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (info->interval != 0U) {
		/* call to bt_data_parse consumes netbufs so shallow clone for verbose output */

		/* If req_recv_state is not NULL then we have been requested by a broadcast
		 * assistant to sync to a specific broadcast source. In that case we do not apply
		 * our own broadcast name filter.
		 */
		if (req_recv_state == NULL && strlen(CONFIG_TARGET_BROADCAST_NAME) > 0U) {
			bool device_found = false;
			struct net_buf_simple buf_copy;

			net_buf_simple_clone(ad, &buf_copy);
			bt_data_parse(&buf_copy, data_cb, &device_found);

			if (!device_found) {
				return;
			}
		}
		bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
	}
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == pa_sync ||
	    (req_recv_state != NULL && bt_addr_le_eq(info->addr, &req_recv_state->addr) &&
	     info->sid == req_recv_state->adv_sid)) {
		printk("PA sync %p synced for broadcast sink with broadcast ID 0x%06X\n", sync,
		       broadcaster_broadcast_id);

		if (pa_sync == NULL) {
			pa_sync = sync;
		}

		k_work_cancel_delayable(&pa_timer);
		k_sem_give(&sem_pa_synced);
	}
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == pa_sync) {
		printk("PA sync %p lost with reason 0x%02X\n", sync, info->reason);
		pa_sync = NULL;

		k_sem_give(&sem_pa_sync_lost);

		if (info->reason != BT_HCI_ERR_LOCALHOST_TERM_CONN && req_recv_state != NULL) {
			int err;

			if (big_synced) {
				err = bt_bap_broadcast_sink_stop(broadcast_sink);
				if (err != 0) {
					printk("Failed to stop Broadcast Sink: %d\n", err);

					return;
				}
			}

			err = bt_bap_scan_delegator_rem_src(req_recv_state->src_id);
			if (err != 0) {
				printk("Failed to remove source: %d\n", err);

				return;
			}
		}
	}
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
};


static int init(void)
{
	const struct bt_bap_pacs_register_param pacs_param = {
		.snk_pac = true,
		.snk_loc = true,
	};
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	err = bt_pacs_register(&pacs_param);
	if (err) {
		printk("Could not register PACS (err %d)\n", err);
		return err;
	}

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err) {
		printk("Capability register failed (err %d)\n", err);
		return err;
	}

	err = bt_bap_scan_delegator_register(&scan_delegator_cbs);
	if (err) {
		printk("Scan delegator register failed (err %d)\n", err);
		return err;
	}

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	bt_le_scan_cb_register(&bap_scan_cb);

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i].stream.ops = &stream_ops;
	}

	/* Initialize ring buffers and USB */
#if defined(CONFIG_USB_DEVICE_AUDIO)
	const struct device *hs_dev = DEVICE_DT_GET(DT_NODELABEL(hs_0));
	static const struct usb_audio_ops usb_ops = {
		.data_request_cb = usb_data_request_cb,
		.data_written_cb = usb_data_written_cb,
	};

	if (!device_is_ready(hs_dev)) {
		printk("Cannot get USB Headset Device\n");
		return -EIO;
	}

	usb_audio_register(hs_dev, &usb_ops);
	err = usb_enable(NULL);
	if (err && err != -EALREADY) {
		printk("Failed to enable USB\n");
		return err;
	}
#endif /* defined(CONFIG_USB_DEVICE_AUDIO) */

	return 0;
}

static int reset(void)
{
	int err;

	printk("Reset\n");

	bis_index_bitfield = 0U;
	requested_bis_sync = 0U;
	req_recv_state = NULL;
	big_synced = false;
	base_received = false;
	stream_count = 0U;
	(void)memset(sink_broadcast_code, 0, sizeof(sink_broadcast_code));
	(void)memset(&broadcaster_info, 0, sizeof(broadcaster_info));
	(void)memset(&broadcaster_addr, 0, sizeof(broadcaster_addr));
	broadcaster_broadcast_id = BT_BAP_INVALID_BROADCAST_ID;

	if (broadcast_sink != NULL) {
		err = bt_bap_broadcast_sink_delete(broadcast_sink);
		if (err) {
			printk("Deleting broadcast sink failed (err %d)\n", err);

			return err;
		}

		broadcast_sink = NULL;
	}

	if (pa_sync != NULL) {
		bt_le_per_adv_sync_delete(pa_sync);
		if (err) {
			printk("Deleting PA sync failed (err %d)\n", err);

			return err;
		}

		pa_sync = NULL;
	}

	k_sem_reset(&sem_broadcaster_found);
	k_sem_reset(&sem_pa_synced);
	k_sem_reset(&sem_base_received);
	k_sem_reset(&sem_syncable);
	k_sem_reset(&sem_pa_sync_lost);
	k_sem_reset(&sem_broadcast_code_received);
	k_sem_reset(&sem_bis_sync_requested);
	k_sem_reset(&sem_stream_connected);
	k_sem_reset(&sem_stream_started);
	k_sem_reset(&sem_broadcast_sink_stopped);

	return 0;
}

static int start_adv(void)
{
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			      BT_UUID_16_ENCODE(BT_UUID_BASS_VAL),
			      BT_UUID_16_ENCODE(BT_UUID_PACS_VAL)),
		BT_DATA_BYTES(BT_DATA_SVC_DATA16, BT_UUID_16_ENCODE(BT_UUID_BASS_VAL)),
		BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
			sizeof(CONFIG_BT_DEVICE_NAME) - 1),
	};
	int err;

	/* Create a connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &ext_adv);
	if (err != 0) {
		printk("Failed to create advertising set (err %d)\n", err);

		return err;
	}

	err = bt_le_ext_adv_set_data(ext_adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		printk("Failed to set advertising data (err %d)\n", err);

		return err;
	}

	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		printk("Failed to start advertising set (err %d)\n", err);

		return err;
	}

	return 0;
}

static int stop_adv(void)
{
	int err;

	err = bt_le_ext_adv_stop(ext_adv);
	if (err != 0) {
		printk("Failed to stop advertising set (err %d)\n", err);

		return err;
	}

	err = bt_le_ext_adv_delete(ext_adv);
	if (err != 0) {
		printk("Failed to delete advertising set (err %d)\n", err);

		return err;
	}

	ext_adv = NULL;

	return 0;
}

static int pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = broadcaster_info.sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	return bt_le_per_adv_sync_create(&create_params, &pa_sync);
}

int main(void)
{
	int err;

	err = init();
	if (err) {
		printk("Init failed (err %d)\n", err);
		return 0;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(streams_p); i++) {
		streams_p[i] = &streams[i].stream;
#if defined(CONFIG_LIBLC3)
		k_mutex_init(&streams[i].lc3_decoder_mutex);
#endif /* defined(CONFIG_LIBLC3) */
	}

	while (true) {
		uint32_t sync_bitfield;

		err = reset();
		if (err != 0) {
			printk("Resetting failed: %d - Aborting\n", err);

			return 0;
		}

		if (IS_ENABLED(CONFIG_SCAN_OFFLOAD)) {
			if (broadcast_assistant_conn == NULL) {
				k_sem_reset(&sem_connected);

				printk("Starting advertising\n");
				/* Stop advertising before starting if needed */
				if (ext_adv != NULL) {
					err = stop_adv();
					if (err != 0) {
						printk("Unable to stop advertising: %d\n", err);

						return 0;
					}
				}
				err = start_adv();
				if (err != 0) {
					printk("Unable to start advertising connectable: %d\n",
					       err);

					return 0;
				}

				printk("Waiting for Broadcast Assistant\n");
				err = k_sem_take(&sem_connected, ADV_TIMEOUT);
				if (err != 0) {
					printk("No Broadcast Assistant connected\n");

					err = stop_adv();
					if (err != 0) {
						printk("Unable to stop advertising: %d\n", err);

						return 0;
					}
				}
			}

			if (broadcast_assistant_conn != NULL) {
				k_sem_reset(&sem_pa_request);
				k_sem_reset(&sem_past_request);
				k_sem_reset(&sem_disconnected);

				/* Wait for the PA request to determine if we
				 * should start scanning, or wait for PAST
				 */
				printk("Waiting for PA sync request\n");
				err = k_sem_take(&sem_pa_request,
						 BROADCAST_ASSISTANT_TIMEOUT);
				if (err != 0) {
					printk("sem_pa_request timed out, resetting\n");
					continue;
				}

				if (k_sem_take(&sem_past_request, K_NO_WAIT) == 0) {
					goto wait_for_pa_sync;
				} /* else continue with scanning below */
			}
		}

		if (strlen(CONFIG_TARGET_BROADCAST_NAME) > 0U) {
			printk("Scanning for broadcast sources containing "
			       "`" CONFIG_TARGET_BROADCAST_NAME "`\n");
		} else {
			printk("Scanning for broadcast sources\n");
		}

		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
		if (err != 0 && err != -EALREADY) {
			printk("Unable to start scan for broadcast sources: %d\n",
			       err);
			return 0;
		}

		printk("Waiting for Broadcaster\n");
		err = k_sem_take(&sem_broadcaster_found, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_broadcaster_found timed out, resetting\n");
			continue;
		}

		err = bt_le_scan_stop();
		if (err != 0) {
			printk("bt_le_scan_stop failed with %d, resetting\n", err);
			continue;
		}

		printk("Attempting to PA sync to the broadcaster with id 0x%06X\n",
		       broadcaster_broadcast_id);
		err = pa_sync_create();
		if (err != 0) {
			printk("Could not create Broadcast PA sync: %d, resetting\n", err);
			continue;
		}

wait_for_pa_sync:
		printk("Waiting for PA synced\n");
		err = k_sem_take(&sem_pa_synced, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_pa_synced timed out, resetting\n");
			continue;
		}

		printk("Broadcast source PA synced, creating Broadcast Sink\n");
		err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id,
						   &broadcast_sink);
		if (err != 0) {
			printk("Failed to create broadcast sink: %d\n", err);
			continue;
		}

		printk("Broadcast Sink created, waiting for BASE\n");
		err = k_sem_take(&sem_base_received, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_base_received timed out, resetting\n");
			continue;
		}

		printk("BASE received, waiting for syncable\n");
		err = k_sem_take(&sem_syncable, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_syncable timed out, resetting\n");
			continue;
		}

		/* sem_broadcast_code_received is also given if the
		 * broadcast is not encrypted
		 */
		printk("Waiting for broadcast code\n");
		err = k_sem_take(&sem_broadcast_code_received, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_broadcast_code_received timed out, resetting\n");
			continue;
		}

		printk("Waiting for BIS sync request\n");
		err = k_sem_take(&sem_bis_sync_requested, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_bis_sync_requested timed out, resetting\n");
			continue;
		}

		sync_bitfield = bis_index_bitfield & requested_bis_sync;
		stream_count = 0;
		for (int i = 0; i < BT_ISO_MAX_GROUP_ISO_COUNT; i++) {
			if ((sync_bitfield & BIT(i)) != 0) {
				stream_count++;
			}
		}

		printk("Syncing to broadcast with bitfield: 0x%08x = 0x%08x (bis_index) & 0x%08x "
		       "(req_bis_sync), stream_count = %u\n",
		       sync_bitfield, bis_index_bitfield, requested_bis_sync, stream_count);

		err = bt_bap_broadcast_sink_sync(broadcast_sink, sync_bitfield, streams_p,
						 sink_broadcast_code);
		if (err != 0) {
			printk("Unable to sync to broadcast source: %d\n", err);
			return 0;
		}

		printk("Waiting for stream(s) started\n");
		err = k_sem_take(&sem_big_synced, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_big_synced timed out, resetting\n");
			continue;
		}

		printk("Waiting for PA disconnected\n");
		k_sem_take(&sem_pa_sync_lost, K_FOREVER);

		printk("Waiting for sink to stop\n");
		err = k_sem_take(&sem_broadcast_sink_stopped, SEM_TIMEOUT);
		if (err != 0) {
			printk("sem_broadcast_sink_stopped timed out, resetting\n");
			continue;
		}
	}

	return 0;
}
