/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_gpio.h"

static struct drv_data data;
static int cb_cnt;

static void callback(const struct device *dev,
		     struct gpio_callback *gpio_cb, uint32_t pins)
{
	/*= checkpoint: pins should be marked with correct pin number bit =*/
	zassert_equal(pins, BIT(PIN_IN),
		      "unexpected pins %x", pins);

	++cb_cnt;
}

ZTEST(after_flash_gpio_config_trigger, test_gpio_config_twice_trigger)
{
	const struct device *const dev = DEVICE_DT_GET(DEV);
	struct drv_data *drv_data = &data;
	int ret;

	cb_cnt = 0;

	ret = gpio_pin_configure(dev, PIN_OUT, GPIO_DISCONNECTED);

	/* 1. Configure PIN_IN callback */
	ret = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT);
	zassert_ok(ret, "config PIN_IN failed");

	gpio_init_callback(&drv_data->gpio_cb, callback, BIT(PIN_IN));
	ret = gpio_add_callback(dev, &drv_data->gpio_cb);
	zassert_ok(ret, "add callback failed");

	/* 2. Enable PIN callback as both edges */
	ret = gpio_pin_interrupt_configure(dev, PIN_IN, GPIO_INT_EDGE_BOTH);
	zassert_ok(ret, "enable callback failed");

	/* 3. Configure PIN_OUT as open drain, internal pull-up (may trigger
	 * callback)
	 */
	ret = gpio_pin_configure(dev, PIN_OUT,
				 GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	if (ret == -ENOTSUP) {
		TC_PRINT("Open drain not supported.\n");
		gpio_remove_callback(dev, &drv_data->gpio_cb);
		ztest_test_skip();
		return;
	}
	zassert_ok(ret, "config PIN_OUT failed");

	/* 4. Configure PIN_OUT again (should not trigger callback)  */
	ret = gpio_pin_configure(dev, PIN_OUT,
				 GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	zassert_ok(ret, "config PIN_OUT twice failed");

	/* 5. Wait a bit and ensure that interrupt happened at most once */
	k_sleep(K_MSEC(10));
	zassert_between_inclusive(cb_cnt, 0, 1, "Got %d interrupts", cb_cnt);

	gpio_remove_callback(dev, &drv_data->gpio_cb);
}

ZTEST(after_flash_gpio_config_trigger, test_gpio_config_trigger)
{
	const struct device *const dev = DEVICE_DT_GET(DEV);
	struct drv_data *drv_data = &data;
	int ret;

	cb_cnt = 0;

	ret = gpio_pin_configure(dev, PIN_OUT, GPIO_DISCONNECTED);

	/* 1. Configure PIN_IN callback */
	ret = gpio_pin_configure(dev, PIN_IN, GPIO_INPUT);
	zassert_ok(ret, "config PIN_IN failed");

	gpio_init_callback(&drv_data->gpio_cb, callback, BIT(PIN_IN));
	ret = gpio_add_callback(dev, &drv_data->gpio_cb);
	zassert_ok(ret, "add callback failed");

	/* 2. Enable PIN callback as both edges */
	ret = gpio_pin_interrupt_configure(dev, PIN_IN, GPIO_INT_EDGE_BOTH);
	zassert_ok(ret, "enable callback failed");

	/* 3. Configure PIN_OUT as open drain, internal pull-up (may trigger
	 * callback)
	 */
	ret = gpio_pin_configure(dev, PIN_OUT,
				 GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	if (ret == -ENOTSUP) {
		TC_PRINT("Open drain not supported.\n");
		gpio_remove_callback(dev, &drv_data->gpio_cb);
		ztest_test_skip();
		return;
	}
	zassert_ok(ret, "config PIN_OUT failed");

	/* 4. Wait a bit and ensure that interrupt happened at most once */
	k_sleep(K_MSEC(10));
	zassert_between_inclusive(cb_cnt, 0, 1, "Got %d interrupts", cb_cnt);

	gpio_remove_callback(dev, &drv_data->gpio_cb);
}
