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

/**
 * @brief Decoded PCM format handed to the playback helper by the sample.
 *
 * All fields come from the LE Audio (BAP/CAP) codec configuration for the
 * accepted stream. The helper uses them to configure LC3, the I2S peripheral
 * and the codec driver.
 */
struct sample_bt_audio_pcm_cfg {
	uint32_t freq_hz;             /**< 8000, 16000, 24000, 32000 or 48000 */
	uint32_t frame_duration_us;   /**< 7500 or 10000 */
	uint16_t octets_per_frame;    /**< per-channel LC3 frame size */
	uint8_t chan_cnt;             /**< 1 or 2 */
	uint8_t frame_blocks_per_sdu; /**< usually 1 */
};

/**
 * @brief Initialise the playback helper.
 *
 * Must be called once at boot before any stream is started. When the
 * required DT aliases (@c i2s_codec_tx, @c audio_codec) or Kconfig
 * dependencies are absent the helper compiles to a no-op and this call
 * succeeds without touching any hardware.
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
 * The DMA is not aborted; queued blocks drain and the codec parks. Cheap
 * enough to call from any context.
 */
void sample_bt_audio_playback_stop(void);

/**
 * @brief Decode a received BAP SDU and queue it for playback.
 *
 * SDUs flagged @c BT_ISO_FLAGS_LOST or @c BT_ISO_FLAGS_ERROR, and SDUs whose
 * size does not match the codec configuration, are passed to LC3 packet-loss
 * concealment instead of being decoded. The @c BT_ISO_FLAGS_VALID bit is
 * intentionally not required, because some BAP unicast paths deliver valid
 * SDUs without explicitly asserting it.
 *
 * @param info ISO receive info from the BAP stream_recv callback.
 * @param buf  ISO SDU. The helper only reads the buffer's payload during this
 *             call; the Bluetooth stack retains ownership of the net_buf.
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
