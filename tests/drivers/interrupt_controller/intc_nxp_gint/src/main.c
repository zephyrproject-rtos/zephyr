/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/interrupt_controller/intc_nxp_gint.h>
#include <zephyr/drivers/gpio.h>

/*
 * GINT module is used to monitor the digital function pin
 * input signal, no matter the pin is configured as GPIO or not.
 *
 * Theoretically, external signal should be connected to the
 * pins, change the signal level, and check GINT function.
 * To make the test easy, this test drives the GPIO pin
 * directly, and use GINT to monitor the GPIO pin. When change
 * the GPIO level, GINT can monitor the change on the same pin.
 * With this method, the externel signal is not necessary.
 */

#define GINT_NODE	DT_NODELABEL(gint0)
#define GPIO_PIN_0_NODE	DT_ALIAS(led0)
#define GPIO_PIN_1_NODE	DT_ALIAS(led1)

#define TEST_GPIO_0	DT_GPIO_CTLR(GPIO_PIN_0_NODE, gpios)
#define TEST_PIN_0	DT_GPIO_PIN(GPIO_PIN_0_NODE, gpios)
#define TEST_PORT_0	DT_REG_ADDR(TEST_GPIO_0)

#define TEST_GPIO_1	DT_GPIO_CTLR(GPIO_PIN_1_NODE, gpios)
#define TEST_PIN_1	DT_GPIO_PIN(GPIO_PIN_1_NODE, gpios)
#define TEST_PORT_1	DT_REG_ADDR(TEST_GPIO_1)

/*
 * For level interrupt testing, we need to disable the pin to prevent
 * continuous triggering in callback, and enable the pin again
 * after exit callback. Define this as a test loop, this macro
 * means how many loops to test.
 */
#define TEST_LEVEL_INT_COUNT 5

static const struct device *gint_dev = DEVICE_DT_GET(GINT_NODE);
static const struct gpio_dt_spec gpio_pin0 = GPIO_DT_SPEC_GET(GPIO_PIN_0_NODE, gpios);
static const struct gpio_dt_spec gpio_pin1 = GPIO_DT_SPEC_GET(GPIO_PIN_1_NODE, gpios);

static atomic_t gint_callback_count;

static void test_gint_edge_callback(const struct device *dev, void *user_data)
{
	atomic_inc(&gint_callback_count);
}

static void test_gint_level_callback(const struct device *dev, void *user_data)
{
	atomic_inc(&gint_callback_count);

	nxp_gint_disable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0);
	nxp_gint_disable_pin(gint_dev, TEST_PORT_1, TEST_PIN_1);
}

static void *test_gint_setup(void)
{
	zassert_true(device_is_ready(gint_dev), "GINT device not ready");
	return NULL;
}

static void test_gint_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Reset callback flags */
	atomic_set(&gint_callback_count, 0);

	/* Clear any pending interrupts */
	nxp_gint_clear_pending(gint_dev);

	/* Disable test pin */
	nxp_gint_disable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0);
}

ZTEST(intc_nxp_gint, test_enable_disable_pin)
{
	int ret;

	/* Enable pin with HIGH polarity */
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0, NXP_GINT_POL_HIGH);
	zassert_ok(ret, "Failed to enable pin %d:%d", TEST_PORT_0, TEST_PIN_0);

	/* Disable pin */
	ret = nxp_gint_disable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0);
	zassert_ok(ret, "Failed to disable pin %d:%d", TEST_PORT_0, TEST_PIN_0);
}

ZTEST(intc_nxp_gint, test_invalid_port)
{
	int ret;

	/* Try to enable pin with invalid port number */
	ret = nxp_gint_enable_pin(gint_dev, 255, TEST_PIN_0, NXP_GINT_POL_HIGH);
	zassert_equal(ret, -EINVAL, "Should fail with invalid port");

	/* Try to disable pin with invalid port number */
	ret = nxp_gint_disable_pin(gint_dev, 255, TEST_PIN_0);
	zassert_equal(ret, -EINVAL, "Should fail with invalid port");
}

ZTEST(intc_nxp_gint, test_invalid_pin)
{
	int ret;

	/* Try to enable pin with invalid pin number */
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_0, 32, NXP_GINT_POL_HIGH);
	zassert_equal(ret, -EINVAL, "Should fail with invalid pin");

	/* Try to disable pin with invalid pin number */
	ret = nxp_gint_disable_pin(gint_dev, TEST_PORT_0, 32);
	zassert_equal(ret, -EINVAL, "Should fail with invalid pin");
}

ZTEST(intc_nxp_gint, test_gint_single_pin_edge)
{
	int ret;
	struct nxp_gint_group_config gint_config = {
		.trigger = NXP_GINT_TRIG_EDGE,
		.combination = NXP_GINT_COMB_OR,
	};

	/* Configure GPIO pin as output */
	ret = gpio_pin_configure_dt(&gpio_pin0, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "Failed to configure GPIO pin as output");

	/* Set pin LOW initially */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 0);
	zassert_ok(ret, "Failed to set pin LOW");

	/* Register callback */
	ret = nxp_gint_register_callback(gint_dev, test_gint_edge_callback, NULL);
	zassert_ok(ret, "Failed to register callback");

	/* Configure GINT for edge trigger */
	ret = nxp_gint_configure_group(gint_dev, &gint_config);
	zassert_ok(ret, "Failed to configure GINT group");

	/* Clear any pending interrupts */
	nxp_gint_clear_pending(gint_dev);

	/* Enable pin with HIGH polarity (rising edge) */
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0, NXP_GINT_POL_HIGH);
	zassert_ok(ret, "Failed to enable pin");

	k_msleep(10);

	/* Reset callback counter */
	atomic_set(&gint_callback_count, 0);

	/* Trigger rising edge: LOW -> HIGH */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 1);
	zassert_ok(ret, "Failed to set pin HIGH");

	/* Wait for interrupt to be processed */
	k_msleep(50);

	/* Verify interrupt was triggered */
	zassert_equal(atomic_get(&gint_callback_count), 1,
		     "GINT callback should be called once");

	/* Cleanup */
	nxp_gint_disable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0);
}

ZTEST(intc_nxp_gint, test_gint_single_pin_level)
{
	int ret;
	struct nxp_gint_group_config gint_config = {
		.trigger = NXP_GINT_TRIG_LEVEL,
		.combination = NXP_GINT_COMB_OR,
	};

	/* Configure GPIO pin as output */
	ret = gpio_pin_configure_dt(&gpio_pin0, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "Failed to configure GPIO pin as output");

	/* Set pin LOW initially */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 0);
	zassert_ok(ret, "Failed to set pin LOW");

	/* Register callback */
	ret = nxp_gint_register_callback(gint_dev, test_gint_level_callback, NULL);
	zassert_ok(ret, "Failed to register callback");

	/* Configure GINT for level trigger */
	ret = nxp_gint_configure_group(gint_dev, &gint_config);
	zassert_ok(ret, "Failed to configure GINT group");

	/* Clear any pending interrupts */
	nxp_gint_clear_pending(gint_dev);

	/* Enable pin with HIGH polarity (high level) */
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0, NXP_GINT_POL_HIGH);
	zassert_ok(ret, "Failed to enable pin");

	k_msleep(10);

	/* Reset callback counter */
	atomic_set(&gint_callback_count, 0);

	/* Set pin HIGH to trigger level interrupt */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 1);
	zassert_ok(ret, "Failed to set pin HIGH");

	for (int i = 1; i < TEST_LEVEL_INT_COUNT; i++) {
		/* Wait for interrupt to be processed */
		k_msleep(50);

		/*
		 * The pin interrupt is disabled in the callback.
		 * Enable it here, then the callback will be called again.
		 */
		nxp_gint_enable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0, NXP_GINT_POL_HIGH);
	}

	/* Verify interrupt was triggered */
	zassert_equal(atomic_get(&gint_callback_count), TEST_LEVEL_INT_COUNT,
		     "GINT callback should be called desired times");

	/* Cleanup */
	nxp_gint_disable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0);
}

ZTEST(intc_nxp_gint, test_gint_multi_pin_or_mode)
{
	int ret;
	struct nxp_gint_group_config gint_config = {
		.trigger = NXP_GINT_TRIG_EDGE,
		.combination = NXP_GINT_COMB_OR,
	};

	/* Configure GPIO pin as output */
	ret = gpio_pin_configure_dt(&gpio_pin0, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "Failed to configure GPIO 0 pin as output");
	ret = gpio_pin_configure_dt(&gpio_pin1, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "Failed to configure GPIO 1 pin as output");

	/* Set both pins HIGH initially */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 1);
	zassert_ok(ret, "Failed to set pin0 HIGH");
	ret = gpio_pin_set_raw(gpio_pin1.port, gpio_pin1.pin, 1);
	zassert_ok(ret, "Failed to set pin1 HIGH");

	/* Register callback */
	ret = nxp_gint_register_callback(gint_dev, test_gint_edge_callback, NULL);
	zassert_ok(ret, "Failed to register callback");

	/* Configure GINT for OR mode */
	ret = nxp_gint_configure_group(gint_dev, &gint_config);
	zassert_ok(ret, "Failed to configure GINT group");

	/* Clear any pending interrupts */
	nxp_gint_clear_pending(gint_dev);

	/* Enable both pins with falling edge */
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0, NXP_GINT_POL_LOW);
	zassert_ok(ret, "Failed to enable pin0");
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_1, TEST_PIN_1, NXP_GINT_POL_LOW);
	zassert_ok(ret, "Failed to enable pin1");

	k_msleep(10);

	/* Reset callback counter */
	atomic_set(&gint_callback_count, 0);

	/* Trigger pin0 - should trigger interrupt in OR mode */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 0);
	zassert_ok(ret, "Failed to set pin0 LOW");

	k_msleep(50);

	zassert_equal(atomic_get(&gint_callback_count), 1,
		      "GINT interrupt should be triggered by pin0 in OR mode");

	/* Reset callback counter */
	atomic_set(&gint_callback_count, 0);
	/* Reset to the initial state for next test */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 1);

	k_msleep(10);

	/* Trigger pin1 - should also trigger interrupt in OR mode */
	ret = gpio_pin_set_raw(gpio_pin1.port, gpio_pin1.pin, 0);
	zassert_ok(ret, "Failed to set pin1 LOW");

	k_msleep(50);

	zassert_equal(atomic_get(&gint_callback_count), 1,
		      "GINT interrupt should be triggered by pin1 in OR mode");

	/* Cleanup */
	nxp_gint_disable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0);
	nxp_gint_disable_pin(gint_dev, TEST_PORT_1, TEST_PIN_1);
}

ZTEST(intc_nxp_gint, test_gint_multi_pin_and_mode)
{
	int ret;
	struct nxp_gint_group_config gint_config = {
		.trigger = NXP_GINT_TRIG_EDGE,
		.combination = NXP_GINT_COMB_AND,
	};

	/* Configure GPIO pin as output */
	ret = gpio_pin_configure_dt(&gpio_pin0, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "Failed to configure GPIO 0 pin as output");
	ret = gpio_pin_configure_dt(&gpio_pin1, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "Failed to configure GPIO 1 pin as output");

	/* Set both pins LOW initially */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 0);
	zassert_ok(ret, "Failed to set pin0 LOW");
	ret = gpio_pin_set_raw(gpio_pin1.port, gpio_pin1.pin, 0);
	zassert_ok(ret, "Failed to set pin1 LOW");

	/* Register callback */
	ret = nxp_gint_register_callback(gint_dev, test_gint_edge_callback, NULL);
	zassert_ok(ret, "Failed to register callback");

	/* Configure GINT for AND mode */
	ret = nxp_gint_configure_group(gint_dev, &gint_config);
	zassert_ok(ret, "Failed to configure GINT group");

	/* Clear any pending interrupts */
	nxp_gint_clear_pending(gint_dev);

	/* Enable both pins with HIGH polarity */
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0, NXP_GINT_POL_HIGH);
	zassert_ok(ret, "Failed to enable pin0");
	ret = nxp_gint_enable_pin(gint_dev, TEST_PORT_1, TEST_PIN_1, NXP_GINT_POL_HIGH);
	zassert_ok(ret, "Failed to enable pin1");

	k_msleep(10);

	/* Reset callback counter */
	atomic_set(&gint_callback_count, 0);

	/* Trigger only pin0 - should NOT trigger interrupt in AND mode */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 1);
	zassert_ok(ret, "Failed to set pin0 HIGH");

	k_msleep(50);

	zassert_equal(atomic_get(&gint_callback_count), 0,
		      "GINT interrupt should NOT be triggered by pin0 alone in AND mode");

	/* Trigger pin1 as well - now both are HIGH, should trigger interrupt */
	ret = gpio_pin_set_raw(gpio_pin1.port, gpio_pin1.pin, 1);
	zassert_ok(ret, "Failed to set pin1 HIGH");

	k_msleep(50);

	zassert_equal(atomic_get(&gint_callback_count), 1,
		      "GINT interrupt should be triggered when both pins are HIGH in AND mode");

	/* Reset both pins and callback counter */
	ret = gpio_pin_set_raw(gpio_pin0.port, gpio_pin0.pin, 0);
	zassert_ok(ret, "Failed to set pin0 LOW");
	ret = gpio_pin_set_raw(gpio_pin1.port, gpio_pin1.pin, 0);
	zassert_ok(ret, "Failed to set pin1 LOW");
	k_msleep(10);
	atomic_set(&gint_callback_count, 0);

	/* Trigger only pin1 - should NOT trigger interrupt in AND mode */
	ret = gpio_pin_set_raw(gpio_pin1.port, gpio_pin1.pin, 1);
	zassert_ok(ret, "Failed to set pin1 HIGH");

	k_msleep(50);

	zassert_equal(atomic_get(&gint_callback_count), 0,
		      "GINT interrupt should NOT be triggered by pin1 alone in AND mode");

	/* Cleanup */
	nxp_gint_disable_pin(gint_dev, TEST_PORT_0, TEST_PIN_0);
	nxp_gint_disable_pin(gint_dev, TEST_PORT_1, TEST_PIN_1);
}

ZTEST_SUITE(intc_nxp_gint, NULL, test_gint_setup, test_gint_before, NULL, NULL);
