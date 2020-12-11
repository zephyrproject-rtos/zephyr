/*
 * Copyright (c) 2019 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_GPIO_API_H_
#define TEST_GPIO_API_H_

#include <zephyr.h>
#include <drivers/gpio.h>
#include <ztest.h>

#if DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
#define TEST_DEV             DT_GPIO_LABEL(DT_ALIAS(led0), gpios)
#define TEST_PIN             DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#define TEST_PIN_DTS_FLAGS   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios)
#else
#error Unsupported board
#endif

#define TEST_GPIO_MAX_RISE_FALL_TIME_US    200

void test_gpio_pin_configure_push_pull(void);
void test_gpio_pin_configure_single_ended(void);

void test_gpio_pin_set_get_raw(void);
void test_gpio_pin_set_get(void);
void test_gpio_pin_set_get_active_high(void);
void test_gpio_pin_set_get_active_low(void);
void test_gpio_pin_toggle(void);
void test_gpio_pin_toggle_visual(void);

void test_gpio_port_set_masked_get_raw(void);
void test_gpio_port_set_masked_get(void);
void test_gpio_port_set_masked_get_active_high(void);
void test_gpio_port_set_masked_get_active_low(void);
void test_gpio_port_set_bits_clear_bits_raw(void);
void test_gpio_port_set_bits_clear_bits(void);
void test_gpio_port_set_clr_bits_raw(void);
void test_gpio_port_set_clr_bits(void);
void test_gpio_port_toggle(void);

void test_gpio_int_edge_rising(void);
void test_gpio_int_edge_falling(void);
void test_gpio_int_edge_both(void);
void test_gpio_int_edge_to_active(void);
void test_gpio_int_edge_to_inactive(void);
void test_gpio_int_level_high(void);
void test_gpio_int_level_low(void);
void test_gpio_int_level_active(void);
void test_gpio_int_level_inactive(void);

#endif /* TEST_GPIO_API_H_ */
