/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink sample audio codec header
 *
 * This file declares the audio codec related functionality for the sample
 *
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLE_BAP_BROADCAST_SINK_HW_CODEC_H
#define SAMPLE_BAP_BROADCAST_SINK_HW_CODEC_H
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Open and initialize the audio codec.
 *
 * This function prepares the audio codec for use, allocating and
 * initializing any required resources.
 *
 * @retval 0        On success.
 * @return negative On failure, with a negative error code.
 */
int hw_codec_open(void);

/**
 * @brief Configure the audio codec output format.
 *
 * This function applies the desired sampling rate and channel count to the
 * audio codec. It must be called after @ref hw_codec_open and before sending
 * audio data with @ref hw_codec_write_data.
 *
 * @param samplerate   Sampling rate in Hz (for example, 48000 for 48 kHz).
 * @param frame_dur_us Codec frame duration in microseconds, as negotiated in
 *                     the codec configuration (7500 or 10000 for LC3). Used to
 *                     size the transfer blocks of the underlying transport.
 * @param channels     Number of interleaved PCM output channels (1 for mono,
 *                     2 for stereo). Must match the number of active BIS
 *                     streams so the I2S consumption rate equals the decoded
 *                     PCM production rate.
 *
 * @retval 0        On success.
 * @retval -EALREADY If the codec is already configured.
 * @return negative  On other failures, with a negative error code.
 */
int hw_codec_cfg(uint32_t samplerate, uint32_t frame_dur_us, uint8_t channels);

/**
 * @brief Write audio data to the codec for playback.
 *
 * This function sends raw audio samples to the codec. It should be
 * called after the codec has been opened and configured.
 *
 * @param data Pointer to the buffer containing audio data.
 * @param len  Number of bytes of audio data to write.
 *
 * @return The number of bytes successfully queued in the internal buffer.
 *         A value less than @p len indicates a partial enqueue. The actual
 *         transfer of the queued data to the codec hardware happens
 *         asynchronously.
 */
uint32_t hw_codec_write_data(const uint8_t *data, uint32_t len);

/**
 * @brief Close the audio codec and release resources.
 *
 * This function stops the codec if necessary and frees any resources
 * that were allocated by @ref hw_codec_open.
 *
 * @retval 0    On success.
 * @retval -EIO If the codec has not been opened or configured.
 */
int hw_codec_close(void);

#endif /* SAMPLE_BAP_BROADCAST_SINK_HW_CODEC_H */
