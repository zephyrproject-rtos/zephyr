/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended API of ADS1x4s0x ADC
 * @ingroup ads1x4s0x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADS1X4S0X_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADS1X4S0X_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Texas Instruments ADS1x4s0x
 * @defgroup ads1x4s0x_interface ADS1x4s0x
 * @ingroup adc_interface_ext
 * @{
 */

/**
 * @brief Configure a GPIO pin of an ADS1x4s0x ADC as an output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param initial_value Initial value of the pin.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_set_output(const struct device *dev, uint8_t pin, bool initial_value);

/**
 * @brief Configure a GPIO pin of an ADS1x4s0x ADC as an input.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_set_input(const struct device *dev, uint8_t pin);

/**
 * @brief Deconfigure a GPIO pin of an ADS1x4s0x ADC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_deconfigure(const struct device *dev, uint8_t pin);

/**
 * @brief Set the value of a GPIO pin of an ADS1x4s0x ADC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param value Value to set.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_set_pin_value(const struct device *dev, uint8_t pin,
				bool value);

/**
 * @brief Get the value of a GPIO pin of an ADS1x4s0x ADC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param value Pointer to where the value will be stored.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_get_pin_value(const struct device *dev, uint8_t pin,
				bool *value);

/**
 * @brief Get the value of the GPIO port of an ADS1x4s0x ADC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Pointer to where the port value will be stored.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_port_get_raw(const struct device *dev,
			       gpio_port_value_t *value);

/**
 * @brief Set the value of the GPIO port of an ADS1x4s0x ADC with a mask.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask Mask of pins to change.
 * @param value Value to set.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_port_set_masked_raw(const struct device *dev,
				      gpio_port_pins_t mask,
				      gpio_port_value_t value);

/**
 * @brief Toggle bits of the GPIO port of an ADS1x4s0x ADC.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Mask of pins to toggle.
 *
 * @retval 0 success.
 * @retval -errno negative errno on failure.
 */
int ads1x4s0x_gpio_port_toggle_bits(const struct device *dev,
				   gpio_port_pins_t pins);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_ADS1X4S0X_H_ */
