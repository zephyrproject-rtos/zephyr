/*
 * SPDX-FileCopyrightText: Copyright 2026 Ezurio LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_LC3_DECODE_H_
#define ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_LC3_DECODE_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/net_buf.h>

/*
 * LC3 decode stage. Transport-agnostic: it turns received LC3 frame blocks into
 * interleaved stereo 16-bit PCM that the playback pump forwards to a pcm_sink.
 */

/**
 * @brief Set up one LC3 decoder per channel.
 *
 * @param freq_hz           LC3 sample rate.
 * @param frame_duration_us LC3 frame duration.
 * @param chan_cnt          Number of channels (1 or 2).
 *
 * @return 0 on success, negative errno otherwise.
 */
int sample_bt_audio_lc3_decode_setup(uint32_t freq_hz, uint32_t frame_duration_us,
				     uint8_t chan_cnt);

/** @brief True once at least one decoder has been set up. */
bool sample_bt_audio_lc3_decode_is_ready(void);

/**
 * @brief Decode one LC3 frame block into interleaved stereo PCM.
 *
 * Mono input is duplicated into both output channels. @p do_plc drives LC3
 * packet-loss concealment for invalid or wrong-size SDUs.
 *
 * @param buf               SDU holding the frame block; consumed on decode.
 * @param octets_per_frame  Per-channel LC3 frame size.
 * @param chan_cnt          Number of source channels (1 or 2).
 * @param samples_per_frame Per-channel PCM samples produced per frame.
 * @param do_plc            Decode as packet-loss concealment.
 * @param pcm               Output buffer for interleaved stereo PCM.
 *
 * @return true when @p pcm holds a block to write, false on a decoder error.
 */
bool sample_bt_audio_lc3_decode_block(struct net_buf *buf, uint16_t octets_per_frame,
				      uint8_t chan_cnt, uint32_t samples_per_frame, bool do_plc,
				      int16_t *pcm);

#endif /* ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_LC3_DECODE_H_ */
