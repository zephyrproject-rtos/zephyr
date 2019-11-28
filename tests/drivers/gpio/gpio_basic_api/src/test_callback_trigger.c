/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_gpio_basic_api
 * @{
 * @defgroup t_gpio_callback_trigger test_gpio_callback_trigger
 * @brief TestPurpose: verify zephyr gpio callback triggered
 * under different INT modes
 * @}
 */

#include "test_gpio.h"

static struct drv_data data;
static int cb_cnt;

static int pin_num(u32_t pins)
{
	int ret = 0;

	while (pins >>= 1) {
		ret++;
	}
	return ret;
}

static void callback(struct device *dev,
		     struct gpio_callback *gpio_cb, u32_t pins)
{
	/*= checkpoint: pins should be marked with correct pin number bit =*/
	zassert_true(pin_num(pins) == PIN_IN, NULL);
	TC_PRINT("callback triggered: %d\n", ++cb_cnt);
	if (cb_cnt >= MAX_INT_CNT) {
		struct drv_data *drv_data = CONTAINER_OF(gpio_cb,
							 struct drv_data, gpio_cb);
		gpio_pin_write(dev, PIN_OUT,
			       (drv_data->mode & GPIO_INT_ACTIVE_HIGH) ? 0 : 1);
		gpio_pin_configure(dev, PIN_IN, GPIO_DIR_IN);
	}
}

static int test_callback(int mode)
{
	struct device *dev = device_get_binding(DEV_NAME);
	struct drv_data *drv_data = &data;

	gpio_pin_disable_callback(dev, PIN_IN);
	gpio_pin_disable_callback(dev, PIN_OUT);

	/* 1. set PIN_OUT to initial state */
	if (gpio_pin_configure(dev, PIN_OUT, GPIO_DIR_OUT) != 0) {
		TC_ERROR("PIN_OUT config fail\n");
		return TC_FAIL;
	}
	if (gpio_pin_write(dev, PIN_OUT,
			   (mode & GPIO_INT_ACTIVE_HIGH) ? 0 : 1) != 0) {
		TC_ERROR("set PIN_OUT init voltage fail\n");
		return TC_FAIL;
	}

	/* 2. configure PIN_IN callback and trigger condition */
	if (gpio_pin_configure(dev, PIN_IN,
			       GPIO_DIR_IN | GPIO_INT | mode | GPIO_INT_DEBOUNCE) != 0) {
		TC_ERROR("config PIN_IN fail");
		goto err_exit;
	}

	drv_data->mode = mode;
	gpio_init_callback(&drv_data->gpio_cb, callback, BIT(PIN_IN));
	if (gpio_add_callback(dev, &drv_data->gpio_cb) != 0) {
		TC_ERROR("set PIN_IN callback fail\n");
		return TC_FAIL;
	}

	/* 3. enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	cb_cnt = 0;
	gpio_pin_enable_callback(dev, PIN_IN);
	k_sleep(K_MSEC(100));
	gpio_pin_write(dev, PIN_OUT, (mode & GPIO_INT_ACTIVE_HIGH) ? 1 : 0);
	k_sleep(K_MSEC(1000));

	/*= checkpoint: check callback is triggered =*/
	TC_PRINT("check enabled callback\n");
	if ((mode & GPIO_INT_EDGE) == GPIO_INT_EDGE) {
		if (cb_cnt != 1) {
			TC_ERROR("not trigger callback correctly\n");
			goto err_exit;
		}
		goto pass_exit;
	}

	if ((mode & GPIO_INT_EDGE) == GPIO_INT_LEVEL) {
		if (cb_cnt != MAX_INT_CNT) {
			TC_ERROR("not trigger callback correctly\n");
			goto err_exit;
		}
	}

pass_exit:
	gpio_remove_callback(dev, &drv_data->gpio_cb);
	return TC_PASS;

err_exit:
	gpio_remove_callback(dev, &drv_data->gpio_cb);
	return TC_FAIL;
}

/* export test cases */
void test_gpio_callback_edge_high(void)
{
	zassert_true(
		test_callback(GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH) == TC_PASS,
		NULL);
}

void test_gpio_callback_edge_low(void)
{
	zassert_true(
		test_callback(GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW) == TC_PASS,
		NULL);
}

void test_gpio_callback_level_high(void)
{
	zassert_true(
		test_callback(GPIO_INT_LEVEL | GPIO_INT_ACTIVE_HIGH) == TC_PASS,
		NULL);
}

void test_gpio_callback_level_low(void)
{
	zassert_true(
		test_callback(GPIO_INT_LEVEL | GPIO_INT_ACTIVE_LOW) == TC_PASS,
		NULL);
}
