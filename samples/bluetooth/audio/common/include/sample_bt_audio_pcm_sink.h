/*
 * SPDX-FileCopyrightText: Copyright 2026 Ezurio LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_PCM_SINK_H_
#define ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_PCM_SINK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Transport-agnostic PCM output sink. The playback pump (audio_playback.c)
 * decodes LC3 to interleaved PCM and hands each block to this sink; the backing
 * implementation (pcm_sink_i2s.c today, a USB backend tomorrow) owns the final
 * transport and, where applicable, the external codec.
 */

/* Worst-case decoded block: 48 kHz, 10 ms, stereo, 16-bit = 480 * 2 * 2 = 1920 B. */
#define SAMPLE_BT_AUDIO_PCM_SINK_MAX_SAMPLES (48U * 10U) /* 48 kHz * 10 ms, per channel */
#define SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN    2U
#define SAMPLE_BT_AUDIO_PCM_SINK_MAX_BLOCK_BYTES                                                   \
	(SAMPLE_BT_AUDIO_PCM_SINK_MAX_SAMPLES * SAMPLE_BT_AUDIO_PCM_SINK_MAX_CHAN * sizeof(int16_t))

/** @brief PCM stream description handed to the sink. */
struct sample_bt_audio_pcm_sink_cfg {
	uint32_t sample_rate_hz;    /**< frame clock in Hz */
	uint32_t frame_duration_us; /**< LC3 frame duration, used for drain timing */
	uint8_t channels;           /**< interleaved channels per block */
	uint8_t bits;               /**< sample width in bits */
	size_t block_bytes;         /**< bytes per sample_bt_audio_pcm_sink_write() block */
};

/**
 * @brief Verify the sink hardware is present and ready.
 *
 * @return 0 on success, negative errno otherwise.
 */
int sample_bt_audio_pcm_sink_init(void);

/**
 * @brief Program the transport (and codec) for @p cfg.
 *
 * Reprograms the hardware only when the stream parameters changed since the
 * last successful call, so it is cheap to invoke on every stream start.
 *
 * @return 0 on success, negative errno otherwise.
 */
int sample_bt_audio_pcm_sink_configure(const struct sample_bt_audio_pcm_sink_cfg *cfg);

/**
 * @brief Prime and start the transport so it is ready to consume blocks.
 *
 * @return 0 on success, negative errno otherwise.
 */
int sample_bt_audio_pcm_sink_start(void);

/** @brief Gracefully drain and park the transport. */
void sample_bt_audio_pcm_sink_stop(void);

/**
 * @brief Write one PCM block, blocking until the transport frees a buffer.
 *
 * @return 0 on success, negative errno if the transport has faulted.
 */
int sample_bt_audio_pcm_sink_write(void *block, size_t bytes);

/** @brief True once the sink has been configured at least once. */
bool sample_bt_audio_pcm_sink_is_configured(void);

/** @brief Apply a VCS-scale (0..255) output volume to the codec. */
void sample_bt_audio_pcm_sink_set_volume(uint8_t vcs_volume);

/** @brief Mute or unmute the codec output. */
void sample_bt_audio_pcm_sink_set_mute(bool mute);

#endif /* ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_PCM_SINK_H_ */
