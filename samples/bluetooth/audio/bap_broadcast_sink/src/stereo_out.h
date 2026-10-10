/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink sample stereo output buffer header
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLE_BAP_BROADCAST_SINK_STEREO_OUT_H
#define SAMPLE_BAP_BROADCAST_SINK_STEREO_OUT_H

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>

#define STEREO_OUT_SAMPLE_RATE_HZ 48000U

/**
 * @brief Add decoded frame to the stereo output buffer
 *
 * @param chan_allocation @ref BT_AUDIO_LOCATION_FRONT_LEFT, @ref BT_AUDIO_LOCATION_FRONT_RIGHT
 *                        or @ref BT_AUDIO_LOCATION_MONO_AUDIO
 * @param frame_size Size of @p frame in octets
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid channel, frame or frame size
 * @retval -ENOEXEC Old timestamp, discarded
 * @retval -ENOMEM No memory to enqueue
 */
int stereo_out_add_frame(enum bt_audio_location chan_allocation, const int16_t *frame,
			 size_t frame_size, uint32_t ts);

/**
 * @brief Clear last sent SDU
 *
 * If only part of the SDU could be decoded, this should be called
 */
void stereo_out_clear_frames(void);

/**
 * @brief Read interleaved stereo samples
 *
 * @param dst Destination for @p size octets, or NULL to discard them
 * @param size Number of octets to read
 *
 * @return Number of octets read, which is less than @p size when not enough are queued
 */
uint32_t stereo_out_read(uint8_t *dst, uint32_t size);

#endif /* SAMPLE_BAP_BROADCAST_SINK_STEREO_OUT_H */
