/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup input_nunchuk
 * @brief Header file for nunchuk input driver synchronous reading.
 */
#ifndef ZEPHYR_INCLUDE_INPUT_INPUT_NUNCHUCK_H_
#define ZEPHYR_INCLUDE_INPUT_INPUT_NUNCHUCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

#ifdef CONFIG_INPUT_NUNCHUK_ACCELERATION

/**
 * @brief Acceleration on the X axis reported by the nunchuk.
 */
#define INPUT_NUNCHUK_ACCEL_X 0

/**
 * @brief Acceleration on the Y axis reported by the nunchuk.
 */
#define INPUT_NUNCHUK_ACCEL_Y 1

/**
 * @brief Acceleration on the Z axis reported by the nunchuk.
 */
#define INPUT_NUNCHUK_ACCEL_Z 2

#endif /* CONFIG_INPUT_NUNCHUK_ACCELERATION */

/**
 * @defgroup input_nunchuk Nunchuk input driver
 * @ingroup input_interface
 * @{
 */

/**
 * @brief Starts the reading process of the current button states, joystick position and optionally
 * the rotation (if INPUT_NUNCHUK_ACCELERATION config option is set) from the nunchuk.
 *
 * This function does only function if the property 'polling-interval-ms' is set to 0 in the DTS
 * defintion for the nunchuck input driver, so that background polling is disabled.
 *
 * Note that the function exits immediately and the data is read from the nunchuk in the background
 * and reported via the standard input event interface.
 *
 * @param dev 	Pointer to the nunchuk device
 * @retval 	SUCCESS if the data read process could be started.
 * @retval	-errno In case of any error.
 */
int nunchuk_read(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_INPUT_INPUT_NUNCHUCK_H_ */
