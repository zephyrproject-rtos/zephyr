/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_gpio_basic_api
 * @{
 * @defgroup t_gpio_callback_manage test_gpio_callback_manage
 * @brief TestPurpose: verify zephyr gpio callback add/remove and enable/disable
 * @}
 */

#include "test_gpio.h"

static struct drv_data cb_data[2];
static int cb_cnt[2];

static void callback_1(struct device *dev,
		       struct gpio_callback *gpio_cb, u32_t pins)
{
	TC_PRINT("%s triggered: %d\n", __func__, ++cb_cnt[0]);

}

static void callback_2(struct device *dev,
		       struct gpio_callback *gpio_cb, u32_t pins)
{
	TC_PRINT("%s triggered: %d\n", __func__, ++cb_cnt[1]);
}

static void init_callback(struct device *dev)
{
	gpio_pin_disable_callback(dev, PIN_IN);
	gpio_pin_disable_callback(dev, PIN_OUT);

	/* 1. set PIN_OUT */
	gpio_pin_configure(dev, PIN_OUT, GPIO_DIR_OUT);
	gpio_pin_write(dev, PIN_OUT, 0);

	/* 2. configure PIN_IN callback and trigger condition */
	gpio_pin_configure(dev, PIN_IN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | \
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&cb_data[0].gpio_cb, callback_1, BIT(PIN_IN));
	gpio_add_callback(dev, &cb_data[0].gpio_cb);

	gpio_init_callback(&cb_data[1].gpio_cb, callback_2, BIT(PIN_IN));
	gpio_add_callback(dev, &cb_data[1].gpio_cb);
}

static void trigger_callback(struct device *dev, int enable_cb)
{
	gpio_pin_write(dev, PIN_OUT, 0);
	k_sleep(100);

	cb_cnt[0] = 0;
	cb_cnt[1] = 0;
	if (enable_cb == 1) {
		gpio_pin_enable_callback(dev, PIN_IN);
	} else {
		gpio_pin_disable_callback(dev, PIN_IN);
	}
	k_sleep(100);
	gpio_pin_write(dev, PIN_OUT, 1);
	k_sleep(1000);
}

static int test_callback_add_remove(void)
{
	struct device *dev = device_get_binding(DEV_NAME);

	/* SetUp: initialize environment */
	init_callback(dev);

	/* 3. enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	trigger_callback(dev, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 4. remove callback_1 */
	gpio_remove_callback(dev, &cb_data[0].gpio_cb);
	trigger_callback(dev, 1);

	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 0 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 5. remove callback_2 */
	gpio_remove_callback(dev, &cb_data[1].gpio_cb);
	trigger_callback(dev, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 0 || cb_cnt[1] != 0) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}
	return TC_PASS;

err_exit:
	gpio_remove_callback(dev, &cb_data[0].gpio_cb);
	gpio_remove_callback(dev, &cb_data[1].gpio_cb);
	return TC_FAIL;
}

static int test_callback_enable_disable(void)
{
	struct device *dev = device_get_binding(DEV_NAME);

	/* SetUp: initialize environment */
	init_callback(dev);

	/* 3. enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	trigger_callback(dev, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 4. disable callback */
	trigger_callback(dev, 0);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 0 || cb_cnt[1] != 0) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 5. enable callback again */
	trigger_callback(dev, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}
	gpio_remove_callback(dev, &cb_data[0].gpio_cb);
	gpio_remove_callback(dev, &cb_data[1].gpio_cb);
	return TC_PASS;

err_exit:
	gpio_remove_callback(dev, &cb_data[0].gpio_cb);
	gpio_remove_callback(dev, &cb_data[1].gpio_cb);
	return TC_FAIL;
}

void test_gpio_callback_add_remove(void)
{
	zassert_true(
		test_callback_add_remove() == TC_PASS, NULL);
}

void test_gpio_callback_enable_disable(void)
{
	zassert_true(
		test_callback_enable_disable() == TC_PASS, NULL);
}
