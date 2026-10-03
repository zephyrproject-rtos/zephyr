/**
 * @file
 * @brief BAP Broadcast Sink I2S playback logic.
 *
 * This module is the pure-software playback layer that sits on top of the
 * hardware-specific SAI/I2S transport in hw_codec_i2s.c. It owns:
 *  - an elastic PCM ring buffer that decouples the LC3 decoder (producer) from
 *    the SAI (consumer), and
 *  - a playback thread that keeps the SAI DMA fed, with a prebuffer latch that
 *    builds a cushion before playing so ordinary arrival jitter is not
 *    mistaken for an underrun.
 *
 * It contains no hardware register access: the SAI device handle, the DMA
 * memory slab, the per-block size and the frame duration are supplied by
 * hw_codec_i2s.c through i2s_play_setup(). This keeps the hardware
 * configuration (hw_codec_i2s.c) separate from the playback/buffering policy
 * (this file).
 *
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BAP_BROADCAST_SINK_I2S_PLAY_H_
#define BAP_BROADCAST_SINK_I2S_PLAY_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

/**
 * @brief Provide the hardware handles and stream timing used for playback.
 *
 * Called by hw_codec_i2s_cfg() after the SAI/I2S peripheral has been
 * configured and the DMA memory slab initialised. The playback module
 * initialises its own state (PCM ring and thread) lazily on the first call, so
 * the hardware layer does not need a separate init entry point. Safe to call
 * again on a reconfiguration; it only updates the handles and (re)arms the
 * playback thread.
 *
 * @param dev          Configured I2S transmit device.
 * @param slab         DMA memory slab used for both PCM and silence blocks.
 * @param block_size   Per-block size in bytes for the current configuration.
 * @param frame_dur_us Duration of one block in microseconds. Used to derive
 *                     the consumer starvation timeout, so that it scales with
 *                     the negotiated frame duration instead of being fixed.
 */
void i2s_play_setup(const struct device *dev, struct k_mem_slab *slab, uint32_t block_size,
		    uint32_t frame_dur_us);

/**
 * @brief Queue PCM data for playback (producer side).
 *
 * Copies the PCM bytes into ring blocks and returns immediately; never blocks
 * on the SAI. If the ring is full the oldest block is recycled (overrun
 * protection).
 *
 * @param data Pointer to the PCM sample buffer.
 * @param len  Number of bytes to queue.
 *
 * @return Number of bytes accepted.
 */
uint32_t i2s_play_write(const uint8_t *data, uint32_t len);

/**
 * @brief Stop playback, join the playback thread and drain the PCM ring.
 *
 * Returns once the playback thread has exited and every block it owned has
 * been returned to the slab, so the caller may safely reconfigure the stream
 * afterwards.
 */
void i2s_play_stop(void);

#endif /* BAP_BROADCAST_SINK_I2S_PLAY_H_ */
