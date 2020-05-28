/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Avoid CI warnings about use of deprecated API.  The purpose of this
 * module is to test that the deprecated API still works.
 */
#undef __deprecated
#define __deprecated
#undef __DEPRECATED_MACRO
#define __DEPRECATED_MACRO

#include "test_gpio.h"

static struct drv_data data;
static int cb_cnt;

static void callback(struct device *dev,
		     struct gpio_callback *gpio_cb, u32_t pins)
{
	int rc;
	const struct drv_data *dd = CONTAINER_OF(gpio_cb,
						 struct drv_data, gpio_cb);

	/*= checkpoint: pins should be marked with correct pin number bit =*/
	zassert_equal(pins, BIT(PIN_IN),
		      "unexpected pins %x", pins);
	++cb_cnt;
	TC_PRINT("callback triggered: %d\n", cb_cnt);
	if ((cb_cnt == 1)
	    && (dd->mode == GPIO_INT_DOUBLE_EDGE)) {
		gpio_pin_write(dev, PIN_OUT, dd->aux);
	}
	if (cb_cnt >= MAX_INT_CNT) {
		gpio_pin_write(dev, PIN_OUT, dd->aux);

		/* NB: The legacy idiom for disabling interrupts is to
		 * pass GPIO_DIR_IN without any interrupt-related
		 * flags.  In the new API this leaves the interrupt
		 * configuration of the pin unchanged, which causes
		 * level interrupts to repeat forever.  To prevent hangs
		 * it's necessary to explicitly disable the interrupt.
		 */
		rc = gpio_pin_configure(dev, PIN_IN, GPIO_DIR_IN
					| GPIO_INT_DISABLE);
		zassert_equal(rc, 0,
			      "disable interrupts failed: %d", rc);
	}
}

static int test_callback(gpio_flags_t int_flags)
{
	struct device *dev = device_get_binding(DEV_NAME);
	struct drv_data *drv_data = &data;
	bool active_level = (int_flags & GPIO_INT_ACTIVE_HIGH) != 0;
	int rc;

	gpio_pin_disable_callback(dev, PIN_IN);
	gpio_pin_disable_callback(dev, PIN_OUT);

	/* 1. set PIN_OUT to non-active state */
	drv_data->aux = (active_level == false);

	rc = gpio_pin_configure(dev, PIN_OUT, GPIO_DIR_OUT);
	if (rc == 0) {
		gpio_pin_write(dev, PIN_OUT, !active_level);
	}

	if (rc != 0) {
		TC_ERROR("PIN_OUT config fail: %d", rc);
		return TC_FAIL;
	}

	/* 2. configure PIN_IN callback and trigger condition */
	rc = gpio_pin_configure(dev, PIN_IN,
				GPIO_DIR_IN | GPIO_INT
				| GPIO_INT_DEBOUNCE
				| int_flags);
	if (rc == -ENOTSUP) {
		TC_PRINT("interrupt configuration not supported\n");
		return TC_PASS;
	} else if (rc != 0) {
		TC_ERROR("config PIN_IN fail: %d\n", rc);
		return TC_FAIL;
	}

	drv_data->mode = int_flags;
	gpio_init_callback(&drv_data->gpio_cb, callback, BIT(PIN_IN));
	rc = gpio_add_callback(dev, &drv_data->gpio_cb);
	if (rc == -ENOTSUP) {
		TC_PRINT("interrupts not supported\n");
		return TC_PASS;
	} else if (rc != 0) {
		TC_ERROR("set PIN_IN callback fail: %d\n", rc);
		return TC_FAIL;
	}

	/* 3. enable callback, trigger PIN_IN interrupt by operate PIN_OUT */
	cb_cnt = 0;
	rc = gpio_pin_enable_callback(dev, PIN_IN);
	if (rc == -ENOTSUP) {
		TC_PRINT("Mode %x not supported\n", int_flags);
		goto pass_exit;
	} else if (rc != 0) {
		TC_ERROR("enable PIN_IN interrupt fail: %d\n", rc);
		goto err_exit;
	}
	k_sleep(K_MSEC(100));
	gpio_pin_write(dev, PIN_OUT, active_level);
	k_sleep(K_MSEC(1000));
	(void)gpio_pin_disable_callback(dev, PIN_IN);
	(void)gpio_remove_callback(dev, &drv_data->gpio_cb);
	(void)gpio_pin_configure(dev, PIN_IN, GPIO_INT_DISABLE);

	/*= checkpoint: check callback is triggered =*/
	TC_PRINT("INT cfg %x, cnt %d\n", int_flags, cb_cnt);
	if (int_flags == GPIO_INT_DOUBLE_EDGE) {
		if (cb_cnt != 2) {
			TC_ERROR("double edge not detected\n");
			goto err_exit;
		}
		goto pass_exit;
	}
	if ((int_flags & GPIO_INT_EDGE) == GPIO_INT_EDGE) {
		if (cb_cnt != 1) {
			TC_ERROR("edge not trigger callback correctly\n");
			goto err_exit;
		}
		goto pass_exit;
	} else {
		if (cb_cnt != MAX_INT_CNT) {
			TC_ERROR("level not trigger callback correctly\n");
			goto err_exit;
		}
	}

pass_exit:
	return TC_PASS;

err_exit:
	return TC_FAIL;
}

static int test_callback_enable_disable(void)
{
	return TC_PASS;
}

/* export test cases */
void test_gpio_deprecated(void)
{
	zassert_equal(test_callback_enable_disable(),
		      TC_PASS, "callback enable/disable failed");
	zassert_equal(test_callback(GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH),
		      TC_PASS, "rising edge failed");
	zassert_equal(test_callback(GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW),
		      TC_PASS, "falling edge failed");
	zassert_equal(test_callback(GPIO_INT_EDGE_BOTH),
		      TC_PASS, "double edge failed");
	zassert_equal(test_callback(GPIO_INT_LEVEL | GPIO_INT_ACTIVE_HIGH),
		      TC_PASS, "level high failed");
	zassert_equal(test_callback(GPIO_INT_LEVEL | GPIO_INT_ACTIVE_LOW),
		      TC_PASS, "level low failed");
}
