/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/assigned_numbers.h>

#include "audio.h"

#define BAP_USB_SAMPLE_RATE 48000U

/* The PCM data exchanged with USB is always interleaved stereo at BAP_USB_SAMPLE_RATE. A "USB
 * frame" is a single left+right sample pair, so that all streams advance their cursors by the same
 * amount for the same duration of audio, independently of their channel allocation.
 */
#define BAP_USB_CHANNELS 2U

int bap_usb_init(void);

/* Declared unconditionally as bap.c guards the calls with IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) */
/** Number of USB frames (left+right sample pairs) covered by a single LC3 frame of @p sh_stream */
size_t bap_usb_get_read_cnt(const struct shell_stream *sh_stream);

/**
 * Place @p sh_stream in the USB OUT ring buffer so that it starts reading data that is old enough
 * to not be overwritten before it is consumed. Shall be called when the stream starts sending.
 */
void bap_usb_tx_stream_started(struct shell_stream *sh_stream);

/**
 * Provide a pointer to @p sh_stream's current position in the interleaved USB OUT ring buffer.
 *
 * The returned pointer is valid for bap_usb_get_read_cnt() interleaved stereo frames
 */
const int16_t *bap_usb_get_frame_block(struct shell_stream *sh_stream);

/**
 * Mark @p chan_alloc as being provided by a stream, so that the USB IN data is taken from the
 * ring buffer rather than being generated from the other channel.
 */
void bap_usb_activate_in_chan(enum bt_audio_location chan_alloc);

/** Mark @p chan_alloc as no longer being provided by any stream */
void bap_usb_deactivate_in_chan(enum bt_audio_location chan_alloc);

/**
 * Provide a pointer to the position of @p chan_alloc in the interleaved USB IN ring buffer that
 * the next decoded frame shall be written to.
 *
 * @p sample_cnt samples shall be written with a stride of BAP_USB_CHANNELS, after which the channel
 * shall be advanced with bap_usb_release_in_frame(). Returns NULL if @p chan_alloc cannot
 * currently be written.
 */
int16_t *bap_usb_claim_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt);

/** Advance @p chan_alloc past the frame claimed with bap_usb_claim_in_frame() */
void bap_usb_release_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt);

/** Offset of @p chan_alloc in an interleaved stereo USB frame */
/**
 * @brief Get the offset in an interleaved stereo USB frame from @p chan_alloc
 *
 * @param chan_alloc Either @ref BT_AUDIO_LOCATION_MONO_AUDIO, @ref BT_AUDIO_LOCATION_FRONT_LEFT or
 *                   @ref BT_AUDIO_LOCATION_FRONT_RIGHT
 *
 * @retval 0 For @ref BT_AUDIO_LOCATION_MONO_AUDIO or @ref BT_AUDIO_LOCATION_FRONT_LEFT
 * @retval 1 For @ref BT_AUDIO_LOCATION_FRONT_RIGHT
 */
uint8_t bap_usb_get_chan_offset(enum bt_audio_location chan_alloc);
