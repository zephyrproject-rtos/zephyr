/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/pinmux.h>
#include <ztest.h>

#define PINMUX_NAME CONFIG_PINMUX_NAME

#if defined(DT_ALIAS_GPIO_0_LABEL)
#define GPIO_DEV_NAME DT_ALIAS_GPIO_0_LABEL
#elif defined(DT_ALIAS_GPIO_1_LABEL)
#define GPIO_DEV_NAME DT_ALIAS_GPIO_1_LABEL
#elif defined(DT_ALIAS_GPIO_3_LABEL)
#define GPIO_DEV_NAME DT_ALIAS_GPIO_3_LABEL
#else
#error Unsupported board
#endif

#define GPIO_OUT 4
#define GPIO_IN 5
#define PIN_IN 3

#define MAX_INT_CNT 10

static bool cb_triggered;

static void callback(struct device *dev,
		     struct gpio_callback *gpio_cb, u32_t pins)
{
	static int cb_cnt;

	TC_PRINT("callback triggered: %d\n", ++cb_cnt);

	if (cb_cnt >= MAX_INT_CNT) {
		cb_triggered = true;
		gpio_pin_write(dev, GPIO_OUT, 0);
	}
}

static int test_gpio(u32_t pin, u32_t func)
{
	u32_t function;
	struct gpio_callback gpio_cb;
	struct device *pinmux = device_get_binding(PINMUX_NAME);

	if (!pinmux) {
		TC_PRINT("Cannot get PINMUX\n");
		return TC_FAIL;
	}

	struct device *gpio_dev = device_get_binding(GPIO_DEV_NAME);

	if (!gpio_dev) {
		TC_PRINT("Cannot get GPIO device\n");
		return TC_FAIL;
	}

	cb_triggered = false;

	/* 1. Configure PIN_OUT and set init voltage */
	if (gpio_pin_configure(gpio_dev, GPIO_OUT, GPIO_DIR_OUT)) {
		TC_PRINT("PIN_OUT configure fail\n");
		return TC_FAIL;
	}

	if (gpio_pin_write(gpio_dev, GPIO_OUT, 0)) {
		TC_PRINT("Set PIN_OUT init LOW fail\n");
		return TC_FAIL;
	}

	/* 2. Configure PIN_IN and set callback */
	if (gpio_pin_configure(gpio_dev, GPIO_IN,
				GPIO_DIR_IN | GPIO_INT | GPIO_INT_DEBOUNCE |
				GPIO_INT_LEVEL | GPIO_INT_ACTIVE_HIGH)) {
		TC_PRINT("PIN_IN configure fail\n");
		return TC_FAIL;
	}

	gpio_init_callback(&gpio_cb, callback, BIT(GPIO_IN));
	if (gpio_add_callback(gpio_dev, &gpio_cb)) {
		TC_PRINT("Set PIN_IN callback fail\n");
		return TC_FAIL;
	}

	gpio_pin_enable_callback(gpio_dev, GPIO_IN);

	/* 3. Verify pinmux_pin_set() */
	if (pinmux_pin_set(pinmux, pin, func)) {
		TC_PRINT("Fail to set pin func, %u : %u\n", pin, func);
		return TC_FAIL;
	}

	/* 4. Verify pinmux_pin_get() */
	if (pinmux_pin_get(pinmux, pin, &function)) {
		TC_PRINT("Fail to get pin func\n");
		return TC_FAIL;
	}

	/* 5. Verify the setting works */
	if (function != func) {
		TC_PRINT("Error. PINMUX get doesn't match PINMUX set\n");
		return TC_FAIL;
	}

	/* 6. Set PIN_OUT HIGH and verify pin func works */
	if (gpio_pin_write(gpio_dev, GPIO_OUT, 1)) {
		TC_PRINT("Set PIN_OUT HIGH fail\n");
		return TC_FAIL;
	}

	k_sleep(K_MSEC(1000));

	if (cb_triggered) {
		TC_PRINT("GPIO callback is triggered\n");
		return TC_PASS;
	} else {
		TC_PRINT("GPIO callback is not triggered\n");
		return TC_FAIL;
	}
}

void test_pinmux_gpio(void)
{
	zassert_true(test_gpio(PIN_IN, PINMUX_FUNC_A) == TC_PASS, NULL);
	zassert_true(test_gpio(PIN_IN, PINMUX_FUNC_B) == TC_FAIL, NULL);
}
