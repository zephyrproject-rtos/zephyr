/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES8311_H_
#define ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES8311_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ES8311 Audio Codec Driver API
 * @defgroup es8311_interface ES8311 Audio Codec Driver API
 * @ingroup audio_interface
 * @{
 */

/** ES8311 sample rates */
enum es8311_sample_rate {
	ES8311_SAMPLE_RATE_8KHZ = 8000,
	ES8311_SAMPLE_RATE_16KHZ = 16000,
	ES8311_SAMPLE_RATE_44_1KHZ = 44100,
	ES8311_SAMPLE_RATE_48KHZ = 48000,
};

/** ES8311 bit depths */
enum es8311_bit_depth {
	ES8311_BIT_DEPTH_16 = 16,
	ES8311_BIT_DEPTH_24 = 24,
	ES8311_BIT_DEPTH_32 = 32,
};

/** ES8311 audio format */
enum es8311_format {
	ES8311_FORMAT_I2S = 0,
	ES8311_FORMAT_LEFT_JUSTIFIED = 1,
	ES8311_FORMAT_DSP_A = 3,
};

/** ES8311 configuration structure */
struct es8311_config {
	enum es8311_sample_rate sample_rate;
	enum es8311_bit_depth bit_depth;
	enum es8311_format format;
	uint8_t channels;
	bool master_mode;
	bool use_mclk;
	bool invert_mclk;
	bool invert_sclk;
};

/**
 * @brief Initialize ES8311 codec
 *
 * @param dev ES8311 device
 * @param config Configuration parameters
 * @return 0 on success, negative error code on failure
 */
int es8311_initialize(const struct device *dev, const struct es8311_config *config);

/**
 * @brief Set ES8311 volume
 *
 * @param dev ES8311 device
 * @param volume Volume level (0-255)
 * @return 0 on success, negative error code on failure
 */
int es8311_set_volume(const struct device *dev, uint8_t volume);

/**
 * @brief Mute/unmute ES8311 output
 *
 * @param dev ES8311 device
 * @param mute true to mute, false to unmute
 * @return 0 on success, negative error code on failure
 */
int es8311_set_mute(const struct device *dev, bool mute);

/**
 * @brief Enable/disable ES8311 codec
 *
 * @param dev ES8311 device
 * @param enable true to enable, false to disable
 * @return 0 on success, negative error code on failure
 */
int es8311_enable(const struct device *dev, bool enable);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES8311_H_ */
