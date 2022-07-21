/*
 * Copyright (c) 2019 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_gpio_api.h"

void test_main(void)
{
	ztest_test_suite(gpio_api_1pin_test,
		ztest_unit_test(test_gpio_pin_configure_push_pull),
		ztest_unit_test(test_gpio_pin_configure_single_ended),
		ztest_unit_test(test_gpio_pin_set_get_raw),
		ztest_unit_test(test_gpio_pin_set_get),
		ztest_unit_test(test_gpio_pin_set_get_active_high),
		ztest_unit_test(test_gpio_pin_set_get_active_low),
		ztest_unit_test(test_gpio_pin_toggle),
		ztest_unit_test(test_gpio_port_set_masked_get_raw),
		ztest_unit_test(test_gpio_port_set_masked_get),
		ztest_unit_test(test_gpio_port_set_masked_get_active_high),
		ztest_unit_test(test_gpio_port_set_masked_get_active_low),
		ztest_unit_test(test_gpio_port_set_bits_clear_bits_raw),
		ztest_unit_test(test_gpio_port_set_bits_clear_bits),
		ztest_unit_test(test_gpio_port_set_clr_bits_raw),
		ztest_unit_test(test_gpio_port_set_clr_bits),
		ztest_unit_test(test_gpio_port_toggle),
		ztest_unit_test(test_gpio_int_edge_rising),
		ztest_unit_test(test_gpio_int_edge_falling),
		ztest_unit_test(test_gpio_int_edge_both),
		ztest_unit_test(test_gpio_int_edge_to_active),
		ztest_unit_test(test_gpio_int_edge_to_inactive),
		ztest_unit_test(test_gpio_int_level_high_interrupt_count_1),
		ztest_unit_test(test_gpio_int_level_high_interrupt_count_5),
		ztest_unit_test(test_gpio_int_level_low_interrupt_count_1),
		ztest_unit_test(test_gpio_int_level_low_interrupt_count_5),
		ztest_unit_test(test_gpio_int_level_active),
		ztest_unit_test(test_gpio_int_level_inactive),
		ztest_unit_test(test_gpio_pin_toggle_visual));
	ztest_run_test_suite(gpio_api_1pin_test);
}
