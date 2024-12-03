/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_PWM_TO_GPIO_LOOPBACK_H__
#define __TEST_PWM_TO_GPIO_LOOPBACK_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/ztest.h>

#define PWM_LOOPBACK_NODE DT_INST(0, test_pwm_to_gpio_loopback)

#define PWM_LOOPBACK_OUT_CTLR	 DT_PWMS_CTLR(PWM_LOOPBACK_NODE)
#define PWM_LOOPBACK_OUT_CHANNEL DT_PWMS_CHANNEL(PWM_LOOPBACK_NODE)
#define PWM_LOOPBACK_OUT_PERIOD	 DT_PWMS_PERIOD(PWM_LOOPBACK_NODE)
#define PWM_LOOPBACK_OUT_FLAGS	 DT_PWMS_FLAGS(PWM_LOOPBACK_NODE)

#define GPIO_LOOPBACK_IN_CTRL  DT_GPIO_CTLR(PWM_LOOPBACK_NODE, gpios)
#define GPIO_LOOPBACK_IN_PIN   DT_GPIO_PIN(PWM_LOOPBACK_NODE, gpios)
#define GPIO_LOOPBACK_IN_FLAGS DT_GPIO_FLAGS(PWM_LOOPBACK_NODE, gpios)

void get_test_devices(struct pwm_dt_spec *out, struct gpio_dt_spec *in);

#endif /* __TEST_PWM_TO_GPIO_LOOPBACK_H__ */
