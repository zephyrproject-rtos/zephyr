/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_management.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define CONSUMER_NODE DT_NODELABEL(emul_dev)

CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(CONSUMER_NODE, default);

/* Get references to each clock management state and output */
static const struct clock_output *dev_out =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(CONSUMER_NODE, default);
static clock_management_state_t dev_default =
	CLOCK_MANAGEMENT_DT_GET_STATE(CONSUMER_NODE, default, default);
static clock_management_state_t dev_sleep =
	CLOCK_MANAGEMENT_DT_GET_STATE(CONSUMER_NODE, default, sleep);
static clock_management_state_t dev_test1 =
	CLOCK_MANAGEMENT_DT_GET_STATE(CONSUMER_NODE, default, test1);
static clock_management_state_t dev_test2 =
	CLOCK_MANAGEMENT_DT_GET_STATE(CONSUMER_NODE, default, test2);
static clock_management_state_t dev_test3 =
	CLOCK_MANAGEMENT_DT_GET_STATE(CONSUMER_NODE, default, test3);

void apply_clock_state(clock_management_state_t state, const char *state_name,
		       int expected_rate)
{
	int ret;

	/* Apply clock state, verify frequencies */
	TC_PRINT("Try to apply %s clock state\n", state_name);

	ret = clock_management_apply_state(dev_out, state);
	zassert_equal(ret, expected_rate,
		      "Failed to apply %s clock management state", state_name);

	/* Check rate */
	ret = clock_management_get_rate(dev_out);
	TC_PRINT("Consumer %s clock rate: %d\n", state_name, ret);
	zassert_equal(ret, expected_rate,
		      "Consumer has invalid %s clock rate", state_name);
}

ZTEST(clock_management_hw, test_apply_states)
{
	apply_clock_state(dev_default, "default",
			  DT_PROP(CONSUMER_NODE, default_freq));
	apply_clock_state(dev_sleep, "sleep",
			  DT_PROP(CONSUMER_NODE, sleep_freq));
	apply_clock_state(dev_test1, "test1",
			  DT_PROP(CONSUMER_NODE, test1_freq));
	apply_clock_state(dev_test2, "test2",
			  DT_PROP(CONSUMER_NODE, test2_freq));
	apply_clock_state(dev_test3, "test3",
			  DT_PROP(CONSUMER_NODE, test3_freq));
}

ZTEST_SUITE(clock_management_hw, NULL, NULL, NULL, NULL, NULL);
