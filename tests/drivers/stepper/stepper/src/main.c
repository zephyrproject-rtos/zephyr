/*
 * Copyright 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

struct stepper_fixture {
	const struct device *dev;
};

struct gpio_dt_spec en_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(stepper), en_gpios, {0});
struct gpio_dt_spec slp_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(stepper), sleep_gpios, {0});
struct gpio_dt_spec m0_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(stepper), m0_gpios, {0});
struct gpio_dt_spec m1_pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(stepper), m1_gpios, {0});

static void *stepper_setup(void)
{
	static struct stepper_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_ALIAS(stepper)),
	};

	zassert_not_null(fixture.dev);
	return &fixture;
}

ZTEST_F(stepper, test_enable_gpio_pins)
{
	int value = 0;
	int err;

	err = stepper_enable(fixture->dev);
	if (err == -ENOTSUP) {
		ztest_test_skip();
	}
	/* As sleep and enable pins are optional, check if they exist*/
	if (en_pin.port != NULL) {
		value = gpio_emul_output_get(en_pin.port, en_pin.pin);
		zassert_equal(value, 1, "Enable pin should be set");
	}
	if (slp_pin.port != NULL) {
		value = !gpio_emul_output_get(slp_pin.port, slp_pin.pin);
		zassert_equal(value, 0, "Sleep pin should not be set");
	}

	/* As enable is supported, disable must also be supported */
	zassert_ok(stepper_disable(fixture->dev));

	if (en_pin.port != NULL) {
		value = gpio_emul_output_get(en_pin.port, en_pin.pin);
		zassert_equal(value, 0, "Enable pin should not be set");
	}
	if (slp_pin.port != NULL) {
		value = !gpio_emul_output_get(slp_pin.port, slp_pin.pin);
		zassert_equal(value, 1, "Sleep pin should be set");
	}
}

ZTEST_F(stepper, test_micro_step_res_set)
{
	enum stepper_micro_step_resolution res;
	int value = 0;

	zassert_ok(stepper_set_micro_step_res(fixture->dev, CONFIG_MICRO_STEP_RESOLUTION));

	if (m0_pin.port == NULL || m1_pin.port == NULL) {
		ztest_test_skip();
	}

	value = gpio_emul_output_get(m0_pin.port, m0_pin.pin);
	zassert_equal(value, CONFIG_MICRO_STEP_RESOLUTION_M0, "M0 pin should be %d",
		      CONFIG_MICRO_STEP_RESOLUTION_M0);

	value = gpio_emul_output_get(m1_pin.port, m1_pin.pin);
	zassert_equal(value, CONFIG_MICRO_STEP_RESOLUTION_M1, "M1 pin should be 1",
		      CONFIG_MICRO_STEP_RESOLUTION_M1);

	zassert_ok(stepper_get_micro_step_res(fixture->dev, &res));
	zassert_equal(res, CONFIG_MICRO_STEP_RESOLUTION,
		      "Micro step resolution not set correctly, should be %d but is %d",
		      CONFIG_MICRO_STEP_RESOLUTION, res);
}

ZTEST_F(stepper, test_set_micro_step_res_invalid)
{
	int ret = stepper_set_micro_step_res(fixture->dev, 127);

	zassert_equal(ret, -EINVAL, "Invalid micro step resolution should return -EINVAL");
}

struct fault_event_state {
	struct gpio_dt_spec pin;
	int inactive_level;
	int release_ret;
	struct k_timer timer;
	struct k_sem cb_sem;
	const struct device *cb_dev;
	enum stepper_event cb_event;
	bool cb_in_isr;
};

static void fault_event_cb(const struct device *dev, const enum stepper_event event,
			   void *user_data)
{
	struct fault_event_state *state = user_data;

	state->cb_dev = dev;
	state->cb_event = event;
	state->cb_in_isr = k_is_in_isr();
	k_sem_give(&state->cb_sem);
}

/* Timer expiry functions run in ISR context, like a SoC GPIO interrupt. */
static void fault_pin_release(struct k_timer *timer)
{
	struct fault_event_state *state = k_timer_user_data_get(timer);

	state->release_ret =
		gpio_emul_input_set(state->pin.port, state->pin.pin, state->inactive_level);
}

/* A fault interrupt taken in ISR context reaches the event callback in thread context. */
ZTEST_F(stepper, test_fault_event_thread_context)
{
	static struct fault_event_state state = {
		.pin = GPIO_DT_SPEC_GET_OR(DT_ALIAS(stepper), fault_gpios, {0}),
	};
	bool active_low;

	if (state.pin.port == NULL) {
		ztest_test_skip();
	}

	active_low = (state.pin.dt_flags & GPIO_ACTIVE_LOW) != 0U;
	state.inactive_level = active_low ? 1 : 0;
	k_sem_init(&state.cb_sem, 0, 1);
	k_timer_init(&state.timer, fault_pin_release, NULL);
	k_timer_user_data_set(&state.timer, &state);

	/* The driver interrupts on the edge to inactive, so start with the fault active. */
	zassert_ok(gpio_emul_input_set(state.pin.port, state.pin.pin, active_low ? 0 : 1));
	zassert_ok(stepper_set_event_cb(fixture->dev, fault_event_cb, &state));
	zassert_ok(stepper_enable(fixture->dev));

	k_timer_start(&state.timer, K_MSEC(1), K_NO_WAIT);
	(void)k_timer_status_sync(&state.timer);
	zassert_ok(state.release_ret);

	zassert_ok(k_sem_take(&state.cb_sem, K_MSEC(100)), "Fault callback not invoked");
	zassert_equal(state.cb_dev, fixture->dev);
	zassert_equal(state.cb_event, STEPPER_EVENT_FAULT_DETECTED);
	zassert_false(state.cb_in_isr, "Fault callback invoked from ISR context");

	zassert_ok(stepper_disable(fixture->dev));
	zassert_ok(stepper_set_event_cb(fixture->dev, NULL, NULL));
}

ZTEST_SUITE(stepper, NULL, stepper_setup, NULL, NULL, NULL);
