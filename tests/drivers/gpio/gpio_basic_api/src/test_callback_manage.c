/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_gpio.h"

static struct drv_data cb_data[2];
static int cb_cnt[2];

static void callback_1(const struct device *dev,
		       struct gpio_callback *gpio_cb, uint32_t pins)
{
	TC_PRINT("%s triggered: %d\n", __func__, ++cb_cnt[0]);

}

static void callback_2(const struct device *dev,
		       struct gpio_callback *gpio_cb, uint32_t pins)
{
	TC_PRINT("%s triggered: %d\n", __func__, ++cb_cnt[1]);
}

static void callback_remove_self(const struct device *dev,
				 struct gpio_callback *gpio_cb, uint32_t pins)
{
	struct drv_data *dd = CONTAINER_OF(gpio_cb, struct drv_data, gpio_cb);

	TC_PRINT("%s triggered: %d\n", __func__, ++cb_cnt[1]);
	dd->aux = gpio_remove_callback(dev, gpio_cb);
}

static int init_callback(const struct device *dev_in, const struct device *dev_out,
			 gpio_callback_handler_t handler_1, gpio_callback_handler_t handler_2)
{
	int rc = gpio_pin_interrupt_configure(dev_in, PIN_IN, GPIO_INT_DISABLE);

	if (rc == 0) {
		rc = gpio_pin_interrupt_configure(dev_out, PIN_OUT, GPIO_INT_DISABLE);
	}
	if (rc == 0) {
		/* 1. set PIN_OUT */
		rc = gpio_pin_configure(dev_out, PIN_OUT, (GPIO_OUTPUT_LOW | PIN_OUT_FLAGS));
	}

	if (rc == 0) {
		/* 2. configure PIN_IN callback, but don't enable */
		rc = gpio_pin_configure(dev_in, PIN_IN, (GPIO_INPUT | PIN_IN_FLAGS));
	}

	if (rc == 0) {
		gpio_init_callback(&cb_data[0].gpio_cb, handler_1, BIT(PIN_IN));
		rc = gpio_add_callback(dev_in, &cb_data[0].gpio_cb);
	}

	if (rc == 0) {
		gpio_init_callback(&cb_data[1].gpio_cb, handler_2, BIT(PIN_IN));
		rc = gpio_add_callback(dev_in, &cb_data[1].gpio_cb);
	}

	return rc;
}

static void trigger_callback(const struct device *dev_in, const struct device *dev_out,
			     int enable_cb)
{
	int rc;

	gpio_pin_set(dev_out, PIN_OUT, 0);
	k_sleep(K_MSEC(100));

	cb_cnt[0] = 0;
	cb_cnt[1] = 0;
	if (enable_cb == 1) {
		rc = gpio_pin_interrupt_configure(dev_in, PIN_IN, GPIO_INT_EDGE_RISING);
	} else {
		rc = gpio_pin_interrupt_configure(dev_in, PIN_IN, GPIO_INT_DISABLE);
	}
	zassert_equal(rc, 0);
	k_sleep(K_MSEC(100));
	gpio_pin_set(dev_out, PIN_OUT, 1);
	k_sleep(K_MSEC(1000));
}

static int test_callback_add_remove(void)
{
	const struct device *const dev_in = DEVICE_DT_GET(DEV_IN);
	const struct device *const dev_out = DEVICE_DT_GET(DEV_OUT);

	/* SetUp: initialize environment */
	int rc = init_callback(dev_in, dev_out, callback_1, callback_2);

	if (rc == -ENOTSUP) {
		TC_PRINT("%s not supported\n", __func__);
		return TC_PASS;
	}
	zassert_equal(rc, 0,
		      "init_callback failed");

	/* 3. enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	trigger_callback(dev_in, dev_out, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 4. remove callback_1 */
	gpio_remove_callback(dev_in, &cb_data[0].gpio_cb);
	trigger_callback(dev_in, dev_out, 1);

	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 0 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 5. remove callback_2 */
	gpio_remove_callback(dev_in, &cb_data[1].gpio_cb);
	trigger_callback(dev_in, dev_out, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 0 || cb_cnt[1] != 0) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}
	return TC_PASS;

err_exit:
	gpio_remove_callback(dev_in, &cb_data[0].gpio_cb);
	gpio_remove_callback(dev_in, &cb_data[1].gpio_cb);
	return TC_FAIL;
}

static int test_callback_self_remove(void)
{
	int res = TC_FAIL;
	const struct device *const dev_in = DEVICE_DT_GET(DEV_IN);
	const struct device *const dev_out = DEVICE_DT_GET(DEV_OUT);

	/* SetUp: initialize environment */
	int rc = init_callback(dev_in, dev_out, callback_1, callback_remove_self);

	if (rc == -ENOTSUP) {
		TC_PRINT("%s not supported\n", __func__);
		return TC_PASS;
	}
	zassert_equal(rc, 0,
		      "init_callback failed");

	gpio_pin_set(dev_out, PIN_OUT, 0);
	k_sleep(K_MSEC(100));

	cb_data[0].aux = INT_MAX;
	cb_data[1].aux = INT_MAX;

	/* 3. enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	trigger_callback(dev_in, dev_out, 1);

	/*= checkpoint: check both callbacks are triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/*= checkpoint: check remove executed is invoked =*/
	if (cb_data[0].aux != INT_MAX || cb_data[1].aux != 0) {
		TC_ERROR("not remove callback correctly\n");
		goto err_exit;
	}

	/* 4 enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	trigger_callback(dev_in, dev_out, 1);

	/*= checkpoint: check only remaining callback is triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 0) {
		TC_ERROR("not trigger remaining callback correctly\n");
		goto err_exit;
	}

	res = TC_PASS;

err_exit:
	gpio_remove_callback(dev_in, &cb_data[0].gpio_cb);
	gpio_remove_callback(dev_in, &cb_data[1].gpio_cb);
	return res;
}

static int test_callback_enable_disable(void)
{
	const struct device *const dev_in = DEVICE_DT_GET(DEV_IN);
	const struct device *const dev_out = DEVICE_DT_GET(DEV_OUT);

	/* SetUp: initialize environment */
	int rc = init_callback(dev_in, dev_out, callback_1, callback_2);

	if (rc == -ENOTSUP) {
		TC_PRINT("%s not supported\n", __func__);
		return TC_PASS;
	}
	zassert_equal(rc, 0,
		      "init_callback failed");

	/* 3. enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	trigger_callback(dev_in, dev_out, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 4. disable callback */
	trigger_callback(dev_in, dev_out, 0);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 0 || cb_cnt[1] != 0) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}

	/* 5. enable callback again */
	trigger_callback(dev_in, dev_out, 1);
	/*= checkpoint: check callback is triggered =*/
	if (cb_cnt[0] != 1 || cb_cnt[1] != 1) {
		TC_ERROR("not trigger callback correctly\n");
		goto err_exit;
	}
	gpio_remove_callback(dev_in, &cb_data[0].gpio_cb);
	gpio_remove_callback(dev_in, &cb_data[1].gpio_cb);
	return TC_PASS;

err_exit:
	gpio_remove_callback(dev_in, &cb_data[0].gpio_cb);
	gpio_remove_callback(dev_in, &cb_data[1].gpio_cb);
	return TC_FAIL;
}

ZTEST(gpio_port_cb_mgmt, test_gpio_callback_add_remove)
{
	zassert_equal(test_callback_add_remove(), TC_PASS);
}

ZTEST(gpio_port_cb_mgmt, test_gpio_callback_self_remove)
{
	zassert_equal(test_callback_self_remove(), TC_PASS);
}

ZTEST(gpio_port_cb_mgmt, test_gpio_callback_enable_disable)
{
	zassert_equal(test_callback_enable_disable(), TC_PASS);
}
