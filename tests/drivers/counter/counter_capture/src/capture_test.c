/*
 * Copyright (c) 2024 Ryan McClelland
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define NUM_OF_CONTINUOUS_CAPTURES CONFIG_NUM_OF_CONTINUOUS_CAPTURES

/* clang-format off */
static const struct gpio_dt_spec capture_tester_gpios[] = {
	DT_FOREACH_PROP_ELEM_SEP(
		DT_PATH(zephyr_user),
		capture_tester_gpios,
		GPIO_DT_SPEC_GET_BY_IDX, (,))
};

const struct counter_capture_dt_spec capture_dt_specs[] = {
	DT_FOREACH_PROP_ELEM_SEP(
		DT_NODELABEL(counter_loopback_0),
		test_counter_captures,
		COUNTER_CAPTURE_DT_SPEC_GET_BY_IDX, (,))
};
/* clang-format on */

K_SEM_DEFINE(capture_sem, 0, 1);

/***************************************************************
 * ISR capture callback
 ***************************************************************/

void capture_cb(const struct device *dev, uint8_t chan, counter_capture_flags_t flags,
		uint32_t ticks, void *user_data)
{
	TC_PRINT("Capture callback: channel %d, flags %d, ticks %u, %lluus\n", chan, flags, ticks,
		 counter_ticks_to_us(dev, ticks));

	k_sem_give(&capture_sem);
}

/***************************************************************
 * Helper cases
 ***************************************************************/

static int counter_capture_test_rising_edge_capture(size_t idx, uint8_t chan_id)
{
	int ret;

	/* Check if GPIO pin is already high, as it needs to be low initially */
	if (gpio_pin_get_dt(&capture_tester_gpios[idx]) == 1) {
		ret = gpio_pin_set_dt(&capture_tester_gpios[idx], 0);
		zassert_ok(ret, "idx %zu channel %u: failed to set GPIO pin low", idx, chan_id);
		/* give it some settling time */
		k_msleep(20);
	}

	/* Set GPIO pin to high to trigger rising edge */
	ret = gpio_pin_set_dt(&capture_tester_gpios[idx], 1);
	zassert_ok(ret, "idx %zu channel %u: failed to set GPIO pin high", idx, chan_id);

	ret = k_sem_take(&capture_sem, K_MSEC(100));
	if (ret != 0) {
		return ret;
	}

	/* Check GPIO pin state */
	ret = gpio_pin_get_dt(&capture_tester_gpios[idx]);
	if (ret == 0) {
		zassert_equal(
			ret, 1,
			"idx %zu channel %u: GPIO pin state does not match expected value of high",
			idx, chan_id);
	} else if (ret != 1) {
		zassert_equal(ret, 0, "idx %zu channel %u: failed to get GPIO pin state", idx,
			      chan_id);
	}

	return 0;
}

static int counter_capture_test_falling_edge_capture(size_t idx, uint8_t chan_id)
{
	int ret;

	/* Check if GPIO pin is already low, as it needs to be high initially */
	if (gpio_pin_get_dt(&capture_tester_gpios[idx]) == 0) {
		ret = gpio_pin_set_dt(&capture_tester_gpios[idx], 1);
		zassert_ok(ret, "idx %zu channel %u: failed to set GPIO pin high", idx, chan_id);
		/* give it some settling time */
		k_msleep(20);
	}

	/* Set GPIO pin to low to trigger falling edge */
	ret = gpio_pin_set_dt(&capture_tester_gpios[idx], 0);
	zassert_ok(ret, "idx %zu channel %u: failed to set GPIO pin low", idx, chan_id);

	ret = k_sem_take(&capture_sem, K_MSEC(100));
	if (ret != 0) {
		return ret;
	}

	/* Check GPIO pin state, expected to be low */
	ret = gpio_pin_get_dt(&capture_tester_gpios[idx]);
	if (ret == 1) {
		zassert_equal(
			ret, 0,
			"idx %zu channel %u: GPIO pin state does not match expected value of low",
			idx, chan_id);
	} else if (ret != 0) {
		zassert_equal(ret, 0, "idx %zu channel %u: failed to get GPIO pin state", idx,
			      chan_id);
	}

	return 0;
}

/***************************************************************
 * Test cases
 ***************************************************************/

ZTEST(counter_capture, test_rising_edge_continuous_capture)
{
	int ret;

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_capture_callback_set(
			spec->dev, spec->chan_id,
			COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_CONTINUOUS, capture_cb, NULL);
		if (ret == -ENOTSUP) {
			TC_PRINT("idx %zu channel %u: rising edge capture type not supported\n",
				 idx, spec->chan_id);
			continue;
		}

		zassert_ok(ret, "idx %zu channel %u: failed to set capture callback", idx,
			   spec->chan_id);

		ret = counter_enable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

		for (uint8_t i = 0; i < NUM_OF_CONTINUOUS_CAPTURES; i++) {
			ret = counter_capture_test_rising_edge_capture(idx, spec->chan_id);
			zassert_ok(ret, "idx %zu channel %u: rising edge capture test failed", idx,
				   spec->chan_id);
		}

		ret = counter_disable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to disable capture", idx,
			   spec->chan_id);
	}
}

ZTEST(counter_capture, test_rising_edge_single_capture)
{
	int ret;

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_capture_callback_set(spec->dev, spec->chan_id,
						   COUNTER_CAPTURE_RISING_EDGE |
							   COUNTER_CAPTURE_SINGLE_SHOT,
						   capture_cb, NULL);
		if (ret == -ENOTSUP) {
			TC_PRINT("idx %zu channel %u: rising edge capture type not supported\n",
				 idx, spec->chan_id);
			continue;
		}

		zassert_ok(ret, "idx %zu channel %u: failed to set capture callback", idx,
			   spec->chan_id);

		ret = counter_enable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

		ret = counter_capture_test_rising_edge_capture(idx, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: rising edge capture test failed", idx,
			   spec->chan_id);

		ret = counter_capture_test_rising_edge_capture(idx, spec->chan_id);
		zassert_equal(ret, -EAGAIN,
			      "idx %zu channel %u: capture callback should not be called after "
			      "single shot capture",
			      idx, spec->chan_id);

		ret = counter_disable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to disable capture", idx,
			   spec->chan_id);
	}
}

ZTEST(counter_capture, test_falling_edge_continuous_capture)
{
	int ret;

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_capture_callback_set(spec->dev, spec->chan_id,
						   COUNTER_CAPTURE_FALLING_EDGE |
							   COUNTER_CAPTURE_CONTINUOUS,
						   capture_cb, NULL);
		if (ret == -ENOTSUP) {
			TC_PRINT("idx %zu channel %u: falling edge capture type not supported\n",
				 idx, spec->chan_id);
			continue;
		}

		zassert_ok(ret, "idx %zu channel %u: failed to set capture callback", idx,
			   spec->chan_id);

		ret = counter_enable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

		for (uint8_t i = 0; i < NUM_OF_CONTINUOUS_CAPTURES; i++) {
			ret = counter_capture_test_falling_edge_capture(idx, spec->chan_id);
			zassert_ok(ret, "idx %zu channel %u: falling edge capture test failed", idx,
				   spec->chan_id);
		}

		ret = counter_disable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to disable capture", idx,
			   spec->chan_id);
	}
}

ZTEST(counter_capture, test_falling_edge_single_capture)
{
	int ret;

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_capture_callback_set(spec->dev, spec->chan_id,
						   COUNTER_CAPTURE_FALLING_EDGE |
							   COUNTER_CAPTURE_SINGLE_SHOT,
						   capture_cb, NULL);
		if (ret == -ENOTSUP) {
			TC_PRINT("idx %zu channel %u: falling edge capture type not supported\n",
				 idx, spec->chan_id);
			continue;
		}

		zassert_ok(ret, "idx %zu channel %u: failed to set capture callback", idx,
			   spec->chan_id);

		ret = counter_enable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

		ret = counter_capture_test_falling_edge_capture(idx, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: falling edge capture test failed", idx,
			   spec->chan_id);

		ret = counter_capture_test_falling_edge_capture(idx, spec->chan_id);
		zassert_equal(ret, -EAGAIN,
			      "idx %zu channel %u: capture callback should not be called after "
			      "single shot capture",
			      idx, spec->chan_id);

		ret = counter_disable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to disable capture", idx,
			   spec->chan_id);
	}
}

ZTEST(counter_capture, test_both_edges_continuous_capture)
{
	int ret;

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_capture_callback_set(
			spec->dev, spec->chan_id,
			COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_CONTINUOUS, capture_cb, NULL);
		if (ret == -ENOTSUP) {
			TC_PRINT("idx %zu channel %u: both edges capture type not supported\n", idx,
				 spec->chan_id);
			continue;
		}

		zassert_ok(ret, "idx %zu channel %u: failed to set capture callback", idx,
			   spec->chan_id);

		ret = counter_enable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

		for (uint8_t i = 0; i < NUM_OF_CONTINUOUS_CAPTURES; i++) {
			if (gpio_pin_get_dt(&capture_tester_gpios[idx]) == 0) {
				ret = counter_capture_test_rising_edge_capture(idx, spec->chan_id);
				zassert_ok(ret,
					   "idx %zu channel %u: rising edge capture test failed",
					   idx, spec->chan_id);

				ret = counter_capture_test_falling_edge_capture(idx, spec->chan_id);
				zassert_ok(ret,
					   "idx %zu channel %u: falling edge capture test failed",
					   idx, spec->chan_id);
			} else {
				ret = counter_capture_test_falling_edge_capture(idx, spec->chan_id);
				zassert_ok(ret,
					   "idx %zu channel %u: falling edge capture test failed",
					   idx, spec->chan_id);

				ret = counter_capture_test_rising_edge_capture(idx, spec->chan_id);
				zassert_ok(ret,
					   "idx %zu channel %u: rising edge capture test failed",
					   idx, spec->chan_id);
			}
		}

		ret = counter_disable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to disable capture", idx,
			   spec->chan_id);
	}
}

ZTEST(counter_capture, test_both_edges_single_capture)
{
	int ret;

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_capture_callback_set(
			spec->dev, spec->chan_id,
			COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_SINGLE_SHOT, capture_cb, NULL);
		if (ret == -ENOTSUP) {
			TC_PRINT("idx %zu channel %u: both edges capture type not supported\n", idx,
				 spec->chan_id);
			continue;
		}

		zassert_ok(ret, "idx %zu channel %u: failed to set capture callback", idx,
			   spec->chan_id);

		ret = counter_enable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

		if (gpio_pin_get_dt(&capture_tester_gpios[idx]) == 0) {
			ret = counter_capture_test_rising_edge_capture(idx, spec->chan_id);
			zassert_ok(ret, "idx %zu channel %u: rising edge capture test failed", idx,
				   spec->chan_id);

			ret = counter_capture_test_falling_edge_capture(idx, spec->chan_id);
			zassert_equal(ret, -EAGAIN,
				      "idx %zu channel %u: capture callback should not be called "
				      "after single shot capture",
				      idx, spec->chan_id);
		} else {
			ret = counter_capture_test_falling_edge_capture(idx, spec->chan_id);
			zassert_ok(ret, "idx %zu channel %u: falling edge capture test failed", idx,
				   spec->chan_id);

			ret = counter_capture_test_rising_edge_capture(idx, spec->chan_id);
			zassert_equal(ret, -EAGAIN,
				      "idx %zu channel %u: capture callback should not be called "
				      "after single shot capture",
				      idx, spec->chan_id);
		}

		ret = counter_disable_capture(spec->dev, spec->chan_id);
		zassert_ok(ret, "idx %zu channel %u: failed to disable capture", idx,
			   spec->chan_id);
	}
}

static void counter_before(void *f)
{
	int ret;

	ARG_UNUSED(f);

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_reset(spec->dev);
		zassert_ok(ret, "idx %zu channel %u: failed to reset counter", idx, spec->chan_id);

		ret = counter_start(spec->dev);
		zassert_ok(ret, "idx %zu channel %u: failed to start counter", idx, spec->chan_id);
	}
}

static void counter_after(void *f)
{
	int ret;

	ARG_UNUSED(f);

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		ret = counter_stop(spec->dev);
		zassert_ok(ret, "idx %zu channel %u: failed to stop counter", idx, spec->chan_id);
	}
}

static void *counter_setup(void)
{
	int ret;

	zassert_equal(ARRAY_SIZE(capture_dt_specs), ARRAY_SIZE(capture_tester_gpios),
		      "capture_dt_specs (%zu) and capture_tester_gpios (%zu) size mismatch",
		      ARRAY_SIZE(capture_dt_specs), ARRAY_SIZE(capture_tester_gpios));

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		zassert_true(device_is_ready(spec->dev),
			     "idx %zu channel %u: counter device is not ready", idx, spec->chan_id);

		zassert_true(gpio_is_ready_dt(&capture_tester_gpios[idx]),
			     "idx %zu channel %u: capture tester GPIO device is not ready", idx,
			     spec->chan_id);

		ret = gpio_pin_configure_dt(&capture_tester_gpios[idx], GPIO_OUTPUT_LOW);
		zassert_ok(ret, "idx %zu channel %u: failed to configure GPIO pin", idx,
			   spec->chan_id);
	}

	return NULL;
}

ZTEST_SUITE(counter_capture, NULL, counter_setup, counter_before, counter_after, NULL);
