/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_HI3861_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_HI3861_H_

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

int pinctrl_hi3861_set_pullup(gpio_pin_t pin, int en);

int pinctrl_hi3861_set_pulldown(gpio_pin_t pin, int en);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_HI3861_H_ */
