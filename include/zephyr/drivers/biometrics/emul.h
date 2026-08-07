/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Test helper APIs for the biometrics emulator.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BIOMETRICS_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_BIOMETRICS_EMUL_H_

#include <stdbool.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Biometrics emulator test control functions
 * @defgroup biometrics_emulator Biometrics emulator test helpers
 * @ingroup io_emulators
 * @ingroup biometrics_interface
 * @{
 */

/**
 * @brief Set the match score for the biometrics emulator
 *
 * Configures the confidence/match score that will be returned by the next
 * match operation. Used for testing match verification and identification.
 *
 * @param dev Pointer to the biometrics device
 * @param score Match score to return (sensor-specific scale)
 */
void biometrics_emul_set_match_score(const struct device *dev, int score);

/**
 * @brief Set the template ID for identification matches
 *
 * Configures which template ID will be returned when performing an
 * IDENTIFY match operation. Used for testing identification scenarios.
 *
 * @param dev Pointer to the biometrics device
 * @param id Template ID to return on successful match
 */
void biometrics_emul_set_match_id(const struct device *dev, int id);

/**
 * @brief Set whether match operations should fail
 *
 * Configures the emulator to fail match operations with -ENOENT.
 * Used for testing match failure scenarios.
 *
 * @param dev Pointer to the biometrics device
 * @param fail True to fail match operations, false for normal operation
 */
void biometrics_emul_set_match_fail(const struct device *dev, bool fail);

/**
 * @brief Set whether capture operations should timeout
 *
 * Configures the emulator to simulate capture timeouts. When enabled,
 * enrollment capture operations will return -ETIMEDOUT.
 *
 * @param dev Pointer to the biometrics device
 * @param timeout True to simulate timeout, false for normal operation
 */
void biometrics_emul_set_capture_timeout(const struct device *dev, bool timeout);

/**
 * @brief Set the image quality score
 *
 * Configures the image quality score that will be returned by capture
 * and match operations. Used for testing quality feedback.
 *
 * @param dev Pointer to the biometrics device
 * @param quality Quality score (0-100)
 */
void biometrics_emul_set_image_quality(const struct device *dev, int quality);

/**
 * @brief Get the current LED state
 *
 * Retrieves the current LED state that was set via biometric_led_control().
 * Used for verifying LED control commands.
 *
 * @param dev Pointer to the biometrics device
 * @return Current LED state
 */
enum biometric_led_state biometrics_emul_get_led_state(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_BIOMETRICS_EMUL_H_ */
