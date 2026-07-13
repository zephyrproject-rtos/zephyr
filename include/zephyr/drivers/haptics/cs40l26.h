/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L26_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L26_H_

#include <zephyr/drivers/haptics.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @file
 * @brief Public API for CS40L26 haptic driver
 * @ingroup haptics_interface_ext
 */

/**
 * @brief Wavetable sources for effects
 *
 * @details Provide to @ref cs40l26_select_output().
 */
enum cs40l26_bank {
	CS40L26_ROM_BANK, /**< Playback from the pre-programmed ROM library */
	CS40L26_BUZ_BANK, /**< Playback from buzz source programmed at runtime */
	CS40L26_NO_BANK,  /**< Reserved for driver error handling */
};

/**
 * @brief Options for edge-triggered haptics effects
 * @details Provide to @ref cs40l26_configure_trigger() to specify the edge for haptic effects.
 */
enum cs40l26_trigger_edge {
	CS40L26_RISING_EDGE,  /**< Configure a rising-edge haptic effect */
	CS40L26_FALLING_EDGE, /**< Configure a falling-edge haptic effect */
};

/**
 * @brief Run calibration to derive ReDC and F0 values and apply results for click compensation
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l26_calibrate(const struct device *const dev);

/**
 * @brief Configure ROM buzz for haptic playback
 *
 * @details With large amplitudes and insufficient power, it's possible to reach the current limit.
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] frequency Frequency of haptic effect in Hz (default: 0xA5)
 * @param[in] level Amplitude of haptic effect, where UINT8_MAX is 100% (default: 0x40)
 * @param[in] duration Playback duration in units of 4 milliseconds
 *
 * @retval 0 if success
 * @retval <0 if failed
 */
int cs40l26_configure_buzz(const struct device *const dev, const uint32_t frequency,
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
int cs40l26_configure_trigger(const struct device *const dev, const struct gpio_dt_spec *const gpio,
			      const enum haptics_source src, const union haptics_config *const cfg,
			      const int8_t attenuation, const enum cs40l26_trigger_edge edge);

/**
 * @brief Select haptic effect triggered via @ref haptics_start_output()
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] bank Refer to @ref cs40l26_bank
 * @param[in] index Wavetable index for desired haptic effect
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
int cs40l26_select_output(const struct device *const dev, const enum cs40l26_bank bank,
			  const uint16_t index);

/**
 * @brief Configure gain for haptic effects triggered via @ref haptics_start_output()
 *
 * @param[in] dev Pointer to the device structure for haptic device instance
 * @param[in] gain Gain setting (valid values between 0 and 100)
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
int cs40l26_set_gain(const struct device *const dev, const uint8_t gain);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L26_H_ */
