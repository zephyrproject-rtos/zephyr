/*
 * Copyright (c) 2023 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gnss.h>

extern const struct gpio_dt_spec led;
extern const struct gpio_dt_spec button;
extern const struct gpio_dt_spec vext_ctl;
extern const struct gpio_dt_spec adc_ctl;
extern const struct gpio_dt_spec tft_en;
extern const struct gpio_dt_spec tft_led_en;
extern const struct gpio_dt_spec gnss_rst;
extern const struct gpio_dt_spec gnss_wakeup;

extern const struct device *led_strip;
extern const struct device *adc_dev;
extern const struct device *tft_display_dev;
extern const struct device *lora_dev;
extern const struct device *gnss_dev;

#endif /* BOARD_INIT_H */
