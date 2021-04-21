/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_LED_API_H_
#define TEST_LED_API_H_

const struct device *get_led_controller(void);

void test_led_setup(void);
void test_led_get_info(void);
void test_led_on(void);
void test_led_off(void);
void test_led_set_color(void);
void test_led_set_brightness(void);

#endif
