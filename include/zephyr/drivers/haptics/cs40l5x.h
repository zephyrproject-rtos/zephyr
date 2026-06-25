/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file providing the API for the CS40L5x haptic driver
 * @ingroup cs40l5x_interface
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
 * @brief CS40L5x Haptic Driver for LRA and VCM
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
 */
enum cs40l5x_source {
	CS40L5X_SOURCE_BUZ = HAPTICS_SOURCE_PRIV_START, /**< Playback from the buzz library */
	CS40L5X_SOURCE_RTH,                             /**< Playback from the runtime library */
};

/**
 * @brief PWLE section definition
 * @details Provide array of sections to @ref cs40l5x_upload_pwle().
 */
struct cs40l5x_pwle_section {
	/** Section duration in unsigned Q14.2 format (time) or Q16.0 format (half-cycles) */
	uint16_t duration;
	/** Section frequency in unsigned Q10.2 format */
	uint16_t frequency;
	/** Section level in unsigned Q0.11 format */
	uint16_t level;
	/** Section flags in unsigned Q4.0 format */
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
 * @return 0 on success, negative errno value on failure.
 * @retval -EIO A control port transaction failed.
 */
int cs40l5x_configure_buzz(const struct device *const dev, const uint32_t frequency,
			   const uint8_t level, const uint32_t duration);

/**
 * @brief Configure edge-triggered haptic effect
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] gpio Pointer to the device structure for the GPIO used as the trigger source
 * @param[in] src Playback source (of type @ref haptics_source)
 * @param[in] cfg Source configuration (of type @ref haptics_config) or NULL
 * @param[in] attenuation Attenuation in dB for desired haptic effect (range 0 to 7)
 * @param[in] edge Specify edge (rising or falling) to trigger haptic effects
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL Invalid wavetable source and index provided (e.g., index out of bounds).
 * @retval -EIO A control port transaction failed.
 */
int cs40l5x_configure_trigger(const struct device *const dev, const struct gpio_dt_spec *const gpio,
			      const enum haptics_source src, const union haptics_config *const cfg,
			      const enum cs40l5x_attenuation attenuation,
			      const enum cs40l5x_trigger_edge edge);

/**
 * @brief Upload PCM effect to the specified index
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] index Wavetable index
 * @param[in] redc ReDC compensation in unsigned Q5.7 format | redc * (24 / 2.9)
 * @param[in] f0 F0 compensation in unsigned Q9.3 format | (f0 - 50) * 8
 * @param[in] samples Array of signed 8-bit PCM samples
 * @param[in] num_samples Number of PCM samples
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL Invalid index provided (e.g., index out of bounds).
 * @retval -EIO A control port transaction failed.
 */
int cs40l5x_upload_pcm(const struct device *const dev, const int index, const uint16_t redc,
		       const uint16_t f0, const int8_t *const samples, const uint16_t num_samples);

/**
 * @brief Upload PWLE effect to the specified index
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] index Wavetable index
 * @param[in] sections Array of @ref cs40l5x_pwle_section
 * @param[in] num_sections Number of PWLE sections
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL Invalid index provided (e.g., index out of bounds).
 * @retval -EIO A control port transaction failed.
 */
int cs40l5x_upload_pwle(const struct device *const dev, const int index,
			const struct cs40l5x_pwle_section *const sections,
			const uint8_t num_sections);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L5X_H_ */
