/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SX1509B_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SX1509B_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure a pin for LED intensity.
 *
 * Configure a pin to be controlled by SX1509B LED driver using
 * the LED intensity functionality.
 * To get back normal GPIO functionality, configure the pin using
 * the standard GPIO API.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 0 If successful.
 * @retval -EWOULDBLOCK if function is called from an ISR.
 * @retval -ERANGE if pin number is out of range.
 * @retval -EIO if I2C fails.
 */
int sx1509b_led_intensity_pin_configure(const struct device *dev,
					gpio_pin_t pin);

/**
 * @brief Set LED intensity of selected pin.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param intensity_val Intensity value.
 *
 * @retval 0 If successful.
 * @retval -EIO if I2C fails.
 */
int sx1509b_led_intensity_pin_set(const struct device *dev, gpio_pin_t pin,
				  uint8_t intensity_val);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SX1509B_H_ */
