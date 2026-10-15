/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <bstests.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <babblekit/testcase.h>
#include "NRF_GPIO.h"
#include "NRF_GPIO_backend.h"

/* This is the port driven by this test's input files */
#define GPIO_CTRL_NODE  DT_NODELABEL(gpio0)

static const struct device *const gpio_dev = DEVICE_DT_GET(GPIO_CTRL_NODE);
static struct gpio_callback gpio_cb;
static struct k_sem irq_sem;
static volatile uint32_t irq_count;

static void gpio_irq_handler(const struct device *port, struct gpio_callback *cb,
			     gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	irq_count++;
	k_sem_give(&irq_sem);
}

static void gpio_file_backend_test(void)
{
	const struct {
		uint8_t pin;
		uint8_t level;
	} expected_levels[] = {
	/* Note this matches both gpio_in_all.csv, and the combination of all test_data/gpio_in.*
	 * which are tested separately
	 */
		{2, 1},
		{4, 1},
		{3, 1},
		{0, 1},
		{0, 0},
		{2, 0},
		{4, 0},
		{3, 0},
		{0, 1},
		{4, 1},
		{0, 0},
		{3, 1},
	};
	const uint8_t used_test_pins[] = {0, 2, 3, 4};

	int err, i;
	gpio_port_pins_t pin_mask = 0;

	k_sem_init(&irq_sem, 0, K_SEM_MAX_LIMIT);
	irq_count = 0U;

	TEST_ASSERT(device_is_ready(gpio_dev), "GPIO device not ready");

	for (i = 0; i < ARRAY_SIZE(used_test_pins); i++) {
		err = gpio_pin_configure(gpio_dev, used_test_pins[i], GPIO_INPUT);
		TEST_ASSERT(err == 0, "gpio_pin_configure failed: %d", err);

		pin_mask |= BIT(used_test_pins[i]);

		err = gpio_pin_interrupt_configure(gpio_dev, used_test_pins[i], GPIO_INT_EDGE_BOTH);
		TEST_ASSERT(err == 0, "gpio_pin_interrupt_configure failed: %d", err);
	}

	gpio_init_callback(&gpio_cb, gpio_irq_handler, pin_mask);
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	TEST_ASSERT(err == 0, "gpio_add_callback failed: %d", err);

	for (i = 0; i < ARRAY_SIZE(expected_levels); i++) {
		err = k_sem_take(&irq_sem, K_SECONDS(1));
		TEST_ASSERT(err == 0, "Timed out waiting for GPIO irq %d", i);

		const int level = gpio_pin_get(gpio_dev, expected_levels[i].pin);

		TEST_ASSERT(level >= 0, "gpio_pin_get failed: %d", level);
		TEST_ASSERT(level == expected_levels[i].level,
			      "Unexpected GPIO level after irq %d: got %d expected %u", i, level,
			      expected_levels[i].level);
	}

	err = k_sem_take(&irq_sem, K_MSEC(200));
	TEST_ASSERT(err == -EAGAIN, "Unexpected extra GPIO irq, total count=%u", irq_count);

	for (i = 0; i < ARRAY_SIZE(used_test_pins); i++) {
		err = gpio_pin_interrupt_configure(gpio_dev, used_test_pins[i], GPIO_INT_DISABLE);
		TEST_ASSERT(err == 0, "gpio_pin_interrupt_configure(disable) failed: %d", err);
	}

	err = gpio_remove_callback(gpio_dev, &gpio_cb);
	TEST_ASSERT(err == 0, "gpio_remove_callback failed: %d", err);

	TEST_PASS_AND_EXIT("GPIO file backend test passed");
}

static bool got_callback_on_pin_8;

static void input_change_callback(unsigned int port, unsigned int n, bool value)
{
	if ((port == 0) && (n == 8) && (value == 1)) {
		got_callback_on_pin_8 = true;
	}
}

static void gpio_shorts_test(void)
{
	int err, level;

	TEST_ASSERT(device_is_ready(gpio_dev), "GPIO device not ready");

	/* On the config file, pin 0.5 is shorted to 0.6 and 0.7. Let's test it */

	/* We add another short through the SW API from 0.5 to 0.8 */
	nrf_gpio_backend_register_short(0, 5, 0, 8);

	/* Let's also test the low level callback API */
	nrf_gpio_test_register_in_callback(input_change_callback);

	err = gpio_pin_configure(gpio_dev, 5, GPIO_OUTPUT);
	TEST_ASSERT(err == 0, "gpio_pin_configure failed: %d", err);

	for (int pin = 6; pin <= 8; pin++) {
		err = gpio_pin_configure(gpio_dev, pin, GPIO_INPUT);
		TEST_ASSERT(err == 0, "gpio_pin_configure %i failed: %d", pin, err);
	}

	/* We expect the pins low as by default 5 drives low */
	for (int pin = 6; pin <= 8; pin++) {
		level = gpio_pin_get(gpio_dev, pin);
		TEST_ASSERT(level == 0, "gpio_pin_get %i failed: %d", pin, level);
	}

	err = gpio_pin_set(gpio_dev, 5, 1);
	TEST_ASSERT(err == 0, "gpio_pin_set failed: %d", err);

	/* And after raising 5, we expect them high */
	for (int pin = 6; pin <= 8; pin++) {
		level = gpio_pin_get(gpio_dev, pin);
		TEST_ASSERT(level == 1, "gpio_pin_get %i failed: %d", pin, level);
	}

	/* And we expect the callback to have registered it too*/
	TEST_ASSERT(got_callback_on_pin_8, "GPIO 0.8 should have got up");

	TEST_PASS_AND_EXIT("GPIO shorts test passed");
}

static const struct bst_test_instance gpio_file_backend[] = {
	{
		.test_id = "file_backend",
		.test_descr = "Test GPIO file backend",
		.test_main_f = gpio_file_backend_test
	},
	{
		.test_id = "shorts",
		.test_descr = "Test GPIO shorts",
		.test_main_f = gpio_shorts_test
	},

	BSTEST_END_MARKER
};

struct bst_test_list *test_gpio_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, gpio_file_backend);
}

bst_test_install_t test_installers[] = {
	test_gpio_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
