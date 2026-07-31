/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLE_TMAP_PERIPHERAL_LE_AUDIO_PLAYBACK_H_
#define SAMPLE_TMAP_PERIPHERAL_LE_AUDIO_PLAYBACK_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/net_buf.h>

struct le_audio_pcm_cfg {
	uint32_t freq_hz;             /* 48000, 32000, 16000 */
	uint32_t frame_duration_us;   /* 7500 or 10000 */
	uint16_t octets_per_frame;    /* per-channel LC3 frame size */
	uint8_t  chan_cnt;            /* 1 or 2 */
	uint8_t  frame_blocks_per_sdu;/* usually 1 */
};

int le_audio_playback_init(void);
int le_audio_playback_stream_start(const struct le_audio_pcm_cfg *cfg);
void le_audio_playback_stream_stop(void);
void le_audio_playback_stream_recv(struct net_buf *buf);

/* Apply a VCS-scale (0..255) output volume to the codec. Cached and re-applied
 * on the next codec (re)configure.
 */
void le_audio_playback_set_volume(uint8_t vcs_volume);
void le_audio_playback_set_mute(bool mute);

#endif /* SAMPLE_TMAP_PERIPHERAL_LE_AUDIO_PLAYBACK_H_ */
