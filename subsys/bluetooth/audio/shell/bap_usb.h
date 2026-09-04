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


#define USB_SAMPLE_RATE            48000U

/* The PCM data exchanged with USB is always interleaved stereo at USB_SAMPLE_RATE. A "USB frame"
 * is a single left+right sample pair, so that all streams advance their cursors by the same
 * amount for the same duration of audio, independently of their channel allocation.
 */
#define USB_CHANNELS 2U

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
 * Returns true if the USB OUT ring buffer holds an entire SDU worth of data for @p sh_stream.
 *
 * Shall be called before bap_usb_claim_frame_block() for every SDU.
 */
bool bap_usb_can_get_full_sdu(struct shell_stream *sh_stream);

/**
 * Provide a pointer to @p sh_stream's current position in the interleaved USB OUT ring buffer.
 *
 * The returned pointer is valid for bap_usb_get_read_cnt() interleaved stereo frames, and shall
 * be released with bap_usb_release_frame_block() once the frame block has been encoded.
 */
const int16_t *bap_usb_claim_frame_block(struct shell_stream *sh_stream);

/** Advance @p sh_stream past the frame block claimed with bap_usb_claim_frame_block() */
void bap_usb_release_frame_block(struct shell_stream *sh_stream);

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
 * @p sample_cnt samples shall be written with a stride of USB_CHANNELS, after which the channel
 * shall be advanced with bap_usb_release_in_frame(). Returns NULL if @p chan_alloc cannot
 * currently be written.
 */
int16_t *bap_usb_claim_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt);

/** Advance @p chan_alloc past the frame claimed with bap_usb_claim_in_frame() */
void bap_usb_release_in_frame(enum bt_audio_location chan_alloc, size_t sample_cnt);
