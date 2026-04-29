/*
 * Copyright (c) 2026 BeagleBoard.org
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

const struct gpio_dt_spec output_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, output_gpios);
const struct gpio_dt_spec input_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, input_gpios);

ZTEST_SUITE(gpio_raw_regs, NULL, NULL, NULL, NULL, NULL);

ZTEST(gpio_raw_regs, test_input_reg)
{
	int ret;
	bool pin_active;
	struct gpio_raw_regs regs;

	ret = gpio_pin_configure_dt(&input_gpio, GPIO_INPUT);
	zassert(ret >= 0, "Failed to configure GPIO pin as input");

	ret = gpio_port_get_raw_regs(input_gpio.port, &regs);
	zassert_equal(ret, 0, "Failed to get RAW Regs");
	zassert_not_equal(regs.in, 0, "Input register is not present");

	ret = gpio_emul_input_set_dt(&input_gpio, 1);
	zassert_equal(ret, 0, "Failed to set input");

	pin_active = !!sys_test_bit(regs.in, input_gpio.pin);
	zassert_true(pin_active, "Should be at high level");

	ret = gpio_emul_input_set_dt(&input_gpio, 0);
	zassert_equal(ret, 0, "Failed to set input");

	pin_active = !!sys_test_bit(regs.in, input_gpio.pin);
	zassert_false(pin_active, "Should be at low level");
}

ZTEST(gpio_raw_regs, test_out_reg)
{
	int ret;
	struct gpio_raw_regs regs;

	ret = gpio_pin_configure_dt(&output_gpio, GPIO_OUTPUT);
	zassert(ret >= 0, "Failed to configure GPIO pin as output");

	ret = gpio_port_get_raw_regs(output_gpio.port, &regs);
	zassert_equal(ret, 0, "Failed to get RAW Regs");
	zassert_not_equal(regs.out, 0, "Output register is not present");

	sys_set_bit(regs.out, output_gpio.pin);
	ret = gpio_emul_output_get_dt(&output_gpio);
	zassert_true(ret, "Pin should be high");

	sys_clear_bit(regs.out, output_gpio.pin);
	ret = gpio_emul_output_get_dt(&output_gpio);
	zassert_false(ret, "Pin should be low");

	sys_write32(BIT(output_gpio.pin), regs.out);
	ret = gpio_emul_output_get_dt(&output_gpio);
	zassert_true(ret, "Pin should be high");
}
