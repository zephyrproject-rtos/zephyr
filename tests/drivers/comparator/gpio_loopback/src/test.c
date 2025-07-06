/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *test_dev = DEVICE_DT_GET(DT_ALIAS(test_comp));
static const struct gpio_dt_spec test_pin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);
static K_SEM_DEFINE(test_sem, 0, 1);

static void test_callback(const struct device *dev, void *user_data)
{
	zassert_equal(&test_sem, user_data);
	k_sem_give(&test_sem);
}

static void *test_setup(void)
{
	zassert_ok(gpio_pin_configure_dt(&test_pin, GPIO_OUTPUT_INACTIVE));
	return NULL;
}

static void test_before(void *f)
{
	ARG_UNUSED(f);

	k_sem_reset(&test_sem);
	zassert_ok(gpio_pin_set_dt(&test_pin, 0));
	zassert_ok(comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_NONE));
	zassert_ok(comparator_set_trigger_callback(test_dev, NULL, NULL));
	zassert_between_inclusive(comparator_trigger_is_pending(test_dev), 0, 1);
}

ZTEST_SUITE(comparator_gpio_loopback, NULL, test_setup, test_before, NULL, NULL);

ZTEST(comparator_gpio_loopback, test_get_output)
{
	zassert_equal(comparator_get_output(test_dev), 0);
	k_msleep(1);
	zassert_ok(gpio_pin_set_dt(&test_pin, 1));
	k_msleep(1);
	zassert_equal(comparator_get_output(test_dev), 1);
	k_msleep(1);
	zassert_ok(gpio_pin_set_dt(&test_pin, 0));
	k_msleep(1);
	zassert_equal(comparator_get_output(test_dev), 0);
}

ZTEST(comparator_gpio_loopback, test_no_trigger_no_pending)
{
	zassert_ok(gpio_pin_set_dt(&test_pin, 1));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
	zassert_ok(gpio_pin_set_dt(&test_pin, 0));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
}

ZTEST(comparator_gpio_loopback, test_trigger_rising_edge_pending)
{
	zassert_ok(comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_RISING_EDGE));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
	zassert_ok(gpio_pin_set_dt(&test_pin, 1));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
	zassert_ok(gpio_pin_set_dt(&test_pin, 0));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
}

ZTEST(comparator_gpio_loopback, test_trigger_falling_edge_pending)
{
	zassert_ok(comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_FALLING_EDGE));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
	zassert_ok(gpio_pin_set_dt(&test_pin, 1));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
	zassert_ok(gpio_pin_set_dt(&test_pin, 0));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
}

ZTEST(comparator_gpio_loopback, test_trigger_both_edges_pending)
{
	zassert_ok(comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_BOTH_EDGES));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
	zassert_ok(gpio_pin_set_dt(&test_pin, 1));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
	zassert_ok(gpio_pin_set_dt(&test_pin, 0));
	k_msleep(1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 1);
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
}

ZTEST(comparator_gpio_loopback, test_trigger_callback)
{
	zassert_ok(comparator_set_trigger_callback(test_dev, test_callback, &test_sem));
	k_msleep(1);
	zassert_equal(k_sem_take(&test_sem, K_NO_WAIT), -EBUSY);
	zassert_ok(comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_RISING_EDGE));
	k_msleep(1);
	zassert_equal(k_sem_take(&test_sem, K_NO_WAIT), -EBUSY);
	zassert_ok(gpio_pin_set_dt(&test_pin, 1));
	k_msleep(1);
	zassert_ok(k_sem_take(&test_sem, K_NO_WAIT));
	zassert_equal(comparator_trigger_is_pending(test_dev), 0);
}
