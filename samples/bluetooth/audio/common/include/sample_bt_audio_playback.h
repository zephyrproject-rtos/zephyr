/*
 * SPDX-FileCopyrightText: Copyright 2026 Ezurio LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_PLAYBACK_H_
#define ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_PLAYBACK_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/iso.h>
#include <zephyr/net_buf.h>

/*
 * The functions declared in this header are provided only when
 * CONFIG_SAMPLE_BT_AUDIO_PLAYBACK is enabled. Their implementation sources
 * (audio_playback.c and the selected PCM sink backend) are omitted from the
 * build otherwise - there is no no-op stub to link against - so every caller
 * must guard the call with IS_ENABLED(CONFIG_SAMPLE_BT_AUDIO_PLAYBACK).
 */

/**
 * @brief Decoded PCM format handed to the playback helper by the sample.
 *
 * All fields come from the LE Audio codec configuration for the accepted
 * stream. This helper decodes LC3 only - the sample advertises LC3-only PAC
 * capabilities, so no other codec is ever negotiated - and the values below
 * are the LC3-supported set rather than a BAP-preset restriction. The helper
 * uses them to configure LC3, the I2S peripheral and the codec driver.
 */
struct sample_bt_audio_pcm_cfg {
	uint32_t freq_hz;             /**< LC3 rate: 8000, 16000, 24000, 32000 or 48000 */
	uint32_t frame_duration_us;   /**< LC3 frame duration: 7500 or 10000 */
	uint16_t octets_per_frame;    /**< per-channel LC3 frame size */
	uint8_t chan_cnt;             /**< 1 or 2 */
	uint8_t frame_blocks_per_sdu; /**< usually 1 */
};

/**
 * @brief Initialise the playback helper.
 *
 * Must be called once at boot before any stream is started. Only available when
 * CONFIG_SAMPLE_BT_AUDIO_PLAYBACK is enabled (see the note at the top of this
 * header); guard the call with IS_ENABLED(CONFIG_SAMPLE_BT_AUDIO_PLAYBACK).
 */
int sample_bt_audio_playback_init(void);

/**
 * @brief Start playing decoded PCM for a newly accepted stream.
 *
 * Safe to call from a BAP RX workqueue context; the actual codec/I2S
 * bring-up is deferred to the system workqueue to avoid starving the host.
 */
int sample_bt_audio_playback_start(const struct sample_bt_audio_pcm_cfg *cfg);

/**
 * @brief Stop feeding the codec.
 *
 * The DMA is not aborted; queued blocks drain and the codec parks. This blocks:
 * it takes an internal mutex, waits for the decode worker to park, and waits for
 * the queued DMA to drain, so it must be called from thread context and never
 * from an ISR.
 */
void sample_bt_audio_playback_stop(void);

/**
 * @brief Queue a received BAP SDU for decoding and playback.
 *
 * The heavy LC3 decode runs on the helper's worker thread rather than the
 * caller's context, so this only references the SDU and enqueues it. SDUs not
 * flagged @c BT_ISO_FLAGS_VALID, and SDUs whose size does not match the codec
 * configuration, are passed to LC3 packet-loss concealment instead of being
 * decoded.
 *
 * @param info ISO receive info from the BAP stream_recv callback.
 * @param buf  ISO SDU. The helper takes a reference on the net_buf and
 *             releases it once the decode worker has consumed it.
 */
void sample_bt_audio_playback_recv(const struct bt_iso_recv_info *info, struct net_buf *buf);

/**
 * @brief Apply a VCS-scale (0..255) output volume to the codec.
 *
 * Cached and re-applied on the next codec (re)configure.
 */
void sample_bt_audio_playback_set_volume(uint8_t vcs_volume);

/**
 * @brief Mute or unmute the codec output.
 */
void sample_bt_audio_playback_set_mute(bool mute);

#endif /* ZEPHYR_SAMPLES_BLUETOOTH_AUDIO_COMMON_SAMPLE_BT_AUDIO_PLAYBACK_H_ */
