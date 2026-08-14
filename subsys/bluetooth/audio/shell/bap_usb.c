/**
 * @file
 * @brief Bluetooth Basic Audio Profile shell USB extension
 *
 * This files handles all the USB related functionality to audio in/out for the BAP shell
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/usb/usb_buf.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/clock.h>
#include <zephyr/toolchain.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/usb/usbd.h>

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#if defined(CONFIG_CLOCK_CONTROL_NRF)
#include <nrfx_clock.h>
#else
#include <nrfx_clock_hfclk.h>
#endif
#include <drivers/nrfx_errors.h>
#include <hal/nrf_clock.h>
#endif /* CONFIG_SOC_NRF5340_CPUAPP */

#include "audio.h"

LOG_MODULE_REGISTER(bap_usb, CONFIG_BT_BAP_STREAM_LOG_LEVEL);

#define USB_LOG_RATE           (30U * MSEC_PER_SEC) /* 30 seconds */
#define USB_FRAME_DURATION_US  1000U
#define USB_SAMPLE_CNT         ((USB_FRAME_DURATION_US * USB_SAMPLE_RATE) / USEC_PER_SEC)
#define USB_BYTES_PER_SAMPLE   sizeof(int16_t)
#define USB_MONO_FRAME_SIZE    (USB_SAMPLE_CNT * USB_BYTES_PER_SAMPLE)
#define USB_STEREO_FRAME_SIZE  (USB_MONO_FRAME_SIZE * USB_CHANNELS)

/* Both ring buffers hold interleaved stereo data, and all cursors into them are in USB frames
 * (left+right sample pairs) rather than in samples or octets.
 *
 * liblc3 is set up with an output sample rate of USB_SAMPLE_RATE for every stream (see
 * init_lc3_encoder() and init_lc3_decoder()), so a stream always produces and consumes PCM at
 * USB_SAMPLE_RATE regardless of its configured LC3 sample rate. Streams thus only differ in how
 * many USB frames a single LC3 frame covers: 120, 240, 360 or 480 for 2.5ms, 5ms, 7.5ms and 10ms
 * frame durations respectively. USB itself transfers USB_SAMPLE_CNT (48) frames every SOF.
 *
 * By sizing the ring buffers to a multiple of the least common multiple of all of those
 * (LCM(48, 120, 240, 360, 480) == 1440), and by keeping every cursor a multiple of its own step
 * size, neither an LC3 frame nor a USB transfer can ever straddle the end of a ring buffer. That
 * removes the need for any wrap handling, and lets both liblc3 and the USB DMA operate directly
 * on the ring buffers.
 *
 * Note that the 44.1kHz LC3 configurations have non-integer frame durations (8.16ms and 10.88ms)
 * which would break this. They cannot reach this code as stream_started_cb() rejects any
 * frequency that is not 8, 16, 24, 32 or 48kHz, but supporting them would require revisiting the
 * ring buffer sizing.
 */
#define USB_RING_ALIGN_FRAMES 1440U /* LCM(48, 120, 240, 360, 480) == 30ms */
#define USB_RING_FRAMES       (USB_RING_ALIGN_FRAMES * 2U) /* 60ms */
#define USB_RING_SAMPLES      (USB_RING_FRAMES * USB_CHANNELS)

/* Maximum number of USB frames covered by a single LC3 frame */
#define USB_MAX_FRAMES_PER_LC3_FRAME                                                               \
	((LC3_MAX_FRAME_DURATION_US * USB_SAMPLE_RATE) / USEC_PER_SEC)

BUILD_ASSERT((USB_RING_FRAMES % USB_RING_ALIGN_FRAMES) == 0U,
	     "The ring buffers must be a multiple of USB_RING_ALIGN_FRAMES");
BUILD_ASSERT((USB_RING_ALIGN_FRAMES % USB_SAMPLE_CNT) == 0U,
	     "A USB transfer must never straddle the end of a ring buffer");
BUILD_ASSERT((USB_RING_ALIGN_FRAMES % USB_MAX_FRAMES_PER_LC3_FRAME) == 0U,
	     "An LC3 frame must never straddle the end of a ring buffer");
BUILD_ASSERT((USB_STEREO_FRAME_SIZE % USB_BUF_GRANULARITY) == 0U,
	     "USB transfers out of the ring buffers must be a multiple of the DMA granularity");

#define IN_TERMINAL_ID  UAC2_ENTITY_ID(DT_NODELABEL(in_terminal))
#define OUT_TERMINAL_ID UAC2_ENTITY_ID(DT_NODELABEL(out_terminal))

#if defined(CONFIG_BT_AUDIO_RX)
static void usb_data_request(const struct device *dev);
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
static void usb_out_terminal_disabled(void);
#endif /* CONFIG_BT_AUDIO_TX */

size_t bap_usb_get_read_cnt(const struct shell_stream *sh_stream)
{
	return (USB_SAMPLE_CNT * sh_stream->lc3_frame_duration_us) / USEC_PER_MSEC;
}

/**
 * Round @p frames down to a multiple of @p step, so that a cursor that is snapped to another
 * cursor keeps the alignment that the ring buffer sizing relies on.
 */
static size_t usb_align_down(size_t frames, size_t step)
{
	__ASSERT(step != 0U, "Invalid step");
	__ASSERT((USB_RING_FRAMES % step) == 0U, "Step %zu does not divide the ring buffer", step);

	return frames - (frames % step);
}

/** Number of frames from @p from to @p to, going forwards through the ring buffer */
static size_t usb_frames_between(size_t from, size_t to)
{
	if (to >= from) {
		return to - from;
	}

	return to + (USB_RING_FRAMES - from);
}

/** Advance @p cursor by @p frames, wrapping around the end of the ring buffer */
static size_t usb_advance(size_t cursor, size_t frames)
{
	cursor += frames;

	if (cursor >= USB_RING_FRAMES) {
		cursor -= USB_RING_FRAMES;
	}

	__ASSERT(cursor < USB_RING_FRAMES, "Invalid cursor %zu", cursor);

	return cursor;
}

static bool in_terminal_enabled;
static bool out_terminal_enabled;
static void usb_terminal_update_cb(const struct device *dev, uint8_t terminal, bool enabled,
				   bool microframes, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(microframes);
	ARG_UNUSED(user_data);

	if (terminal == IN_TERMINAL_ID) {
		in_terminal_enabled = enabled;
	} else if (terminal == OUT_TERMINAL_ID) {
		out_terminal_enabled = enabled;
#if defined(CONFIG_BT_AUDIO_TX)
		if (!enabled) {
			usb_out_terminal_disabled();
		}
#endif /* CONFIG_BT_AUDIO_TX */
	} else {
		/* no-op */
	}
}

static void usb_sof_cb(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

#if defined CONFIG_BT_AUDIO_RX
	if (in_terminal_enabled) {
		usb_data_request(dev);
	} /* else no-op, but is mandatory to register */
#endif /* CONFIG_BT_AUDIO_RX */
}

#if defined CONFIG_BT_AUDIO_RX
/* Interleaved stereo ring buffer holding decoded audio on its way to the USB host.
 *
 * It has up to 2 producers (a single stream for each of the left and right channels, elected by
 * stream_started_cb()) that each have their own write cursor, and a single consumer (the USB SOF
 * handler). The buffer is written directly by liblc3 and read directly by the USB DMA.
 */
USB_STATIC_BUF_DEFINE(usb_in_ring_buf_mem, USB_RING_SAMPLES * USB_BYTES_PER_SAMPLE);
static int16_t *const usb_in_ring_buf = (int16_t *)usb_in_ring_buf_mem;
/* Sent when there is nothing to send. Kept separate from the ring buffer so that an underrun does
 * not discard data that a channel has already decoded.
 */
USB_STATIC_BUF_DEFINE(usb_in_silence_mem, USB_STEREO_FRAME_SIZE);
static int16_t *const usb_in_silence = (int16_t *)usb_in_silence_mem;
static size_t usb_in_left_write_cursor;
static size_t usb_in_right_write_cursor;
static size_t usb_in_read_cursor;
static bool usb_in_left_active;
static bool usb_in_right_active;
/* Number of consecutive underruns, used to bound how long a single starving channel may hold
 * back a channel that is still producing data
 */
static size_t usb_in_underrun_cnt;

/* Amount of data to keep between the read cursor and a write cursor, to absorb the jitter of the
 * incoming SDUs. Also used as the target when a channel has to be resynchronized.
 */
#define USB_IN_TARGET_PREFILL_FRAMES (USB_MAX_FRAMES_PER_LC3_FRAME * 2U) /* 20ms */

/* Number of consecutive underruns after which a starving channel is sent as silence rather than
 * blocking the channels that do produce data. A stream that stops without being deactivated would
 * otherwise mute the other channel indefinitely.
 */
#define USB_IN_MAX_UNDERRUNS 100U /* 100ms */

/* usb_in_data_mutex guards usb_in_ring_buf and all of the cursors and flags above */
static K_MUTEX_DEFINE(usb_in_data_mutex);
#define USB_IN_DATA_MUTEX_TIMEOUT K_MSEC(1)

/** Number of frames that @p write_cursor is ahead of the USB read cursor */
static size_t usb_in_chan_fill(size_t write_cursor)
{
	return usb_frames_between(usb_in_read_cursor, write_cursor);
}

/* USB consumer callback, called every 1ms, consumes USB_SAMPLE_CNT frames from the ring buffer */
static void usb_data_request(const struct device *dev)
{
	int16_t *pcm_buf;
	bool left_active;
	bool right_active;
	bool starving = false;
	bool give_up;
	bool have_data;
	int err;

	err = k_mutex_lock(&usb_in_data_mutex, USB_IN_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_in_data_mutex for USB data request: %d", err);
		return;
	}

	left_active = usb_in_left_active;
	right_active = usb_in_right_active;

	/* A channel that has starved for too long is ignored entirely, so that a channel which
	 * does produce data is not held back indefinitely. This bounds the effect of a producer
	 * that stops without deactivating its channel.
	 */
	give_up = usb_in_underrun_cnt >= USB_IN_MAX_UNDERRUNS;

	if (left_active && usb_in_chan_fill(usb_in_left_write_cursor) < USB_SAMPLE_CNT) {
		left_active = !give_up;
		starving = true;
	}

	if (right_active && usb_in_chan_fill(usb_in_right_write_cursor) < USB_SAMPLE_CNT) {
		right_active = !give_up;
		starving = true;
	}

	/* Only consume data that every channel that is still considered active has produced */
	have_data = (left_active || right_active) && (!starving || give_up);

	if (have_data) {
		static size_t cnt;

		pcm_buf = &usb_in_ring_buf[usb_in_read_cursor * USB_CHANNELS];

		if (left_active != right_active) {
			/* Duplicate the single active channel to both. This is the only per-sample
			 * operation left on this path.
			 */
			const size_t src = left_active ? 0U : 1U;
			const size_t dst = left_active ? 1U : 0U;

			for (size_t i = 0U; i < USB_SAMPLE_CNT; i++) {
				pcm_buf[(i * USB_CHANNELS) + dst] =
					pcm_buf[(i * USB_CHANNELS) + src];
			}
		}

		usb_in_read_cursor = usb_advance(usb_in_read_cursor, USB_SAMPLE_CNT);

		if (!starving) {
			usb_in_underrun_cnt = 0U;
		}

		cnt++;
		LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "[%zu]: Sending USB audio", cnt);
	} else {
		static size_t cnt;

		/* Underrun. Send silence and leave the read cursor alone, so that the data that is
		 * still being decoded is not skipped.
		 */
		pcm_buf = usb_in_silence;
		(void)memset(pcm_buf, 0, USB_STEREO_FRAME_SIZE);

		if (usb_in_underrun_cnt < USB_IN_MAX_UNDERRUNS) {
			usb_in_underrun_cnt++;
		}

		cnt++;
		LOG_WRN_RATELIMIT_RATE(USB_LOG_RATE, "[%zu]: Sending silent USB audio", cnt);
	}

	err = k_mutex_unlock(&usb_in_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_in_data_mutex: %d", err);

	err = usbd_uac2_send(dev, IN_TERMINAL_ID, pcm_buf, USB_STEREO_FRAME_SIZE);
	if (err != 0) {
		static size_t cnt;

		cnt++;
		LOG_ERR_RATELIMIT_RATE(USB_LOG_RATE, "Failed to send USB audio: %d (%zu)", err,
				       cnt);
	}
}

static void usb_buf_release_cb(const struct device *dev, uint8_t terminal, void *buf,
			       void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(buf);
	ARG_UNUSED(user_data);

	/* The buffer is part of usb_in_ring_buf and is not owned by the USB stack */
}

static size_t *usb_in_write_cursor(enum bt_audio_location chan_alloc)
{
	if (chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT) {
		return &usb_in_right_write_cursor;
	}

	/* Mono is stored in, and sent from, the left channel */
	return &usb_in_left_write_cursor;
}

static size_t usb_in_chan_offset(enum bt_audio_location chan_alloc)
{
	return chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT ? 1U : 0U;
}

void bap_usb_activate_in_chan(enum bt_audio_location chan_alloc)
{
	const bool is_right = chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT;
	size_t *cursor = usb_in_write_cursor(chan_alloc);
	int err;

	err = k_mutex_lock(&usb_in_data_mutex, USB_IN_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN("Failed to lock usb_in_data_mutex to activate channel: %d", err);
		return;
	}

	/* Park the cursor at the consumer. The first claim resynchronizes it to the target
	 * prefill using the frame size of the stream, which is not known here.
	 */
	*cursor = usb_in_read_cursor;

	if (is_right) {
		usb_in_right_active = true;
	} else {
		usb_in_left_active = true;
	}

	err = k_mutex_unlock(&usb_in_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_in_data_mutex: %d", err);

	LOG_INF("Activated USB IN channel 0x%08X", (uint32_t)chan_alloc);
}

void bap_usb_deactivate_in_chan(enum bt_audio_location chan_alloc)
{
	const bool is_right = chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT;
	int err;

	err = k_mutex_lock(&usb_in_data_mutex, USB_IN_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN("Failed to lock usb_in_data_mutex to deactivate channel: %d", err);
		return;
	}

	if (is_right) {
		usb_in_right_active = false;
	} else {
		usb_in_left_active = false;
	}

	usb_in_underrun_cnt = 0U;

	err = k_mutex_unlock(&usb_in_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_in_data_mutex: %d", err);

	LOG_INF("Deactivated USB IN channel 0x%08X", (uint32_t)chan_alloc);
}

/**
 * Place a channel at USB_IN_TARGET_PREFILL_FRAMES ahead of the consumer, and fill the frames it
 * skips over with silence so that the consumer never sends stale ring buffer content.
 *
 * The cursor is rounded up to a multiple of @p sample_cnt so that it stays at or ahead of the
 * consumer, and so that the LC3 frames written from it never straddle the end of the ring buffer.
 */
static size_t usb_in_resync(enum bt_audio_location chan_alloc, size_t sample_cnt)
{
	const size_t chan_offset = usb_in_chan_offset(chan_alloc);
	size_t cursor = usb_align_down(usb_in_read_cursor, sample_cnt);
	size_t silence_cnt;

	if (cursor != usb_in_read_cursor) {
		cursor = usb_advance(cursor, sample_cnt);
	}

	cursor = usb_advance(cursor, usb_align_down(USB_IN_TARGET_PREFILL_FRAMES, sample_cnt));

	silence_cnt = usb_frames_between(usb_in_read_cursor, cursor);
	for (size_t i = 0U; i < silence_cnt; i++) {
		const size_t frame = usb_advance(usb_in_read_cursor, i);

		usb_in_ring_buf[(frame * USB_CHANNELS) + chan_offset] = 0;
	}

	return cursor;
}

int16_t *bap_usb_claim_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt)
{
	size_t *cursor = usb_in_write_cursor(chan_alloc);
	int16_t *frame;
	size_t fill;
	int err;

	if (sample_cnt == 0U || sample_cnt > USB_MAX_FRAMES_PER_LC3_FRAME ||
	    (USB_RING_FRAMES % sample_cnt) != 0U) {
		LOG_WRN_RATELIMIT("Invalid sample count %zu", sample_cnt);

		return NULL;
	}

	err = k_mutex_lock(&usb_in_data_mutex, USB_IN_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_in_data_mutex to claim frame: %d", err);

		return NULL;
	}

	fill = usb_in_chan_fill(*cursor);

	/* Resynchronize the channel if it has fallen behind the consumer (which has already sent
	 * silence for the data being decoded now), if it has run so far ahead that it is about to
	 * overwrite data that has not been sent yet, or if it is not aligned to its own frame size
	 * (which happens on the first frame after the channel was activated).
	 */
	if (fill < sample_cnt || fill > (USB_RING_FRAMES - USB_MAX_FRAMES_PER_LC3_FRAME) ||
	    (*cursor % sample_cnt) != 0U) {
		*cursor = usb_in_resync(chan_alloc, sample_cnt);

		LOG_WRN_RATELIMIT_RATE(USB_LOG_RATE,
				       "Resynchronized USB IN channel 0x%08X (fill was %zu)",
				       (uint32_t)chan_alloc, fill);
	}

	frame = &usb_in_ring_buf[(*cursor * USB_CHANNELS) + usb_in_chan_offset(chan_alloc)];

	err = k_mutex_unlock(&usb_in_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_in_data_mutex: %d", err);

	return frame;
}

void bap_usb_release_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt)
{
	size_t *cursor = usb_in_write_cursor(chan_alloc);
	static size_t cnt;
	int err;

	err = k_mutex_lock(&usb_in_data_mutex, USB_IN_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_in_data_mutex to release frame: %d", err);
		return;
	}

	*cursor = usb_advance(*cursor, sample_cnt);

	err = k_mutex_unlock(&usb_in_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_in_data_mutex: %d", err);

	cnt++;
	LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "[%zu]: Added USB audio frame", cnt);
}
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
/* Interleaved stereo ring buffer holding audio received from the USB host.
 *
 * It has a single producer (the USB OUT endpoint, which writes into it by DMA) and 0 or more
 * consumers, one per TX stream, that each have their own read cursor and that may consume at
 * different rates. The buffer is read directly by liblc3.
 */
USB_STATIC_BUF_DEFINE(usb_out_ring_buf_mem, USB_RING_SAMPLES * USB_BYTES_PER_SAMPLE);
static int16_t *const usb_out_ring_buf = (int16_t *)usb_out_ring_buf_mem;
/* Points to the oldest/uninitialized data */
static size_t usb_out_write_cursor;
/* Position that has been handed to the USB stack but not yet received. The UAC2 class may have
 * up to 2 transfers queued at a time, so this may be ahead of usb_out_write_cursor.
 */
static size_t usb_out_pending_cursor;
/* Number of frames in a single USB transfer. This is the wMaxPacketSize of the OUT endpoint, and
 * is thus USB_SAMPLE_CNT when operating at full speed, but only an eighth of that at high speed
 * where a transfer covers a microframe rather than a frame.
 */
static size_t usb_out_slot_frames = USB_SAMPLE_CNT;

/* Amount of data a stream aims to keep between itself and the write cursor. Used when a stream
 * starts, and when it has to be resynchronized because it was about to be overwritten.
 */
#define USB_OUT_TARGET_PREFILL_FRAMES (USB_MAX_FRAMES_PER_LC3_FRAME * 2U) /* 20ms */

/* usb_out_data_mutex guards usb_out_ring_buf, the cursors above and shell_stream.tx.usb_read_cursor
 */
static K_MUTEX_DEFINE(usb_out_data_mutex);
#define USB_OUT_DATA_MUTEX_TIMEOUT K_MSEC(1)

static void usb_out_terminal_disabled(void)
{
	int err;

	err = k_mutex_lock(&usb_out_data_mutex, USB_OUT_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN("Failed to lock usb_out_data_mutex to disable terminal: %d", err);
		return;
	}

	/* Any queued transfers are discarded by the USB stack, so the slots that were handed out
	 * become available again
	 */
	usb_out_pending_cursor = usb_out_write_cursor;
	usb_out_slot_frames = USB_SAMPLE_CNT;

	err = k_mutex_unlock(&usb_out_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_out_data_mutex: %d", err);
}

/** Move @p cursor @p frames backwards, wrapping around the start of the ring buffer */
static size_t usb_retreat(size_t cursor, size_t frames)
{
	__ASSERT(frames <= USB_RING_FRAMES, "Invalid frame count %zu", frames);

	if (cursor >= frames) {
		return cursor - frames;
	}

	return cursor + (USB_RING_FRAMES - frames);
}

/** Number of frames that @p sh_stream can still read before catching up with the producer */
static size_t usb_out_stream_avail(const struct shell_stream *sh_stream)
{
	return usb_frames_between(sh_stream->tx.usb_read_cursor, usb_out_write_cursor);
}

static void stream_cb(struct shell_stream *sh_stream, void *user_data)
{
	ARG_UNUSED(user_data);

	if (sh_stream->is_tx && sh_stream->tx.active) {
		const size_t read_cnt = bap_usb_get_read_cnt(sh_stream);
		size_t avail;

		if (read_cnt == 0U) {
			return;
		}

		avail = usb_out_stream_avail(sh_stream);

		/* If the producer is about to overwrite data that this stream has not consumed
		 * yet, move the stream forwards to the oldest data that is still valid. Each
		 * stream is evaluated against its own margin, as they may consume at different
		 * rates, and a stream that is not reading at all must not hold back the producer.
		 */
		if (avail > (USB_RING_FRAMES - USB_MAX_FRAMES_PER_LC3_FRAME)) {
			/* Drop the backlog and keep the most recent data, so that the stream does
			 * not immediately fall behind again
			 */
			sh_stream->tx.usb_read_cursor =
				usb_retreat(usb_align_down(usb_out_write_cursor, read_cnt),
					    usb_align_down(USB_OUT_TARGET_PREFILL_FRAMES,
							   read_cnt));

			LOG_WRN_RATELIMIT_RATE(USB_LOG_RATE,
					       "Resynchronized USB OUT stream %p (avail was %zu)",
					       (void *)sh_stream, avail);
		}
	}
}

void bap_usb_tx_stream_started(struct shell_stream *sh_stream)
{
	const size_t read_cnt = bap_usb_get_read_cnt(sh_stream);
	int err;

	if (read_cnt == 0U) {
		LOG_WRN("Invalid frame duration %u for stream %p",
			sh_stream->lc3_frame_duration_us, (void *)sh_stream);
		return;
	}

	__ASSERT((USB_RING_FRAMES % read_cnt) == 0U,
		 "Read count %zu does not divide the ring buffer", read_cnt);

	err = k_mutex_lock(&usb_out_data_mutex, USB_OUT_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN("Failed to lock usb_out_data_mutex to start stream: %d", err);
		return;
	}

	/* Start at the producer so that only data received after the stream started is sent,
	 * rather than at whatever the union with the RX state left in the field.
	 */
	sh_stream->tx.usb_read_cursor = usb_align_down(usb_out_write_cursor, read_cnt);
	sh_stream->tx.usb_needs_prefill = true;
	sh_stream->tx.usb_underrun = false;

	err = k_mutex_unlock(&usb_out_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_out_data_mutex: %d", err);
}

static void *usb_get_recv_buf_cb(const struct device *dev, uint8_t terminal, uint16_t size,
				 void *user_data)
{
	size_t frame_cnt;
	void *buf;
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	if (!out_terminal_enabled) {
		return NULL;
	}

	/* The USB DMA may write up to size octets, so the slot handed out below must be that
	 * large. size is the wMaxPacketSize of the endpoint and thus constant while the terminal
	 * is enabled, which lets usb_data_recv_cb() commit the same amount.
	 */
	frame_cnt = size / (USB_CHANNELS * USB_BYTES_PER_SAMPLE);
	if (frame_cnt == 0U || frame_cnt > USB_SAMPLE_CNT ||
	    (size % (USB_CHANNELS * USB_BYTES_PER_SAMPLE)) != 0U ||
	    (USB_RING_FRAMES % frame_cnt) != 0U) {
		LOG_WRN_RATELIMIT("Unsupported receive buffer size %u", size);

		return NULL;
	}

	err = k_mutex_lock(&usb_out_data_mutex, USB_OUT_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_out_data_mutex to get receive buffer: %d",
				  err);

		return NULL;
	}

	if (frame_cnt != usb_out_slot_frames) {
		/* Keep the cursors aligned to the new transfer size */
		usb_out_slot_frames = frame_cnt;
		usb_out_pending_cursor = usb_align_down(usb_out_write_cursor, frame_cnt);
		usb_out_write_cursor = usb_out_pending_cursor;
	}

	/* Hand out the next unused slot in the ring buffer, so that the USB DMA writes directly
	 * into it. The slot is committed by usb_data_recv_cb().
	 */
	buf = &usb_out_ring_buf[usb_out_pending_cursor * USB_CHANNELS];
	usb_out_pending_cursor = usb_advance(usb_out_pending_cursor, frame_cnt);

	err = k_mutex_unlock(&usb_out_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_out_data_mutex: %d", err);

	return buf;
}

static void usb_data_recv_cb(const struct device *dev, uint8_t terminal, void *buf, uint16_t size,
			     void *user_data)
{
	static size_t cnt;
	size_t frame_cnt;
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	if (buf == NULL) {
		return;
	}

	err = k_mutex_lock(&usb_out_data_mutex, USB_OUT_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_out_data_mutex for new USB data: %d", err);
		return;
	}

	/* The data has been written into the ring buffer by DMA already, so all that is left is
	 * to make it available to the consumers. The host may send a short packet, in which case
	 * the remainder of the slot is zero-filled; the entire slot is always committed so that
	 * the cursors keep the alignment that the ring buffer sizing relies on.
	 */
	frame_cnt = MIN(size / (USB_CHANNELS * USB_BYTES_PER_SAMPLE), usb_out_slot_frames);
	if (frame_cnt < usb_out_slot_frames) {
		int16_t *pcm = (int16_t *)buf;

		(void)memset(&pcm[frame_cnt * USB_CHANNELS], 0,
			     (usb_out_slot_frames - frame_cnt) * USB_CHANNELS *
				     USB_BYTES_PER_SAMPLE);

		LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "Received short USB packet of %u octets",
				       size);
	}

	usb_out_write_cursor = usb_advance(usb_out_write_cursor, usb_out_slot_frames);

	/* Move any stream that is about to be overwritten forwards */
	bap_foreach_stream(stream_cb, NULL);

	err = k_mutex_unlock(&usb_out_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_out_data_mutex: %d", err);

	cnt++;
	LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "USB Data received (count = %zu)", cnt);
}

bool bap_usb_can_get_full_sdu(struct shell_stream *sh_stream)
{
	const size_t read_cnt = bap_usb_get_read_cnt(sh_stream);
	const size_t retrieve_cnt = read_cnt * sh_stream->lc3_frame_blocks_per_sdu;
	size_t avail;
	int err;

	if (read_cnt == 0U || retrieve_cnt == 0U) {
		return false;
	}

	err = k_mutex_lock(&usb_out_data_mutex, USB_OUT_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_out_data_mutex to validate SDU "
				  "availability: %d", err);
		return false;
	}

	avail = usb_out_stream_avail(sh_stream);

	err = k_mutex_unlock(&usb_out_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_out_data_mutex: %d", err);

	/* For the first SDU, we want to wait until we can send at least 2 SDUs to help reduce
	 * issues at the cost of presentation delay. This is tracked per stream, as streams may
	 * start at different times and consume at different rates.
	 */
	if (sh_stream->tx.usb_needs_prefill) {
		if (avail < (retrieve_cnt * 2U)) {
			return false;
		}

		sh_stream->tx.usb_needs_prefill = false;
	}

	if (avail < retrieve_cnt) {
		/* Not enough for a frame yet */
		if (!sh_stream->tx.usb_underrun) {
			LOG_WRN_RATELIMIT("Ring buffer (%zu/%u) does not contain enough for an "
					  "entire SDU %zu for channel allocation 0x%08X",
					  avail, USB_RING_FRAMES, retrieve_cnt,
					  (uint32_t)sh_stream->lc3_chan_allocation);
		}

		sh_stream->tx.usb_underrun = true;

		return false;
	}

	sh_stream->tx.usb_underrun = false;

	return true;
}

const int16_t *bap_usb_claim_frame_block(struct shell_stream *sh_stream)
{
	const size_t read_cnt = bap_usb_get_read_cnt(sh_stream);
	const int16_t *block;
	int err;

	if (read_cnt == 0U) {
		return NULL;
	}

	err = k_mutex_lock(&usb_out_data_mutex, USB_OUT_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_out_data_mutex to claim frame block: %d",
				  err);

		return NULL;
	}

	__ASSERT((sh_stream->tx.usb_read_cursor % read_cnt) == 0U,
		 "Misaligned cursor %zu for read count %zu", sh_stream->tx.usb_read_cursor,
		 read_cnt);
	__ASSERT(sh_stream->tx.usb_read_cursor < USB_RING_FRAMES, "Invalid cursor %zu",
		 sh_stream->tx.usb_read_cursor);

	/* The ring buffer size is a multiple of read_cnt, so the frame block never straddles the
	 * end of the ring buffer and can be handed to liblc3 as-is.
	 */
	block = &usb_out_ring_buf[sh_stream->tx.usb_read_cursor * USB_CHANNELS];

	err = k_mutex_unlock(&usb_out_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_out_data_mutex: %d", err);

	return block;
}

void bap_usb_release_frame_block(struct shell_stream *sh_stream)
{
	const size_t read_cnt = bap_usb_get_read_cnt(sh_stream);
	int err;

	if (read_cnt == 0U) {
		return;
	}

	err = k_mutex_lock(&usb_out_data_mutex, USB_OUT_DATA_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_WRN_RATELIMIT("Failed to lock usb_out_data_mutex to release frame block: %d",
				  err);
		return;
	}

	sh_stream->tx.usb_read_cursor = usb_advance(sh_stream->tx.usb_read_cursor, read_cnt);

	err = k_mutex_unlock(&usb_out_data_mutex);
	__ASSERT(err == 0, "Failed to unlock usb_out_data_mutex: %d", err);
}
#endif /* CONFIG_BT_AUDIO_TX */

static int bap_usbd_setup_device(struct usbd_context *const bap_usbd)
{
	static const uint8_t attributes =
		(IS_ENABLED(CONFIG_BT_BAP_SHELL_USB_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0U) |
		(IS_ENABLED(CONFIG_BT_BAP_SHELL_USB_REMOTE_WAKEUP) ? USB_SCD_REMOTE_WAKEUP : 0U);
	USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
	USBD_CONFIGURATION_DEFINE(bap_usb_fs_config, attributes, CONFIG_BT_BAP_SHELL_USB_MAX_POWER,
				  &fs_cfg_desc);
	USBD_DESC_PRODUCT_DEFINE(bap_usb_product, CONFIG_BT_BAP_SHELL_USB_PRODUCT);
	USBD_DESC_MANUFACTURER_DEFINE(bap_usb_mfr, "Zephyr Project");
	USBD_DESC_LANG_DEFINE(bap_usb_lang);
	const uint8_t class_cfg = 0x01U;
	const uint8_t subclass = 0x02U;
	const uint8_t protocol = 0x01U;

	int err;

	err = usbd_add_descriptor(bap_usbd, &bap_usb_lang);
	if (err != 0) {
		LOG_ERR("Failed to initialize language descriptor: %d", err);

		return err;
	}

	err = usbd_add_descriptor(bap_usbd, &bap_usb_mfr);
	if (err != 0) {
		LOG_ERR("Failed to initialize manufacturer descriptor: %d", err);

		return err;
	}

	err = usbd_add_descriptor(bap_usbd, &bap_usb_product);
	if (err != 0) {
		LOG_ERR("Failed to initialize product descriptor: %d", err);

		return err;
	}

	if (IS_ENABLED(CONFIG_HWINFO)) {
		USBD_DESC_SERIAL_NUMBER_DEFINE(bap_usb_sn);

		err = usbd_add_descriptor(bap_usbd, &bap_usb_sn);
		if (err != 0) {
			LOG_ERR("Failed to initialize serial number descriptor: %d", err);

			return err;
		}
	}

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(bap_usbd) == USBD_SPEED_HS) {
		USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");
		USBD_CONFIGURATION_DEFINE(bap_usb_hs_config, attributes,
					  CONFIG_BT_BAP_SHELL_USB_MAX_POWER, &hs_cfg_desc);

		LOG_DBG("Setting up High-Speed USB");

		err = usbd_add_configuration(bap_usbd, USBD_SPEED_HS, &bap_usb_hs_config);
		if (err != 0) {
			LOG_ERR("Failed to add High-Speed configuration: %d", err);

			return err;
		}

		err = usbd_register_all_classes(bap_usbd, USBD_SPEED_HS, class_cfg, NULL);
		if (err != 0) {
			LOG_ERR("Failed to add register High-Speed classes: %d", err);

			return err;
		}

		err = usbd_device_set_code_triple(bap_usbd, USBD_SPEED_HS, USB_BCC_MISCELLANEOUS,
						  subclass, protocol);
		if (err != 0) {
			LOG_ERR("Failed to set High-Speed code triple: %d", err);

			return err;
		}
	}

	LOG_DBG("Setting up Full-Speed USB");

	err = usbd_add_configuration(bap_usbd, USBD_SPEED_FS, &bap_usb_fs_config);
	if (err != 0) {
		LOG_ERR("Failed to add Full-Speed configuration: %d", err);

		return err;
	}

	err = usbd_register_all_classes(bap_usbd, USBD_SPEED_FS, class_cfg, NULL);
	if (err != 0) {
		LOG_ERR("Failed to register Full-Speed classes: %d", err);

		return err;
	}

	err = usbd_device_set_code_triple(bap_usbd, USBD_SPEED_FS, USB_BCC_MISCELLANEOUS, subclass,
					  protocol);
	if (err != 0) {
		LOG_ERR("Failed to set Full-Speed code triple: %d", err);

		return err;
	}

	usbd_self_powered(bap_usbd, attributes & USB_SCD_SELF_POWERED);

	return 0;
}

int bap_usb_init(void)
{
	USBD_DEVICE_DEFINE(bap_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
			   CONFIG_BT_BAP_SHELL_USB_VID, CONFIG_BT_BAP_SHELL_USB_PID);
	const struct device *uac2_headset = DEVICE_DT_GET(DT_NODELABEL(uac2_headset));
	static struct uac2_ops usb_audio_ops = {
		.terminal_update_cb = usb_terminal_update_cb,
		.sof_cb = usb_sof_cb,
#if defined(CONFIG_BT_AUDIO_TX)
		.get_recv_buf = usb_get_recv_buf_cb,
		.data_recv_cb = usb_data_recv_cb,
#endif /* CONFIG_BT_AUDIO_TX */
#if defined(CONFIG_BT_AUDIO_RX)
		.buf_release_cb = usb_buf_release_cb,
#endif /* CONFIG_BT_AUDIO_RX */
	};
	int err;

	if (!device_is_ready(uac2_headset)) {
		LOG_ERR("Cannot get USB Headset Device");
		return -EIO;
	}

	usbd_uac2_set_ops(uac2_headset, &usb_audio_ops, NULL);

	err = bap_usbd_setup_device(&bap_usbd);
	if (err != 0) {
		LOG_ERR("Failed to setup USB device: %d", err);
		return err;
	}

	err = usbd_init(&bap_usbd);
	if (err != 0) {
		LOG_ERR("Failed to initialize device support: %d", err);
		return err;
	}

	err = usbd_enable(&bap_usbd);
	if (err != 0) {
		LOG_ERR("Failed to enable USBD: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_SOC_NRF5340_CPUAPP)) {
		/* Use this to turn on 128 MHz clock for the nRF5340 cpu_app
		 * This may not be required, but reduces the risk of not decoding fast enough
		 * to keep up with USB
		 */
#if defined(CONFIG_CLOCK_CONTROL_NRF)
		err = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);

		if (err != 0) {
			LOG_WRN("Failed to set 128 MHz: %d", err);
		}
#else
		nrfx_clock_hfclk_divider_set(NRF_CLOCK_HFCLK_DIV_1);
#endif
	}

	LOG_INF("USB audio enabled");

	return 0;
}
