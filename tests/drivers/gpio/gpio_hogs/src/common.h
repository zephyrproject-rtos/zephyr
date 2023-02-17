/*
 * Copyright 2023 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_DRIVERS_GPIO_GPIO_HOGS_COMMON_H_
#define ZEPHYR_TEST_DRIVERS_GPIO_GPIO_HOGS_COMMON_H_

#include <zephyr/drivers/gpio.h>

void assert_gpio_hog_direction(const struct gpio_dt_spec *spec, bool output);
void assert_gpio_hog_config(const struct gpio_dt_spec *spec, gpio_flags_t expected);

#endif /* ZEPHYR_TEST_DRIVERS_GPIO_GPIO_HOGS_COMMON_H_ */
