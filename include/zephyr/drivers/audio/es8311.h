/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ES8311 mono audio codec driver public API
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
	ES8311_SAMPLE_RATE_8KHZ = 8000,    /**< 8 kHz sample rate */
	ES8311_SAMPLE_RATE_16KHZ = 16000,  /**< 16 kHz sample rate */
	ES8311_SAMPLE_RATE_44_1KHZ = 44100, /**< 44.1 kHz sample rate */
	ES8311_SAMPLE_RATE_48KHZ = 48000,  /**< 48 kHz sample rate */
};

/** ES8311 bit depths */
enum es8311_bit_depth {
	ES8311_BIT_DEPTH_16 = 16, /**< 16-bit PCM */
	ES8311_BIT_DEPTH_24 = 24, /**< 24-bit PCM */
	ES8311_BIT_DEPTH_32 = 32, /**< 32-bit PCM */
};

/** ES8311 audio format */
enum es8311_format {
	ES8311_FORMAT_I2S = 0,            /**< Standard I2S format */
	ES8311_FORMAT_LEFT_JUSTIFIED = 1, /**< Left-justified format */
	ES8311_FORMAT_DSP_A = 3,          /**< DSP/PCM mode A format */
};

/** ES8311 configuration structure */
struct es8311_config {
	enum es8311_sample_rate sample_rate; /**< Codec sample rate */
	enum es8311_bit_depth bit_depth;     /**< PCM bit depth */
	enum es8311_format format;           /**< Serial audio data format */
	uint8_t channels;                    /**< Number of active channels (1 or 2) */
	bool master_mode;                    /**< true for I2S master, false for slave */
	bool use_mclk;                       /**< true to enable MCLK input */
	bool invert_mclk;                    /**< true to invert the MCLK polarity */
	bool invert_sclk;                    /**< true to invert the SCLK (BCLK) polarity */
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
