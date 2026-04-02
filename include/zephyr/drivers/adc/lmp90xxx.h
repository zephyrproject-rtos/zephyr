/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended API of LMP90xxx ADC
 * @ingroup lmp90xxx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_LMP90XXX_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_LMP90XXX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Texas Instruments LMP90xxx Analog Front End (AFE)
 * @defgroup lmp90xxx_interface LMP90xxx
 * @ingroup adc_interface_ext
 * @{
 */

/**
 * @brief Maximum pin number supported by LMP90xxx
 *
 * LMP90xxx supports GPIO D0..D6
 */
#define LMP90XXX_GPIO_MAX 6

/**
 * @brief Configure a GPIO pin of an LMP90xxx as an output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number. The value must be between 0 and LMP90XXX_GPIO_MAX.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_set_output(const struct device *dev, uint8_t pin);

/**
 * @brief Configure a GPIO pin of an LMP90xxx as an input.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number. The value must be between 0 and LMP90XXX_GPIO_MAX.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_set_input(const struct device *dev, uint8_t pin);

/**
 * @brief Set the value of a GPIO pin of an LMP90xxx.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number. The value must be between 0 and LMP90XXX_GPIO_MAX.
 * @param value Value to set.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_set_pin_value(const struct device *dev, uint8_t pin,
				bool value);

/**
 * @brief Get the value of a GPIO pin of an LMP90xxx.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number. The value must be between 0 and LMP90XXX_GPIO_MAX.
 * @param value Pointer to where the value will be stored.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_get_pin_value(const struct device *dev, uint8_t pin,
				bool *value);

/**
 * @brief Get the value of the GPIO port of an LMP90xxx.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Pointer to where the port value will be stored.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_port_get_raw(const struct device *dev,
			       gpio_port_value_t *value);

/**
 * @brief Set the value of the GPIO port of an LMP90xxx with a mask.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask Mask of pins to change.
 * @param value Value to set.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_port_set_masked_raw(const struct device *dev,
				      gpio_port_pins_t mask,
				      gpio_port_value_t value);

/**
 * @brief Set bits of the GPIO port of an LMP90xxx.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Mask of pins to set.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_port_set_bits_raw(const struct device *dev,
				    gpio_port_pins_t pins);

/**
 * @brief Clear bits of the GPIO port of an LMP90xxx.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Mask of pins to clear.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_port_clear_bits_raw(const struct device *dev,
				      gpio_port_pins_t pins);

/**
 * @brief Toggle bits of the GPIO port of an LMP90xxx.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Mask of pins to toggle.
 *
 * @retval 0 on success.
 * @retval -EIO or other negative errno if communication failed.
 */
int lmp90xxx_gpio_port_toggle_bits(const struct device *dev,
				   gpio_port_pins_t pins);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_LMP90XXX_H_ */
