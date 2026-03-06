/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L5X_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L5X_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_instance.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup cs40l5x_interface CS40L5x
 * @ingroup haptics_interface_ext
 * @brief CS40L5x Haptic Driver
 * @{
 */

/**
 * @brief Attenuation options for triggered haptic effects
 * @details Provide to @ref cs40l5x_configure_trigger().
 */
enum cs40l5x_attenuation {
	CS40L5X_ATTENUATION_7DB = -7, /**< Configure haptic effect with 7 dB attenuation */
	CS40L5X_ATTENUATION_6DB,      /**< Configure haptic effect with 6 dB attenuation */
	CS40L5X_ATTENUATION_5DB,      /**< Configure haptic effect with 5 dB attenuation */
	CS40L5X_ATTENUATION_4DB,      /**< Configure haptic effect with 4 dB attenuation */
	CS40L5X_ATTENUATION_3DB,      /**< Configure haptic effect with 3 dB attenuation */
	CS40L5X_ATTENUATION_2DB,      /**< Configure haptic effect with 2 dB attenuation */
	CS40L5X_ATTENUATION_1DB,      /**< Configure haptic effect with 1 dB attenuation */
	CS40L5X_ATTENUATION_0DB,      /**< Configure haptic effect with no attenuation */
};

/**
 * @brief Wavetable sources for haptic effects
 * @details Provide to @ref cs40l5x_configure_trigger() or @ref cs40l5x_select_output().
 */
enum cs40l5x_bank {
	CS40L5X_ROM_BANK,    /**< Playback from the pre-programmed ROM library */
	CS40L5X_CUSTOM_BANK, /**< Playback from custom haptics source programmed at runtime */
	CS40L5X_BUZ_BANK,    /**< Playback from buzz source programmed at runtime */
	CS40L5X_NO_BANK,     /**< Reserved for driver error handling */
};

/**
 * @brief Custom haptics source indices (0 or 1)
 * @details Provide to @ref cs40l5x_upload_pcm() or @ref cs40l5x_upload_pwle() to specify
 * index to upload custom haptics source.
 */
enum cs40l5x_custom_index {
	CS40L5X_CUSTOM_0,           /**< Custom haptics source at index 0 */
	CS40L5X_CUSTOM_1,           /**< Custom haptics source at index 1 */
	CS40L5X_NUM_CUSTOM_EFFECTS, /**< Maximum number of custom haptics effects */
};

/**
 * @brief Options for runtime haptics logging
 * @details Provide to @ref cs40l5x_logger() to update runtime haptics logging.
 */
enum cs40l5x_logger {
	CS40L5X_LOGGER_DISABLE,   /**< Disable runtime logging for the device */
	CS40L5X_LOGGER_ENABLE,    /**< Enable runtime logging for the device */
	CS40L5X_LOGGER_NO_CHANGE, /**< Use to retrieve haptics logging status without updating */
};

/**
 * @brief Options for runtime haptics logging sources
 * @details Provide to @ref cs40l5x_logger_get() to get runtime device characteristics.
 */
enum cs40l5x_logger_source {
	CS40L5X_LOGGER_BEMF, /**< Back EMF (SVC) in signed Q0.23 format (full scale is 24 V) */
	CS40L5X_LOGGER_VBST, /**< Boost voltage in unsigned Q0.24 format (full scale is 14 V) */
	CS40L5X_LOGGER_VMON, /**< Output voltage in signed Q0.23 format (full scale is 24 V) */
};

/**
 * @brief Type specification for runtime haptics logging sources
 * @details Provide to @ref cs40l5x_logger_get() to get minimum, mean, or maximum logged values.
 */
enum cs40l5x_logger_source_type {
	CS40L5X_LOGGER_MIN,  /**< Minimum value sampled for a logger source */
	CS40L5X_LOGGER_MAX,  /**< Maximum value sampled for a logger source */
	CS40L5X_LOGGER_MEAN, /**< Mean value sampled for a logger source */
};

/**
 * @brief PWLE section definition
 * @details Provide array of sections to @ref cs40l5x_upload_pwle().
 */
struct cs40l5x_pwle_section {
	/**< Section duration in unsigned Q14.2 format (time) or Q16.0 format (half-cycles) */
	uint16_t duration;
	/**< Section frequency in unsigned Q10.2 format */
	uint16_t frequency;
	/**< Section level in unsigned Q0.11 format */
	uint16_t level;
	/**< Section flags in unsigned Q4.0 format */
	uint8_t flags;
};

/**
 * @brief Options for edge-triggered haptics effects
 * @details Provide to @ref cs40l5x_configure_trigger() to specify the edge for haptic effects.
 */
enum cs40l5x_trigger_edge {
	CS40L5X_RISING_EDGE,  /**< Configure a rising-edge haptic effect */
	CS40L5X_FALLING_EDGE, /**< Configure a falling-edge haptic effect */
};

/**
 * @brief Run calibration to derive ReDC and F0 values and apply results for click compensation
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_calibrate(const struct device *const dev);

/**
 * @brief Configure ROM buzz for haptic playback
 *
 * @details With large amplitudes and insufficient power (e.g., in the case of internal boost
 * configurations), it's possible to induce boost errors.
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] frequency Frequency of haptic effect in Hz (default: 0xF0)
 * @param[in] level Amplitude of haptic effect, where UINT8_MAX is 100% (default: 0x1B)
 * @param[in] duration Playback duration in milliseconds, where 0 is indefinite (default: 0x32)
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_configure_buzz(const struct device *const dev, const uint32_t frequency,
			   const uint8_t level, const uint32_t duration);

/**
 * @brief Configure edge-triggered haptic effect
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] gpio Pointer to the device structure for the GPIO used as the trigger source
 * @param[in] bank Wavetable source for desired haptic effect, see @ref cs40l5x_bank
 * @param[in] index Wavetable index for desired haptic effect
 * @param[in] attenuation Attenuation in dB for desired haptic effect, see @ref cs40l5x_attenuation
 * @param[in] edge Specify edge (rising or falling) to trigger haptic effects
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_configure_trigger(const struct device *const dev, const struct gpio_dt_spec *const gpio,
			      const enum cs40l5x_bank bank, const uint8_t index,
			      const enum cs40l5x_attenuation attenuation,
			      const enum cs40l5x_trigger_edge edge);

/**
 * @brief Update runtime haptics logging and get current status
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] logger_state See @ref cs40l5x_logger
 *
 * @retval 1 if logging is enabled
 * @retval 0 if logging is disabled
 * @retval <0 if failed
 */
int cs40l5x_logger(const struct device *const dev, enum cs40l5x_logger logger_state);

/**
 * @brief Get runtime haptics logging data for the specified logger source
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] source See @ref cs40l5x_logger_source
 * @param[in] type See @ref cs40l5x_logger_source_type
 * @param[out] value Unsigned 32-bit integer to store the retrieved data
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_logger_get(const struct device *const dev, enum cs40l5x_logger_source source,
		       enum cs40l5x_logger_source_type type, uint32_t *const value);

/**
 * @brief Select haptic effect triggered via @ref haptics_start_output()
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] bank Wavetable source for desired haptic effect, see @ref cs40l5x_bank
 * @param[in] index Wavetable index for desired haptic effect
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_select_output(const struct device *const dev, const enum cs40l5x_bank bank,
			  const uint8_t index);

/**
 * @brief Configure gain for haptic effects triggered via @ref haptics_start_output()
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] gain Gain setting (valid values between 0 and 100)
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_set_gain(const struct device *const dev, const uint8_t gain);

/**
 * @brief Upload PCM effect to the specified index
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] index See @ref cs40l5x_custom_index
 * @param[in] redc ReDC compensation in unsigned Q5.7 format | redc * (24 / 2.9)
 * @param[in] f0 F0 compensation in unsigned Q9.3 format | (f0 - 50) * 8
 * @param[in] samples Array of signed 8-bit PCM samples
 * @param[in] num_samples Number of PCM samples
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_upload_pcm(const struct device *const dev, const enum cs40l5x_custom_index index,
		       const uint16_t redc, const uint16_t f0, const int8_t *const samples,
		       const uint16_t num_samples);

/**
 * @brief Upload PWLE effect to the specified index
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] index See @ref cs40l5x_custom_index
 * @param[in] sections Array of @ref cs40l5x_pwle_section
 * @param[in] num_sections Number of PWLE sections
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l5x_upload_pwle(const struct device *const dev, const enum cs40l5x_custom_index index,
			const struct cs40l5x_pwle_section *const sections,
			const uint8_t num_sections);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
