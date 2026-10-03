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
#include "bap_usb.h"

LOG_MODULE_REGISTER(bap_usb, CONFIG_BT_BAP_STREAM_LOG_LEVEL);

#define USB_LOG_RATE              (30U * MSEC_PER_SEC) /* 30 seconds */
#define USB_FRAME_DURATION_US     1000U
#define USB_SAMPLE_CNT            ((USB_FRAME_DURATION_US * BAP_USB_SAMPLE_RATE) / USEC_PER_SEC)
#define USB_BYTES_PER_SAMPLE      sizeof(int16_t)
#define USB_MONO_FRAME_SIZE       (USB_SAMPLE_CNT * USB_BYTES_PER_SAMPLE)
#define USB_STEREO_FRAME_SIZE     (USB_MONO_FRAME_SIZE * BAP_USB_CHANNELS)
#define USB_UAC2_SLOT_CNT         4U
/* Number of microframes in a frame. At high speed a transfer covers a microframe rather than a
 * frame, so an isochronous endpoint transfers an eighth of the samples per transfer.
 */
#define USB_MICROFRAMES_PER_FRAME 8U
#define USB_HS_SAMPLE_CNT         (USB_SAMPLE_CNT / USB_MICROFRAMES_PER_FRAME)

/* Both ring buffers hold interleaved stereo data, and all cursors into them are in USB frames
 * (left+right sample pairs) rather than in samples or octets.
 *
 * liblc3 is set up with an output sample rate of BAP_USB_SAMPLE_RATE for every stream (see
 * init_lc3_encoder() and init_lc3_decoder()), so a stream always produces and consumes PCM at
 * BAP_USB_SAMPLE_RATE regardless of its configured LC3 sample rate. Streams thus only differ in how
 * many USB frames a single LC3 frame covers: 120, 240, 360 or 480 for 2.5ms, 5ms, 7.5ms and 10ms
 * frame durations respectively. USB transfers USB_SAMPLE_CNT (48) frames per SOF at full speed, or
 * USB_HS_SAMPLE_CNT (6) frames per microframe at high speed.
 *
 * By sizing the ring buffers to a multiple of the least common multiple of all of those
 * (LCM(6, 48, 120, 240, 360, 480) == 1440), and by keeping every cursor a multiple of its own
 * step size, neither an LC3 frame nor a USB transfer can ever straddle the end of a ring buffer.
 * That removes the need for any wrap handling, lets liblc3 operate directly on the ring buffers,
 * and gives USB contiguous copy regions into and out of DMA staging slots.
 *
 * Note that the 44.1kHz LC3 configurations have non-integer frame durations (8.16ms and 10.88ms)
 * which would break this. They cannot reach this code as stream_started_cb() rejects any
 * frequency that is not 8, 16, 24, 32 or 48kHz, but supporting them would require revisiting the
 * ring buffer sizing.
 */
#define USB_RING_ALIGN_FRAMES 1440U                        /* LCM(48, 120, 240, 360, 480) == 30ms */
#define USB_RING_FRAMES       (USB_RING_ALIGN_FRAMES * 2U) /* 60ms */
#define USB_RING_SAMPLES      (USB_RING_FRAMES * BAP_USB_CHANNELS)

/* Maximum number of USB frames covered by a single LC3 frame */
#define USB_MAX_FRAMES_PER_LC3_FRAME                                                               \
	((LC3_MAX_FRAME_DURATION_US * BAP_USB_SAMPLE_RATE) / USEC_PER_SEC)

BUILD_ASSERT((USB_RING_FRAMES % USB_RING_ALIGN_FRAMES) == 0U,
	     "The ring buffers must be a multiple of USB_RING_ALIGN_FRAMES");
BUILD_ASSERT((USB_RING_ALIGN_FRAMES % USB_SAMPLE_CNT) == 0U,
	     "A USB transfer must never straddle the end of a ring buffer");
BUILD_ASSERT((USB_RING_ALIGN_FRAMES % USB_HS_SAMPLE_CNT) == 0U,
	     "A high speed USB transfer must never straddle the end of a ring buffer");
BUILD_ASSERT((USB_RING_ALIGN_FRAMES % USB_MAX_FRAMES_PER_LC3_FRAME) == 0U,
	     "An LC3 frame must never straddle the end of a ring buffer");
BUILD_ASSERT((USB_STEREO_FRAME_SIZE % USB_BUF_GRANULARITY) == 0U,
	     "USB transfers out of the ring buffers must be a multiple of the DMA granularity");

#define IN_TERMINAL_ID  UAC2_ENTITY_ID(DT_NODELABEL(in_terminal))
#define OUT_TERMINAL_ID UAC2_ENTITY_ID(DT_NODELABEL(out_terminal))

#if defined(CONFIG_BT_AUDIO_RX)
static void usb_data_request(const struct device *dev);
static void usb_in_terminal_update(bool microframes);
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
	ARG_UNUSED(user_data);
#if !defined(CONFIG_BT_AUDIO_RX)
	ARG_UNUSED(microframes);
#endif /* !CONFIG_BT_AUDIO_RX */

	if (terminal == IN_TERMINAL_ID) {
#if defined(CONFIG_BT_AUDIO_RX)
		usb_in_terminal_update(microframes);
#endif /* CONFIG_BT_AUDIO_RX */
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
 * handler). The buffer is written directly by liblc3, then USB SOF copies each transfer to
 * usb_in_dma_slots for USB DMA.
 */
USB_STATIC_BUF_DEFINE(usb_in_ring_buf_mem, USB_RING_SAMPLES * USB_BYTES_PER_SAMPLE);
static int16_t *const usb_in_ring_buf = (int16_t *)usb_in_ring_buf_mem;
USB_STATIC_BUF_DEFINE(usb_in_dma_slot0, USB_STEREO_FRAME_SIZE);
USB_STATIC_BUF_DEFINE(usb_in_dma_slot1, USB_STEREO_FRAME_SIZE);
USB_STATIC_BUF_DEFINE(usb_in_dma_slot2, USB_STEREO_FRAME_SIZE);
USB_STATIC_BUF_DEFINE(usb_in_dma_slot3, USB_STEREO_FRAME_SIZE);
static uint8_t *const usb_in_dma_slots[USB_UAC2_SLOT_CNT] = {
	usb_in_dma_slot0,
	usb_in_dma_slot1,
	usb_in_dma_slot2,
	usb_in_dma_slot3,
};
static bool usb_in_dma_slot_busy[USB_UAC2_SLOT_CNT];
static size_t usb_in_left_write_cursor;
static size_t usb_in_right_write_cursor;
static size_t usb_in_read_cursor;
static bool usb_in_left_active;
static bool usb_in_right_active;
/* Number of USB frames in a single transfer to the host. This is the wMaxPacketSize of the IN
 * endpoint, and is thus USB_SAMPLE_CNT when operating at full speed, but only an eighth of that
 * at high speed where a transfer covers a microframe rather than a frame.
 */
static size_t usb_in_slot_frames = USB_SAMPLE_CNT;
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
#define USB_IN_MAX_UNDERRUNS 100U /* 100 USB frames at full speed */

/* Store the transfer size that the connection speed implies, as the SOF callback is called once
 * per microframe at high speed and thus consumes an eighth of the frames per call.
 */
static void usb_in_terminal_update(bool microframes)
{
	const size_t slot_frames =
		(USBD_SUPPORTS_HIGH_SPEED && microframes) ? USB_HS_SAMPLE_CNT : USB_SAMPLE_CNT;

	if (slot_frames != usb_in_slot_frames) {
		usb_in_slot_frames = slot_frames;
		/* Keep the read cursor aligned to the new transfer size, so that a transfer never
		 * straddles the end of the ring buffer
		 */
		usb_in_read_cursor = usb_align_down(usb_in_read_cursor, slot_frames);
	}
}

/** Number of frames that @p write_cursor is ahead of the USB read cursor */
static size_t usb_in_chan_fill(size_t write_cursor)
{
	return usb_frames_between(usb_in_read_cursor, write_cursor);
}

static int16_t *usb_in_dma_slot_acquire(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(usb_in_dma_slot_busy); i++) {
		if (!usb_in_dma_slot_busy[i]) {
			usb_in_dma_slot_busy[i] = true;
			return (int16_t *)usb_in_dma_slots[i];
		}
	}

	return NULL;
}

static bool usb_in_dma_slot_release(void *buf)
{
	for (size_t i = 0U; i < ARRAY_SIZE(usb_in_dma_slot_busy); i++) {
		if (buf == usb_in_dma_slots[i]) {
			usb_in_dma_slot_busy[i] = false;
			return true;
		}
	}

	return false;
}

/* USB consumer callback, called once per (micro)frame, consumes usb_in_slot_frames frames from
 * the ring buffer
 */
static void usb_data_request(const struct device *dev)
{
	size_t slot_frames;
	size_t slot_size;
	size_t max_underruns;
	int16_t *pcm_buf;
	bool left_active;
	bool right_active;
	bool starving = false;
	bool give_up;
	bool have_data;
	int err;

	slot_frames = usb_in_slot_frames;
	slot_size = slot_frames * BAP_USB_CHANNELS * USB_BYTES_PER_SAMPLE;
	max_underruns = USB_IN_MAX_UNDERRUNS * (USB_SAMPLE_CNT / slot_frames);
	left_active = usb_in_left_active;
	right_active = usb_in_right_active;

	/* A channel that has starved for too long is ignored entirely, so that a channel which
	 * does produce data is not held back indefinitely. This bounds the effect of a producer
	 * that stops without deactivating its channel.
	 */
	give_up = usb_in_underrun_cnt >= max_underruns;

	if (left_active && usb_in_chan_fill(usb_in_left_write_cursor) < slot_frames) {
		left_active = !give_up;
		starving = true;
	}

	if (right_active && usb_in_chan_fill(usb_in_right_write_cursor) < slot_frames) {
		right_active = !give_up;
		starving = true;
	}

	/* Only consume data that every channel that is still considered active has produced */
	have_data = (left_active || right_active) && (!starving || give_up);

	if (have_data) {
		const int16_t *src;
		static size_t cnt;
		int16_t *dma_buf;

		src = &usb_in_ring_buf[usb_in_read_cursor * BAP_USB_CHANNELS];

		/* Acquire the slot before advancing, so that a failed acquire does not
		 * silently drop a frame block (the current code advances first and then
		 * returns, skipping data that was never sent).
		 */
		dma_buf = usb_in_dma_slot_acquire();
		if (dma_buf == NULL) {
			LOG_WRN_RATELIMIT("No available USB IN DMA slot");
			return;
		}

		if (left_active != right_active) {
			/* Duplicate the single active channel to both while copying */
			const size_t off = left_active ? 0U : 1U;

			for (size_t i = 0U; i < slot_frames; i++) {
				const int16_t sample = src[(i * BAP_USB_CHANNELS) + off];

				dma_buf[(i * BAP_USB_CHANNELS)] = sample;
				dma_buf[(i * BAP_USB_CHANNELS) + 1U] = sample;
			}
		} else {
			(void)memcpy(dma_buf, src, slot_size);
		}

		usb_in_read_cursor = usb_advance(usb_in_read_cursor, slot_frames);

		if (!starving) {
			usb_in_underrun_cnt = 0U;
		}

		pcm_buf = dma_buf;

		cnt++;
		LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "[%zu]: Sending USB audio", cnt);
	} else {
		static size_t cnt;
		int16_t *dma_buf;

		/* Underrun. Send silence and leave the read cursor alone, so that the data that is
		 * still being decoded is not skipped.
		 */
		dma_buf = usb_in_dma_slot_acquire();
		if (dma_buf == NULL) {
			LOG_WRN_RATELIMIT("No available USB IN DMA slot for silence");
			return;
		}

		pcm_buf = dma_buf;
		(void)memset(pcm_buf, 0, slot_size);

		usb_in_underrun_cnt++;
		cnt++;
		LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "[%zu]: Sending USB silence", cnt);
	}

	err = usbd_uac2_send(dev, IN_TERMINAL_ID, pcm_buf, slot_size);
	if (err != 0) {
		const int send_err = err;
		static size_t cnt;

		(void)usb_in_dma_slot_release(pcm_buf);

		cnt++;
		LOG_ERR_RATELIMIT_RATE(USB_LOG_RATE, "Failed to send USB audio: %d (%zu)", send_err,
				       cnt);
	}
}

static void usb_buf_release_cb(const struct device *dev, uint8_t terminal, void *buf,
			       void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	(void)usb_in_dma_slot_release(buf);
}

static size_t usb_in_chan_offset(enum bt_audio_location chan_alloc)
{
	return chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT ? 1U : 0U;
}

static size_t *usb_in_get_write_cursor(enum bt_audio_location chan_alloc)
{
	if (chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT) {
		return &usb_in_right_write_cursor;
	}

	/* Mono is stored in, and sent from, the left channel */
	return &usb_in_left_write_cursor;
}

void bap_usb_activate_in_chan(enum bt_audio_location chan_alloc)
{
	const bool is_right = chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT;

	/* There is a */
	if (is_right) {
		usb_in_right_active = true;
		usb_in_right_write_cursor = usb_in_read_cursor;
	} else {
		usb_in_left_active = true;
		usb_in_left_write_cursor = usb_in_read_cursor;
	}

	LOG_INF("Activated USB IN channel 0x%08X", (uint32_t)chan_alloc);
}

void bap_usb_deactivate_in_chan(enum bt_audio_location chan_alloc)
{
	const bool is_right = chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT;

	if (is_right) {
		usb_in_right_active = false;
	} else {
		usb_in_left_active = false;
	}

	usb_in_underrun_cnt = 0U;

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
	/* Snapshot the consumer state once; both are owned by the usbd thread */
	const size_t read_cursor = usb_in_read_cursor;
	const size_t slot_frames = usb_in_slot_frames;
	/* The consumer may be copying the frames at read_cursor right now, so the
	 * first frame this producer may touch is one transfer ahead of it.
	 */
	const size_t safe = usb_advance(read_cursor, slot_frames);
	size_t cursor = usb_align_down(safe, sample_cnt);
	size_t silence_cnt;

	if (cursor != safe) {
		cursor = usb_advance(cursor, sample_cnt);
	}

	cursor = usb_advance(cursor, usb_align_down(USB_IN_TARGET_PREFILL_FRAMES, sample_cnt));

	silence_cnt = usb_frames_between(safe, cursor);
	for (size_t i = 0U; i < silence_cnt; i++) {
		const size_t frame = usb_advance(safe, i);

		usb_in_ring_buf[(frame * BAP_USB_CHANNELS) + chan_offset] = 0;
	}

	return cursor;
}

int16_t *bap_usb_claim_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt)
{
	size_t *cursor = usb_in_get_write_cursor(chan_alloc);
	int16_t *frame;
	size_t fill;

	if (sample_cnt == 0U || sample_cnt > USB_MAX_FRAMES_PER_LC3_FRAME ||
	    (USB_RING_FRAMES % sample_cnt) != 0U) {
		LOG_WRN_RATELIMIT("Invalid sample count %zu", sample_cnt);

		return NULL;
	}

	fill = usb_in_chan_fill(*cursor);

	/* Resynchronize the channel if it has fallen behind the consumer (which has already sent
	 * silence for the data being decoded now), if it has run so far ahead that it is about to
	 * overwrite data that has not been sent yet, or if it is not aligned to its own frame size
	 * (which happens on the first frame after the channel was activated).
	 *
	 * The upper bound is inclusive, as a write cursor that ends up exactly at the read cursor
	 * is indistinguishable from an empty ring buffer.
	 */
	if (fill < sample_cnt || fill >= (USB_RING_FRAMES - USB_MAX_FRAMES_PER_LC3_FRAME) ||
	    (*cursor % sample_cnt) != 0U) {
		*cursor = usb_in_resync(chan_alloc, sample_cnt);

		LOG_WRN_RATELIMIT_RATE(USB_LOG_RATE,
				       "Resynchronized USB IN channel 0x%08X (fill was %zu)",
				       (uint32_t)chan_alloc, fill);
	}

	frame = &usb_in_ring_buf[(*cursor * BAP_USB_CHANNELS) + usb_in_chan_offset(chan_alloc)];

	return frame;
}

void bap_usb_release_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt)
{
	size_t *cursor = usb_in_get_write_cursor(chan_alloc);
	static size_t cnt;

	*cursor = usb_advance(*cursor, sample_cnt);

	cnt++;
	LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "[%zu]: Added USB audio frame", cnt);
}
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
/* Interleaved stereo ring buffer holding audio received from the USB host.
 *
 * It has a single producer, where the USB OUT endpoint writes by DMA into
 * usb_out_dma_slots and usb_data_recv_cb() copies those samples into this ring,
 * and 0 or more consumers, one per TX stream, that each have their own read
 * position and that may consume at different rates. The buffer is read directly
 * by liblc3.
 *
 * All positions on this path are monotonic counts of USB frames rather than ring buffer indices.
 * That makes the distance between the producer and a consumer unambiguous no matter how long a
 * consumer has been idle, which in turn lets a stream be resynchronized lazily when it asks for
 * data instead of eagerly on every received USB transfer.
 */
USB_STATIC_BUF_DEFINE(usb_out_ring_buf_mem, USB_RING_SAMPLES * USB_BYTES_PER_SAMPLE);
static int16_t *const usb_out_ring_buf = (int16_t *)usb_out_ring_buf_mem;
USB_STATIC_BUF_DEFINE(usb_out_dma_slot0, USB_STEREO_FRAME_SIZE);
USB_STATIC_BUF_DEFINE(usb_out_dma_slot1, USB_STEREO_FRAME_SIZE);
USB_STATIC_BUF_DEFINE(usb_out_dma_slot2, USB_STEREO_FRAME_SIZE);
USB_STATIC_BUF_DEFINE(usb_out_dma_slot3, USB_STEREO_FRAME_SIZE);
static uint8_t *const usb_out_dma_slots[USB_UAC2_SLOT_CNT] = {
	usb_out_dma_slot0,
	usb_out_dma_slot1,
	usb_out_dma_slot2,
	usb_out_dma_slot3,
};
struct usb_out_dma_slot {
	uint64_t pos;
	size_t frame_cnt;
	bool pending;
};
static struct usb_out_dma_slot usb_out_dma_slot_state[USB_UAC2_SLOT_CNT];
/* Points to the oldest/uninitialized data */
static uint64_t usb_out_write_pos;
/* Position that has been handed to the USB stack but not yet received. The UAC2 class may have
 * up to 2 transfers queued at a time, so this may be ahead of usb_out_write_pos.
 */
static uint64_t usb_out_pending_pos;
/* Number of frames in a single USB transfer. This is the wMaxPacketSize of the OUT endpoint, and
 * is thus USB_SAMPLE_CNT when operating at full speed, but only an eighth of that at high speed
 * where a transfer covers a microframe rather than a frame.
 */
static size_t usb_out_slot_frames = USB_SAMPLE_CNT;

/* Amount of data a stream aims to keep between itself and the write position. Used when a stream
 * starts, and when it has to be resynchronized because it was about to be overwritten.
 */
#define USB_OUT_TARGET_PREFILL_FRAMES (USB_MAX_FRAMES_PER_LC3_FRAME * 2U) /* 20ms */

/* Largest amount of data a stream may wait for before it starts sending. Kept below the point
 * where usb_out_stream_sync() resynchronizes the stream, so that the wait can always be satisfied.
 */
#define USB_OUT_MAX_PREFILL_FRAMES (USB_RING_FRAMES - (USB_MAX_FRAMES_PER_LC3_FRAME * 2U))

uint8_t bap_usb_get_chan_offset(enum bt_audio_location chan_alloc)
{
	__ASSERT((chan_alloc == BT_AUDIO_LOCATION_MONO_AUDIO ||
		  chan_alloc == BT_AUDIO_LOCATION_FRONT_LEFT ||
		  chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT),
		 "Invalid chan_alloc %d", chan_alloc);

	return chan_alloc == BT_AUDIO_LOCATION_FRONT_RIGHT ? 1U : 0U;
}

/** Ring buffer index of an absolute frame position */
static size_t usb_ring_index(uint64_t pos)
{
	return (size_t)(pos % USB_RING_FRAMES);
}

/** Round an absolute position down to a multiple of @p step */
static uint64_t usb_pos_align_down(uint64_t pos, size_t step)
{
	__ASSERT(step != 0U, "Invalid step");
	__ASSERT((USB_RING_FRAMES % step) == 0U, "Step %zu does not divide the ring buffer", step);

	return pos - (pos % step);
}

static void usb_out_terminal_disabled(void)
{
	/* Any queued transfers are discarded by the USB stack, so the slots that were handed out
	 * become available again
	 */
	usb_out_pending_pos = usb_out_write_pos;
	for (size_t i = 0U; i < ARRAY_SIZE(usb_out_dma_slot_state); i++) {
		usb_out_dma_slot_state[i].pending = false;
	}
}

/**
 * Number of frames that @p sh_stream can still read before catching up with the producer,
 * resynchronizing the stream first if the producer has overwritten data that it had not consumed
 * yet.
 *
 * This is evaluated lazily when a stream asks for data rather than eagerly on every received USB
 * transfer, so a stream that is not reading costs nothing until it actually wants a frame. Each
 * stream is evaluated against its own margin, as they may consume at different rates, and a
 * stream that is not reading at all must not hold back the producer.
 */
static size_t usb_out_stream_sync(struct shell_stream *sh_stream, size_t read_cnt)
{
	const uint64_t write_pos = usb_out_write_pos; /* take snapshot */
	uint64_t avail;

	if (sh_stream->tx.usb_read_pos >= write_pos) {
		/* The stream was started at the next frame-aligned position at or after the
		 * producer, so it may legitimately be ahead of it until the producer catches up.
		 */
		return 0U;
	}

	avail = write_pos - sh_stream->tx.usb_read_pos;

	if (avail > (USB_RING_FRAMES - USB_MAX_FRAMES_PER_LC3_FRAME)) {
		const uint64_t aligned = usb_pos_align_down(write_pos, read_cnt);
		const uint64_t target = usb_align_down(USB_OUT_TARGET_PREFILL_FRAMES, read_cnt);

		/* Drop the backlog and keep the most recent data, so that the stream does not
		 * immediately fall behind again
		 */
		sh_stream->tx.usb_read_pos = (aligned > target) ? (aligned - target) : 0U;
		avail = write_pos - sh_stream->tx.usb_read_pos;

		LOG_WRN_RATELIMIT_RATE(USB_LOG_RATE,
				       "Resynchronized USB OUT stream %p (avail was %zu)",
				       (void *)sh_stream, (size_t)avail);
	}

	return (size_t)avail;
}

static int usb_out_dma_slot_index_by_buf(void *buf)
{
	for (size_t i = 0U; i < ARRAY_SIZE(usb_out_dma_slot_state); i++) {
		if (buf == usb_out_dma_slots[i]) {
			return (int)i;
		}
	}

	return -1;
}

static int usb_out_dma_slot_index_free(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(usb_out_dma_slot_state); i++) {
		if (!usb_out_dma_slot_state[i].pending) {
			return (int)i;
		}
	}

	return -1;
}

void bap_usb_tx_stream_started(struct shell_stream *sh_stream)
{
	const size_t read_cnt = bap_usb_get_read_cnt(sh_stream);

	if (read_cnt == 0U) {
		LOG_WRN("Invalid frame duration %u for stream %p", sh_stream->lc3_frame_duration_us,
			(void *)sh_stream);
		return;
	}

	__ASSERT((USB_RING_FRAMES % read_cnt) == 0U,
		 "Read count %zu does not divide the ring buffer", read_cnt);

	/* Start at the next frame-aligned position at or after the producer, so that only data
	 * received after the stream started is sent, rather than at whatever the union with the
	 * RX state left in the field.
	 *
	 * This rounds up rather than down: rounding down would place the read position behind the
	 * producer whenever usb_out_write_pos is not a multiple of read_cnt, making the first LC3
	 * frame contain up to read_cnt - 1 frames of data from before the stream started.
	 * Positions are monotonic, so the stream may temporarily be ahead of the producer, which
	 * usb_out_stream_sync() handles by reporting that no data is available yet.
	 */
	sh_stream->tx.usb_read_pos = ROUND_UP(usb_out_write_pos, read_cnt);
}

static void *usb_get_recv_buf_cb(const struct device *dev, uint8_t terminal, uint16_t size,
				 void *user_data)
{
	size_t frame_cnt;
	void *buf;
	int slot_idx;

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
	frame_cnt = size / (BAP_USB_CHANNELS * USB_BYTES_PER_SAMPLE);
	if (frame_cnt == 0U || frame_cnt > USB_SAMPLE_CNT ||
	    (size % (BAP_USB_CHANNELS * USB_BYTES_PER_SAMPLE)) != 0U ||
	    (USB_RING_FRAMES % frame_cnt) != 0U) {
		LOG_WRN_RATELIMIT("Unsupported receive buffer size %u", size);

		return NULL;
	}

	if (frame_cnt != usb_out_slot_frames) {
		/* Keep the positions aligned to the new transfer size. This rounds up rather than
		 * down, as moving the producer backwards past a consumer would make the distance
		 * computed by usb_out_stream_sync() underflow.
		 */
		usb_out_slot_frames = frame_cnt;
		usb_out_pending_pos =
			usb_pos_align_down(usb_out_write_pos + frame_cnt - 1U, frame_cnt);
		usb_out_write_pos = usb_out_pending_pos;
	}

	slot_idx = usb_out_dma_slot_index_free();

	if (slot_idx < 0) {
		LOG_WRN_RATELIMIT("No available USB OUT DMA slot");
		return NULL;
	}

	/* Hand out the next unused DMA-aligned slot. The slot is committed by
	 * usb_data_recv_cb().
	 */
	usb_out_dma_slot_state[slot_idx].pos = usb_out_pending_pos;
	usb_out_dma_slot_state[slot_idx].frame_cnt = frame_cnt;
	usb_out_dma_slot_state[slot_idx].pending = true;
	buf = usb_out_dma_slots[slot_idx];
	usb_out_pending_pos += frame_cnt;

	return buf;
}

static void usb_data_recv_cb(const struct device *dev, uint8_t terminal, void *buf, uint16_t size,
			     void *user_data)
{
	struct usb_out_dma_slot *slot;
	const int16_t *src;
	static size_t cnt;
	size_t frame_cnt;
	int16_t *dst;
	int slot_idx;

	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	if (buf == NULL) {
		return;
	}

	/* The data has been written into the DMA-aligned slot already, so all that is left is to
	 * make it available to the consumers. The host may send a short packet, in which case the
	 * remainder of the slot is zero-filled; the entire slot is always committed so that the
	 * positions keep the alignment that the ring buffer sizing relies on.
	 */
	slot_idx = usb_out_dma_slot_index_by_buf(buf);
	if (slot_idx < 0) {
		LOG_WRN_RATELIMIT("Unknown USB OUT DMA slot %p", buf);
		return;
	}

	slot = &usb_out_dma_slot_state[slot_idx];
	if (!slot->pending) {
		LOG_WRN_RATELIMIT("USB OUT DMA slot %d not pending", slot_idx);
		return;
	}

	frame_cnt = MIN(size / (BAP_USB_CHANNELS * USB_BYTES_PER_SAMPLE), slot->frame_cnt);
	if (frame_cnt < slot->frame_cnt) {
		int16_t *pcm = (int16_t *)buf;

		(void)memset(&pcm[frame_cnt * BAP_USB_CHANNELS], 0,
			     (slot->frame_cnt - frame_cnt) * BAP_USB_CHANNELS *
				     USB_BYTES_PER_SAMPLE);

		LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "Received short USB packet of %u octets",
				       size);
	}

	__ASSERT(slot->pos == usb_out_write_pos, "Unexpected USB OUT position");

	src = (const int16_t *)buf;
	dst = &usb_out_ring_buf[usb_ring_index(slot->pos) * BAP_USB_CHANNELS];
	(void)memcpy(dst, src, slot->frame_cnt * BAP_USB_CHANNELS * USB_BYTES_PER_SAMPLE);
	usb_out_write_pos += slot->frame_cnt;
	slot->pending = false;

	cnt++;
	LOG_DBG_RATELIMIT_RATE(USB_LOG_RATE, "USB Data received (count = %zu)", cnt);
}

const int16_t *bap_usb_get_frame_block(struct shell_stream *sh_stream)
{
	const size_t read_cnt = bap_usb_get_read_cnt(sh_stream);
	const int16_t *block;

	if (read_cnt == 0U) {
		return NULL;
	}

	/* Resynchronize here rather than on every received USB transfer, so that the read position
	 * is only touched when a frame block is actually wanted.
	 */
	if (usb_out_stream_sync(sh_stream, read_cnt) < read_cnt) {
		LOG_WRN_RATELIMIT_RATE(USB_LOG_RATE, "No frame block available for stream %p",
				       (void *)sh_stream);

		return NULL;
	}

	__ASSERT((sh_stream->tx.usb_read_pos % read_cnt) == 0U,
		 "Misaligned position %llu for read count %zu",
		 (unsigned long long)sh_stream->tx.usb_read_pos, read_cnt);

	/* The ring buffer size is a multiple of read_cnt, so the frame block never straddles the
	 * end of the ring buffer and can be handed to liblc3 as-is.
	 */
	block = &usb_out_ring_buf[usb_ring_index(sh_stream->tx.usb_read_pos) * BAP_USB_CHANNELS];
	sh_stream->tx.usb_read_pos += read_cnt;

	return block;
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
