/**
 * @file
 * @brief BAP Broadcast Sink I2S/SAI PCM transport public interface.
 *
 * This header declares the SAI/I2S data path helpers used by hw_codec.c on
 * boards whose codec is a control-only device (for example the WM8962 on NXP
 * i.MX RT boards). The codec control driver only implements the control API,
 * so the PCM samples are transported to the codec over the SoC SAI/I2S
 * peripheral using the Zephyr I2S driver API.
 *
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BAP_BROADCAST_SINK_HW_CODEC_I2S_H_
#define BAP_BROADCAST_SINK_HW_CODEC_I2S_H_

#include <stdint.h>

/*
 * PCM sample width in bits.
 *
 * The per-block size and the channel count are no longer fixed: the block
 * size is computed at run time in hw_codec_i2s_cfg() from the sample rate,
 * this sample width and the number of channels passed in, so multi-BIS
 * (stereo and beyond) and different sample rates are handled automatically.
 */
#define CODEC_PCM_WIDTH_BITS 16U


/**
 * @brief Initialise the I2S tx device and DMA memory slab.
 *
 * @return 0 on success, negative errno otherwise.
 */
int hw_codec_i2s_init(void);

/**
 * @brief Configure the I2S tx stream for the given sample rate, channels and
 *        LC3 frame duration.
 *
 * The per-block size is computed at run time from the sample rate, PCM sample
 * width, the number of channels and the LC3 frame duration, so the same call
 * adapts to different sample rates, frame durations and BIS counts (mono,
 * stereo, ...). The block size follows the LC3 frame duration (per the BT LE
 * Audio spec, either 7500 us or 10000 us), not the ISO interval.
 *
 * @param samplerate   PCM frame clock frequency in Hz.
 * @param channels     Number of interleaved PCM channels (BIS streams rendered).
 * @param frame_dur_us LC3 frame duration in microseconds (7500 or 10000).
 *
 * @return 0 on success, negative errno otherwise.
 */
int hw_codec_i2s_cfg(uint32_t samplerate, uint8_t channels, uint32_t frame_dur_us);


/**
 * @brief Queue PCM data to the I2S tx stream.
 *
 * @param data Pointer to the PCM sample buffer.
 * @param len  Number of bytes to write.
 *
 * @return Number of bytes accepted.
 */
uint32_t hw_codec_i2s_write(const uint8_t *data, uint32_t len);

/**
 * @brief Drain and stop the I2S tx stream.
 *
 * @return 0 on success, negative errno otherwise.
 */
int hw_codec_i2s_close(void);

#endif /* BAP_BROADCAST_SINK_HW_CODEC_I2S_H_ */
