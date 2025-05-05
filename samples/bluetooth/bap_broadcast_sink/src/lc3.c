/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink LC3 extension
 *
 * This files handles all the LC3 related functionality for the sample
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>

#include <lc3.h>

#include "lc3.h"
#include "stream_rx.h"
#include "usb.h"

LOG_MODULE_REGISTER(lc3, CONFIG_LOG_DEFAULT_LEVEL);

#define LC3_ENCODER_STACK_SIZE 4096
#define LC3_ENCODER_PRIORITY   5

struct lc3_data {
	void *fifo_reserved; /* 1st word reserved for use by FIFO */
	struct net_buf *buf;
	struct stream_rx *stream;
	uint32_t ts;
	bool do_plc;
};

K_MEM_SLAB_DEFINE_STATIC(lc3_data_slab, sizeof(struct lc3_data), CONFIG_BT_ISO_RX_BUF_COUNT,
			 __alignof__(struct lc3_data));

static int16_t lc3_rx_buf[LC3_MAX_NUM_SAMPLES_MONO];
static K_FIFO_DEFINE(lc3_in_fifo);

/* We only want to send USB to left/right from a single stream. If we have 2 left streams, the
 * outgoing audio is going to be terrible.
 * Since a stream can contain stereo data, both of these may be the same stream.
 */
static struct stream_rx *usb_left_stream;
static struct stream_rx *usb_right_stream;

static int init_lc3_decoder(struct stream_rx *stream, uint32_t lc3_frame_duration_us,
			    uint32_t lc3_freq_hz)
{
	if (stream == NULL) {
		LOG_ERR("NULL stream to init LC3 decoder");
		return -EINVAL;
	}

	if (stream->lc3_decoder != NULL) {
		LOG_ERR("Already initialized");
		return -EALREADY;
	}

	if (lc3_freq_hz == 0U || lc3_frame_duration_us == 0U) {
		LOG_ERR("Invalid freq (%u) or frame duration (%u)", lc3_freq_hz,
			lc3_frame_duration_us);

		return -EINVAL;
	}

	LOG_INF("Initializing the LC3 decoder with %u us duration and %u Hz frequency",
		lc3_frame_duration_us, lc3_freq_hz);
	/* Create the decoder instance. This shall complete before stream_started() is called. */
	stream->lc3_decoder =
		lc3_setup_decoder(lc3_frame_duration_us, lc3_freq_hz,
				  IS_ENABLED(CONFIG_USE_USB_AUDIO_OUTPUT) ? USB_SAMPLE_RATE_HZ : 0,
				  &stream->lc3_decoder_mem);
	if (stream->lc3_decoder == NULL) {
		LOG_ERR("Failed to setup LC3 decoder - wrong parameters?\n");
		return -EINVAL;
	}

	LOG_INF("Initialized LC3 decoder for %p", stream);

	return 0;
}

static bool decode_frame(struct lc3_data *data, size_t frame_cnt)
{
	const struct stream_rx *stream = data->stream;
	const size_t total_frames = stream->lc3_chan_cnt * stream->lc3_frame_blocks_per_sdu;
	const uint16_t octets_per_frame = stream->lc3_octets_per_frame;
	struct net_buf *buf = data->buf;
	void *iso_data;
	int err;

	if (data->do_plc) {
		iso_data = NULL; /* perform PLC */

#if CONFIG_INFO_REPORTING_INTERVAL > 0
		if ((stream->reporting_info.lc3_decoded_cnt % CONFIG_INFO_REPORTING_INTERVAL) ==
		    0) {
			LOG_DBG("[%zu]: Performing PLC", stream->reporting_info.lc3_decoded_cnt);
		}
#endif /* CONFIG_INFO_REPORTING_INTERVAL > 0 */

		data->do_plc = false; /* clear flag */
	} else {
		iso_data = net_buf_pull_mem(data->buf, octets_per_frame);

#if CONFIG_INFO_REPORTING_INTERVAL > 0
		if ((stream->reporting_info.lc3_decoded_cnt % CONFIG_INFO_REPORTING_INTERVAL) ==
		    0) {
			LOG_DBG("[%zu]: Decoding frame of size %u (%u/%u)",
				stream->reporting_info.lc3_decoded_cnt, octets_per_frame,
				frame_cnt + 1, total_frames);
		}
#endif /* CONFIG_INFO_REPORTING_INTERVAL > 0 */
	}

	err = lc3_decode(stream->lc3_decoder, iso_data, octets_per_frame, LC3_PCM_FORMAT_S16,
			 lc3_rx_buf, 1);
	if (err < 0) {
		LOG_ERR("Failed to decode LC3 data (%u/%u - %u/%u)", frame_cnt + 1, total_frames,
			octets_per_frame * frame_cnt, buf->len);
		return false;
	}

	return true;
}

static int get_lc3_chan_alloc_from_index(const struct stream_rx *stream, uint8_t index,
					 enum bt_audio_location *chan_alloc)
{
#if defined(CONFIG_USE_USB_AUDIO_OUTPUT)
	const bool has_left = (stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_LEFT) != 0;
	const bool has_right = (stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_RIGHT) != 0;
	const bool is_mono = stream->lc3_chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO;
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
#else  /* !CONFIG_USE_USB_AUDIO_OUTPUT */
	return -EINVAL;
#endif /* CONFIG_USE_USB_AUDIO_OUTPUT */
}

static size_t decode_frame_block(struct lc3_data *data, size_t frame_cnt)
{
	const struct stream_rx *stream = data->stream;
	const uint8_t chan_cnt = stream->lc3_chan_cnt;
	size_t decoded_frames = 0U;

	for (uint8_t i = 0U; i < chan_cnt; i++) {
		/* We provide the total number of decoded frames to `decode_frame` for logging
		 * purposes
		 */
		if (decode_frame(data, frame_cnt + decoded_frames)) {
			decoded_frames++;

			if (IS_ENABLED(CONFIG_USE_USB_AUDIO_OUTPUT)) {
				enum bt_audio_location chan_alloc;
				int err;

				err = get_lc3_chan_alloc_from_index(stream, i, &chan_alloc);
				if (err != 0) {
					/* Not suitable for USB */
					continue;
				}

				/* We only want to left or right from one stream to USB */
				if ((chan_alloc == BT_AUDIO_LOCATION_FRONT_LEFT &&
				     stream != usb_left_stream) ||
				    (chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT &&
				     stream != usb_right_stream)) {
					continue;
				}

				/* TODO: Add support for properly support the presentation delay.
				 * For now we just send audio to USB as soon as we get it
				 */
				err = usb_add_frame_to_usb(chan_alloc, lc3_rx_buf,
							   sizeof(lc3_rx_buf), data->ts);
				if (err == -EINVAL) {
					continue;
				}
			}
		} else {
			/* If decoding failed, we clear the data to USB as it would contain
			 * invalid data
			 */
			if (IS_ENABLED(CONFIG_USE_USB_AUDIO_OUTPUT)) {
				usb_clear_frames_to_usb();
			}

			break;
		}
	}

	return decoded_frames;
}

static void do_lc3_decode(struct lc3_data *data)
{
	struct stream_rx *stream = data->stream;

	if (stream->lc3_decoder != NULL) {
		const uint8_t frame_blocks_per_sdu = stream->lc3_frame_blocks_per_sdu;
		size_t frame_cnt;

		frame_cnt = 0;
		for (uint8_t i = 0U; i < frame_blocks_per_sdu; i++) {
			const size_t decoded_frames = decode_frame_block(data, frame_cnt);

			if (decoded_frames == 0) {
				break;
			}

			frame_cnt += decoded_frames;
		}

#if CONFIG_INFO_REPORTING_INTERVAL > 0
		stream->reporting_info.lc3_decoded_cnt++;
#endif /* CONFIG_INFO_REPORTING_INTERVAL > 0 */
	}

	net_buf_unref(data->buf);
}

static void lc3_decoder_thread_func(void *arg1, void *arg2, void *arg3)
{
	while (true) {
		struct lc3_data *data = k_fifo_get(&lc3_in_fifo, K_FOREVER);
		struct stream_rx *stream = data->stream;

		if (stream->lc3_decoder == NULL) {
			LOG_WRN("Decoder is NULL, discarding data from FIFO");
			k_mem_slab_free(&lc3_data_slab, (void *)data);
			continue; /* Wait for new data */
		}

		do_lc3_decode(data);

		k_mem_slab_free(&lc3_data_slab, (void *)data);
	}
}

int lc3_enable(struct stream_rx *stream)
{
	const struct bt_audio_codec_cfg *codec_cfg = stream->stream.codec_cfg;
	uint32_t lc3_frame_duration_us;
	uint32_t lc3_freq_hz;
	int ret;

	if (codec_cfg->id != BT_HCI_CODING_FORMAT_LC3) {
		return -EINVAL;
	}

	ret = bt_audio_codec_cfg_get_freq(codec_cfg);
	if (ret >= 0) {
		ret = bt_audio_codec_cfg_freq_to_freq_hz(ret);

		if (ret > 0) {
			if (ret == 8000 || ret == 16000 || ret == 24000 || ret == 32000 ||
			    ret == 48000) {
				lc3_freq_hz = (uint32_t)ret;
			} else {
				LOG_ERR("Unsupported frequency for LC3: %d", ret);
				lc3_freq_hz = 0U;
			}
		} else {
			LOG_ERR("Invalid frequency: %d", ret);
			lc3_freq_hz = 0U;
		}
	} else {
		LOG_ERR("Could not get frequency: %d", ret);
		lc3_freq_hz = 0U;
	}

	if (lc3_freq_hz == 0U) {
		return -EINVAL;
	}

	ret = bt_audio_codec_cfg_get_frame_dur(codec_cfg);
	if (ret >= 0) {
		ret = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
		if (ret > 0) {
			lc3_frame_duration_us = (uint32_t)ret;
		} else {
			LOG_ERR("Invalid frame duration: %d", ret);
			lc3_frame_duration_us = 0U;
		}
	} else {
		LOG_ERR("Could not get frame duration: %d", ret);
		lc3_frame_duration_us = 0U;
	}

	if (lc3_frame_duration_us == 0U) {
		return -EINVAL;
	}

	ret = bt_audio_codec_cfg_get_chan_allocation(codec_cfg, &stream->lc3_chan_allocation, true);
	if (ret == 0) {
		stream->lc3_chan_cnt = bt_audio_get_chan_count(stream->lc3_chan_allocation);
	} else {
		LOG_DBG("Could not get channel allocation: %d", ret);
		stream->lc3_chan_cnt = 0U;
	}

	if (stream->lc3_chan_cnt == 0U) {
		return -EINVAL;
	}

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec_cfg, true);
	if (ret >= 0) {
		stream->lc3_frame_blocks_per_sdu = (uint8_t)ret;
	} else {
		LOG_ERR("Could not get frame blocks per SDU: %d", ret);
		stream->lc3_frame_blocks_per_sdu = 0U;
	}

	if (stream->lc3_frame_blocks_per_sdu == 0U) {
		return -EINVAL;
	}

	ret = bt_audio_codec_cfg_get_octets_per_frame(codec_cfg);
	if (ret >= 0) {
		stream->lc3_octets_per_frame = (uint16_t)ret;
	} else {
		LOG_ERR("Could not get octets per frame: %d", ret);
		stream->lc3_octets_per_frame = 0U;
	}

	if (stream->lc3_octets_per_frame == 0U) {
		return -EINVAL;
	}

	if (stream->lc3_decoder == NULL) {
		const int err = init_lc3_decoder(stream, lc3_frame_duration_us, lc3_freq_hz);

		if (err != 0) {
			LOG_ERR("Failed to init LC3 decoder: %d", err);

			return err;
		}
	}

	if (IS_ENABLED(CONFIG_USE_USB_AUDIO_OUTPUT)) {
		if ((stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_LEFT) != 0) {
			if (usb_left_stream == NULL) {
				LOG_INF("Setting USB left stream to %p", stream);
				usb_left_stream = stream;
			} else {
				LOG_WRN("Multiple left streams started");
			}
		}

		if ((stream->lc3_chan_allocation & BT_AUDIO_LOCATION_FRONT_RIGHT) != 0) {
			if (usb_right_stream == NULL) {
				LOG_INF("Setting USB right stream to %p", stream);
				usb_right_stream = stream;
			} else {
				LOG_WRN("Multiple right streams started");
			}
		}
	}

	return 0;
}

int lc3_disable(struct stream_rx *stream)
{
	if (stream->lc3_decoder == NULL) {
		return -EINVAL;
	}

	stream->lc3_decoder = NULL;

	if (IS_ENABLED(CONFIG_USB_DEVICE_AUDIO)) {
		if (usb_left_stream == stream) {
			usb_left_stream = NULL;
		}
		if (usb_right_stream == stream) {
			usb_right_stream = NULL;
		}
	}

	return 0;
}

void lc3_enqueue_for_decoding(struct stream_rx *stream, const struct bt_iso_recv_info *info,
			      struct net_buf *buf)
{
	const uint8_t frame_blocks_per_sdu = stream->lc3_frame_blocks_per_sdu;
	const uint16_t octets_per_frame = stream->lc3_octets_per_frame;
	const uint8_t chan_cnt = stream->lc3_chan_cnt;
	struct lc3_data *data;

	if (stream->lc3_decoder == NULL) {
		return;
	}

	/* Allocate a context that holds both the buffer and the stream so that we can
	 * send both of these values to the LC3 decoder thread as a single struct
	 * in a FIFO
	 */
	if (k_mem_slab_alloc(&lc3_data_slab, (void **)&data, K_NO_WAIT)) {
		LOG_WRN("Could not allocate LC3 data item");
		return;
	}

	if ((info->flags & BT_ISO_FLAGS_VALID) == 0) {
		data->do_plc = true;
	} else if (buf->len != (octets_per_frame * chan_cnt * frame_blocks_per_sdu)) {
		if (buf->len != 0U) {
			LOG_WRN("Expected %u frame blocks with %u channels of size %u, but "
				"length is %u",
				frame_blocks_per_sdu, chan_cnt, octets_per_frame, buf->len);
		}

		data->do_plc = true;
	}

	data->buf = net_buf_ref(buf);
	data->stream = stream;
	if (info->flags & BT_ISO_FLAGS_TS) {
		data->ts = info->ts;
	} else {
		data->ts = 0U;
	}

	k_fifo_put(&lc3_in_fifo, data);
}

int lc3_init(void)
{
	static K_KERNEL_STACK_DEFINE(lc3_decoder_thread_stack, 4096);
	const int lc3_decoder_thread_prio = K_PRIO_PREEMPT(5);
	static struct k_thread lc3_decoder_thread;
	static bool initialized;

	if (initialized) {
		return -EALREADY;
	}

	k_thread_create(&lc3_decoder_thread, lc3_decoder_thread_stack,
			K_KERNEL_STACK_SIZEOF(lc3_decoder_thread_stack), lc3_decoder_thread_func,
			NULL, NULL, NULL, lc3_decoder_thread_prio, 0, K_NO_WAIT);
	k_thread_name_set(&lc3_decoder_thread, "LC3 Decoder");

	LOG_INF("LC3 initialized");
	initialized = true;

	return 0;
}
