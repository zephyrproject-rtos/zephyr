/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file providing the API for the CS40L26/27 haptic driver
 * @ingroup cs40l26_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L26_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L26_H_

#include <zephyr/drivers/haptics.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup cs40l26_interface CS40L26/27
 * @ingroup haptics_interface_ext
 * @brief CS40L26 Haptic Driver for LRA and VCM
 * @{
 */

/**
 * @brief Wavetable sources for effects
 */
enum cs40l26_source {
	CS40L26_SOURCE_BUZ = HAPTICS_SOURCE_PRIV_START, /**< Playback from the buzz library */
};

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTICS_CS40L26_H_ */
