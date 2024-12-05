/*
 * Copyright (c) 2019 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_GPIO_API_H_
#define TEST_GPIO_API_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

/* If possible, use a dedicated GPIO with an external pulldown resistor.
 * Otherwise, fallback to repurposing led0 as a GPIO. The latter won't always
 * work as expected when reconfigured as an input.
 */
#if DT_NODE_HAS_STATUS(DT_INST(0, test_gpio_external_pulldown), okay)
#define TEST_NODE          DT_GPIO_CTLR(DT_INST(0, test_gpio_external_pulldown), gpios)
#define TEST_PIN           DT_GPIO_PIN(DT_INST(0, test_gpio_external_pulldown), gpios)
#define TEST_PIN_DTS_FLAGS DT_GPIO_FLAGS(DT_INST(0, test_gpio_external_pulldown), gpios)
#elif DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
#define TEST_NODE          DT_GPIO_CTLR(DT_ALIAS(led0), gpios)
#define TEST_PIN           DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#define TEST_PIN_DTS_FLAGS DT_GPIO_FLAGS(DT_ALIAS(led0), gpios)
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
void test_gpio_int_level_high_interrupt_count_1(void);
void test_gpio_int_level_high_interrupt_count_5(void);
void test_gpio_int_level_low_interrupt_count_1(void);
void test_gpio_int_level_low_interrupt_count_5(void);
void test_gpio_int_level_active(void);
void test_gpio_int_level_inactive(void);

#endif /* TEST_GPIO_API_H_ */
