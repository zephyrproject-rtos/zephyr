/*
 * Copyright (c) 2026 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LED Matrix driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_MATRIX_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_MATRIX_H_

#include <limits.h>

#include <zephyr/drivers/led.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Special value indicating that matrix refreshing can be suspended.
 */
#define LED_MATRIX_REFRESH_SUSPEND INT_MAX

#ifdef CONFIG_LED_MATRIX_REFRESH_METHOD_EXTERNAL
/**
 * @brief Refresh the LED matrix.
 *
 * To suspend refreshing to save processing or power, the caller must keep track
 * of the matrix state. LED_MATRIX_REFRESH_SUSPEND can be used as a hint, but
 * due to data races, this is not guaranteed to work. The function may return
 * LED_MATRIX_REFRESH_SUSPEND while a higher-priority thread enabled a LED in
 * the matrix.
 *
 * @param dev      Pointer to the LED matrix device.
 * @param prev_row The previous row that was refreshed, or -1 if no row was refreshed yet.
 *
 * @retval 0..num_rows-1 The currently active row
 * @retval LED_MATRIX_REFRESH_SUSPEND Refreshing can be suspended (all LEDs off)
 * @retval negative error code on failure
 */
int led_matrix_refresh(const struct device *dev, int prev_row);
#endif /* CONFIG_LED_MATRIX_REFRESH_METHOD_EXTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_MATRIX_H_ */
