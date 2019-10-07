/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_LMP90XXX_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_LMP90XXX_H_

#include <device.h>
#include <zephyr/types.h>

/* LMP90xxx supports GPIO D0..D6 */
#define LMP90XXX_GPIO_MAX 6

int lmp90xxx_gpio_set_output(struct device *dev, u8_t pin);

int lmp90xxx_gpio_set_input(struct device *dev, u8_t pin);

int lmp90xxx_gpio_set_pin_value(struct device *dev, u8_t pin, bool value);

int lmp90xxx_gpio_get_pin_value(struct device *dev, u8_t pin, bool *value);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_LMP90XXX_H_ */
