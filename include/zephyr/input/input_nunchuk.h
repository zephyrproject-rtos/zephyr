/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Nunchuk input driver synchronous reading.
 * @ingroup nunchuk_interface
 */

#ifndef ZEPHYR_INCLUDE_INPUT_INPUT_NUNCHUK_H_
#define ZEPHYR_INCLUDE_INPUT_INPUT_NUNCHUK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

/**
 * @defgroup nunchuk_interface Nunchuk
 * @ingroup input_interface_ext
 * @brief Nintendo Nunchuk joystick
 * @{
 */

/**
 * @brief Acceleration on the X axis reported by the Nunchuk.
 *
 * @kconfig_dep{CONFIG_INPUT_NUNCHUK_ACCELERATION}
 */
#define INPUT_NUNCHUK_ACCEL_X 0

/**
 * @brief Acceleration on the Y axis reported by the Nunchuk.
 *
 * @kconfig_dep{CONFIG_INPUT_NUNCHUK_ACCELERATION}
 */
#define INPUT_NUNCHUK_ACCEL_Y 1

/**
 * @brief Acceleration on the Z axis reported by the Nunchuk.
 *
 * @kconfig_dep{CONFIG_INPUT_NUNCHUK_ACCELERATION}
 */
#define INPUT_NUNCHUK_ACCEL_Z 2

/**
 * @brief Starts the reading process of the current button states, joystick position and optionally
 * the acceleration (if @c CONFIG_INPUT_NUNCHUK_ACCELERATION is enabled) from the Nunchuk.
 *
 * This function does only function if the property 'polling-interval-ms' is set to 0 in the DTS
 * definition for the Nunchuk input driver, so that background polling is disabled.
 *
 * Note that the function exits immediately and the data is read from the Nunchuk in the background
 * and reported via the standard input event interface.
 *
 * @param dev	Pointer to the Nunchuk device
 * @retval	SUCCESS if the data read process could be started.
 * @retval	-errno In case of any error.
 */
int nunchuk_read(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_INPUT_INPUT_NUNCHUK_H_ */
