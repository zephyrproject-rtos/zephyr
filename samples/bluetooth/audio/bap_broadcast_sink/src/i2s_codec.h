/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink sample I2S codec output header
 *
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLE_BAP_BROADCAST_SINK_I2S_CODEC_H
#define SAMPLE_BAP_BROADCAST_SINK_I2S_CODEC_H

/**
 * @brief Initialize the I2S codec output
 *
 * Configures the codec and the I2S controller and starts playing the stereo output buffer
 *
 * @retval 0 Success
 * @retval -EALREADY Already initialized
 * @retval -ENODEV The codec or the I2S controller is not ready
 * @retval <0 Negative errno from the codec, I2S or memory slab call that failed
 */
int i2s_codec_init(void);

#endif /* SAMPLE_BAP_BROADCAST_SINK_I2S_CODEC_H */
