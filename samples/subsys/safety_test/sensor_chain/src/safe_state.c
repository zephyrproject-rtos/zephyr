/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/safety_test/safety_test.h>
#include <zephyr/sys/atomic.h>

#include "sample_common.h"

LOG_MODULE_REGISTER(sample_safe_state, LOG_LEVEL_INF);

static const struct gpio_dt_spec fault_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static atomic_t fault_flag;

bool sample_fault_latched(void)
{
	return atomic_get(&fault_flag) != 0;
}

static void light_fault_led(void)
{
	/*
	 * A boot-level failure can arrive before the GPIO driver exists, so the
	 * LED is best effort. Driving it unconditionally would fault inside the
	 * failure path, which is the worst place to fault.
	 */
	if (gpio_is_ready_dt(&fault_led)) {
		(void)gpio_pin_configure_dt(&fault_led, GPIO_OUTPUT_ACTIVE);
	}
}

static void on_failure(const struct safety_test *test,
		       const struct safety_test_result_record *rec)
{
	atomic_set(&fault_flag, 1);

	LOG_ERR("test '%s' failed with %d after %u us", test->name, rec->error_code,
		rec->duration_us);

	light_fault_led();
}

SAFETY_TEST_FAILURE_HOOK_DEFINE(sample_fault_hook, on_failure);

static enum safety_test_action configured_action(void)
{
	if (IS_ENABLED(CONFIG_SAMPLE_SAFE_STATE_ACTION_RESET)) {
		return SAFETY_TEST_ACTION_RESET;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_SAFE_STATE_ACTION_HALT)) {
		return SAFETY_TEST_ACTION_HALT;
	}

	return SAFETY_TEST_ACTION_CONTINUE;
}

static const char *action_name(enum safety_test_action action)
{
	switch (action) {
	case SAFETY_TEST_ACTION_RESET:
		return "RESET";
	case SAFETY_TEST_ACTION_CONTINUE:
		return "CONTINUE";
	default:
		return "HALT";
	}
}

enum safety_test_action safety_test_safe_state(const struct safety_test *test,
					       const struct safety_test_result_record *rec)
{
	enum safety_test_action action = configured_action();

	ARG_UNUSED(rec);

	light_fault_led();

	/*
	 * What the board actually does is not decided here alone. With
	 * CONFIG_SAFETY_TEST_STRICT_CRITICAL set the subsystem refuses a
	 * CONTINUE and halts instead, so the same source produces two
	 * behaviours from configuration alone.
	 */
	LOG_ERR("safe state reached for '%s', returning %s", test->name, action_name(action));

	return action;
}
