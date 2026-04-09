/*
 * Copyright (c) 2025 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES7210_H_
#define ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES7210_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ES7210 Audio ADC Driver API
 * @defgroup es7210_interface ES7210 Audio ADC Driver API
 * @ingroup audio_interface
 * @{
 */

/** ES7210 sample rates */
enum es7210_sample_rate {
	ES7210_SAMPLE_RATE_8KHZ = 8000,
	ES7210_SAMPLE_RATE_16KHZ = 16000,
	ES7210_SAMPLE_RATE_32KHZ = 32000,
	ES7210_SAMPLE_RATE_44_1KHZ = 44100,
	ES7210_SAMPLE_RATE_48KHZ = 48000,
};

/** ES7210 bit depths */
enum es7210_bit_depth {
	ES7210_BIT_DEPTH_16 = 16,
	ES7210_BIT_DEPTH_24 = 24,
	ES7210_BIT_DEPTH_32 = 32,
};

/** ES7210 audio format */
enum es7210_format {
	ES7210_FORMAT_I2S = 0,
	ES7210_FORMAT_LEFT_JUSTIFIED = 1,
	ES7210_FORMAT_DSP_A = 3,
};

/** ES7210 microphone channels */
enum es7210_channels {
	ES7210_CHANNELS_1 = 1,
	ES7210_CHANNELS_2 = 2,
	ES7210_CHANNELS_4 = 4,
};

/** ES7210 configuration structure */
struct es7210_config {
	enum es7210_sample_rate sample_rate;
	enum es7210_bit_depth bit_depth;
	enum es7210_format format;
	enum es7210_channels channels;
	bool master_mode;
	bool use_mclk;
	bool invert_mclk;
	bool invert_sclk;
};

/**
 * @brief Initialize ES7210 ADC
 *
 * @param dev ES7210 device
 * @param config Configuration parameters
 * @return 0 on success, negative error code on failure
 */
int es7210_initialize(const struct device *dev, const struct es7210_config *config);

/**
 * @brief Set ES7210 gain
 *
 * @param dev ES7210 device
 * @param gain Gain level (0-255)
 * @return 0 on success, negative error code on failure
 */
int es7210_set_gain(const struct device *dev, uint8_t gain);

/**
 * @brief Mute/unmute ES7210 input
 *
 * @param dev ES7210 device
 * @param mute true to mute, false to unmute
 * @return 0 on success, negative error code on failure
 */
int es7210_set_mute(const struct device *dev, bool mute);

/**
 * @brief Enable/disable ES7210 ADC
 *
 * @param dev ES7210 device
 * @param enable true to enable, false to disable
 * @return 0 on success, negative error code on failure
 */
int es7210_enable(const struct device *dev, bool enable);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUDIO_ES7210_H_ */
