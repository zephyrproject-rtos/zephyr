/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PWM_WAVESHARE_CH32V003_H_
#define ZEPHYR_DRIVERS_PWM_WAVESHARE_CH32V003_H_

#include <stdint.h>
#include <zephyr/device.h>

int ch32v003_set_pwm_duty(const struct device *dev, uint8_t duty);

#endif /* ZEPHYR_DRIVERS_PWM_WAVESHARE_CH32V003_H_ */
