/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_mgmt.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

/* Custom clock management states for this test */
#define CLOCK_MGMT_STATE_TEST1 CLOCK_MGMT_STATE_PRIV_START
#define CLOCK_MGMT_STATE_TEST2 (CLOCK_MGMT_STATE_PRIV_START + 1)
#define CLOCK_MGMT_STATE_TEST3 (CLOCK_MGMT_STATE_PRIV_START + 2)

#define CONSUMER_NODE DT_INST(0, vnd_emul_clock_consumer)

/* Define clock management states for clock consumer */
CLOCK_MGMT_DEFINE(CONSUMER_NODE);

/* Get references to clock management state */
static const struct clock_mgmt *consumer =
	CLOCK_MGMT_DT_DEV_CONFIG_GET(CONSUMER_NODE);

void apply_clock_state(uint8_t state_idx, const char *state_name,
		       int expected_rate)
{
	int ret;

	/* Apply clock state, verify frequencies */
	TC_PRINT("Try to apply %s clock state\n", state_name);

	ret = clock_mgmt_apply_state(consumer, state_idx);
	zassert_equal(ret, 0, "Failed to apply %s clock management state",
		      state_name);

	/* Check rate */
	ret = clock_mgmt_get_rate(consumer, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer %s clock rate: %d\n", state_name, ret);
	zassert_equal(ret, expected_rate,
		      "Consumer has invalid %s clock rate", state_name);
}

ZTEST(clock_mgmt_hw, test_apply_states)
{
	apply_clock_state(CLOCK_MGMT_STATE_DEFAULT, "default",
			  DT_PROP(CONSUMER_NODE, default_freq));
	apply_clock_state(CLOCK_MGMT_STATE_SLEEP, "sleep",
			  DT_PROP(CONSUMER_NODE, sleep_freq));
	apply_clock_state(CLOCK_MGMT_STATE_TEST1, "test1",
			  DT_PROP(CONSUMER_NODE, test1_freq));
	apply_clock_state(CLOCK_MGMT_STATE_TEST2, "test2",
			  DT_PROP(CONSUMER_NODE, test2_freq));
	apply_clock_state(CLOCK_MGMT_STATE_TEST3, "test3",
			  DT_PROP(CONSUMER_NODE, test3_freq));
}

ZTEST_SUITE(clock_mgmt_hw, NULL, NULL, NULL, NULL, NULL);
