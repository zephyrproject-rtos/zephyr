/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* test_data/gpio_in.csv is driving this port and pin */
#define GPIO_CTRL_NODE  DT_NODELABEL(gpio0)
#define TEST_PIN        0
#define EXPECTED_EVENTS 3

static const struct device *const gpio_dev = DEVICE_DT_GET(GPIO_CTRL_NODE);
static struct gpio_callback gpio_cb;
static struct k_sem irq_sem;
static volatile uint32_t irq_count;

/* Values from test_data/gpio_in.csv */
static const uint8_t expected_levels[EXPECTED_EVENTS] = {1, 0, 1};

static void gpio_irq_handler(const struct device *port, struct gpio_callback *cb,
			     gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	irq_count++;
	k_sem_give(&irq_sem);
}

ZTEST(file_backend, test_gpio_file_backend)
{
	int err;

	k_sem_init(&irq_sem, 0, K_SEM_MAX_LIMIT);
	irq_count = 0U;

	zassert_true(device_is_ready(gpio_dev), "GPIO device not ready");

	err = gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT);
	zassert_equal(err, 0, "gpio_pin_configure failed: %d", err);

	gpio_init_callback(&gpio_cb, gpio_irq_handler, BIT(TEST_PIN));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	zassert_equal(err, 0, "gpio_add_callback failed: %d", err);

	err = gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_EDGE_BOTH);
	zassert_equal(err, 0, "gpio_pin_interrupt_configure failed: %d", err);

	for (int i = 0; i < EXPECTED_EVENTS; i++) {
		err = k_sem_take(&irq_sem, K_SECONDS(1));
		zassert_equal(err, 0, "Timed out waiting for GPIO irq %d", i);

		const int level = gpio_pin_get(gpio_dev, TEST_PIN);

		zassert_true(level >= 0, "gpio_pin_get failed: %d", level);
		zassert_equal(level, expected_levels[i],
			      "Unexpected GPIO level after irq %d: got %d expected %u", i, level,
			      expected_levels[i]);
	}

	err = k_sem_take(&irq_sem, K_MSEC(200));
	zassert_equal(err, -EAGAIN, "Unexpected extra GPIO irq, total count=%u", irq_count);

	err = gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_DISABLE);
	zassert_equal(err, 0, "gpio_pin_interrupt_configure(disable) failed: %d", err);

	err = gpio_remove_callback(gpio_dev, &gpio_cb);
	zassert_equal(err, 0, "gpio_remove_callback failed: %d", err);
}

ZTEST_SUITE(file_backend, NULL, NULL, NULL, NULL, NULL);
