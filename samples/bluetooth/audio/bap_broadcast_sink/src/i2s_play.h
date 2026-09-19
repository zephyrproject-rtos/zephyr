/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_I2S_PLAY_H_
#define ZEPHYR_INCLUDE_I2S_PLAY_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>

/* Forward declaration to avoid pulling stream_rx.h into every include site. */
struct stream_rx;

int i2s_play_init(void);

void i2s_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels);

/*
 * Start I2S playback for a BAP stream.
 *
 * Derives the PCM sample rate from the codec configuration, resets the PCM
 * ring buffer, applies the codec/I2S hardware configuration (via the
 * codec_play thread) and enables playback.
 *
 * Returns 0 on success or a negative errno if the sample rate cannot be
 * derived from the codec configuration.
 */
int i2s_play_start(const struct bt_audio_codec_cfg *codec_cfg);

void i2s_play_stop(void);

/*
 * Push one decoded PCM frame from the LC3 decoder into the I2S playback
 * ring buffer.
 *
 * Returns 0 on success (including intentional drops) or a negative errno
 * on error.
 */
int i2s_play_add_frame(const struct stream_rx *stream, int chn, const void *pcm);

void i2s_keep_play(void);

#endif /* ZEPHYR_INCLUDE_I2S_PLAY_H_ */
