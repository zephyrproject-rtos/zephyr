/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_mgmt.h>
#include <zephyr/logging/log.h>

#include "clock_drivers/emul_clock_drivers.h"
LOG_MODULE_REGISTER(test);

/* Custom clock management states for this test */
#define CLOCK_MGMT_STATE_INVALID CLOCK_MGMT_STATE_SLEEP
#define CLOCK_MGMT_STATE_SHARED CLOCK_MGMT_STATE_PRIV_START
#define CLOCK_MGMT_STATE_SETRATE (CLOCK_MGMT_STATE_PRIV_START + 1)
#define CLOCK_MGMT_STATE_SETRATE1 (CLOCK_MGMT_STATE_PRIV_START + 2)

/* Define clock management states for both clock consumers */
CLOCK_MGMT_DEFINE(DT_NODELABEL(emul_dev1));
CLOCK_MGMT_DEFINE(DT_NODELABEL(emul_dev2));

/* Get references to each clock management state */
static const struct clock_mgmt *consumer1 =
	CLOCK_MGMT_DT_DEV_CONFIG_GET(DT_NODELABEL(emul_dev1));
static const struct clock_mgmt *consumer2 =
	CLOCK_MGMT_DT_DEV_CONFIG_GET(DT_NODELABEL(emul_dev2));

struct consumer_cb_data {
	uint32_t rate;
	bool signalled;
};

static struct consumer_cb_data consumer1_cb_data;
static struct consumer_cb_data consumer2_cb_data;

static int consumer_cb(uint8_t output_idx, uint32_t new_rate,
			const void *data)
{
	struct consumer_cb_data *cb_data = (struct consumer_cb_data *)data;

	cb_data->rate = new_rate;
	cb_data->signalled = true;
	return 0;
}

ZTEST(clock_mgmt_api, test_basic_state)
{
	int ret;

	/* Apply default clock states for both consumers, make sure
	 * that rates match what is expected
	 */
	TC_PRINT("Applying default clock states\n");

	ret = clock_mgmt_apply_state(consumer1, CLOCK_MGMT_STATE_DEFAULT);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	ret = clock_mgmt_get_rate(consumer1, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 1 default clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), default_freq),
		      "Consumer 1 has invalid clock rate");

	ret = clock_mgmt_apply_state(consumer2, CLOCK_MGMT_STATE_DEFAULT);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	ret = clock_mgmt_get_rate(consumer2, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 2 default clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev2), default_freq),
		      "Consumer 2 has invalid clock rate");
}

ZTEST(clock_mgmt_api, test_invalid_state)
{
	int ret;
	/* Apply invalid clock state, verify an error is returned */
	TC_PRINT("Try to apply invalid clock states\n");

	ret = clock_mgmt_apply_state(consumer1, CLOCK_MGMT_STATE_INVALID);
	zassert_not_equal(ret, 0, "Invalid state should return an error");
	ret = clock_mgmt_apply_state(consumer2, CLOCK_MGMT_STATE_INVALID);
	zassert_not_equal(ret, 0, "Invalid state should return an error");
}


ZTEST(clock_mgmt_api, test_shared_notification)
{
	int ret;
	/* Apply invalid clock state, verify an error is returned */
	TC_PRINT("Try to apply shared clock states\n");

	ret = clock_mgmt_set_callback(consumer1, consumer_cb,
				      &consumer1_cb_data);
	zassert_equal(ret, 0, "Could not install callback");
	ret = clock_mgmt_set_callback(consumer2, consumer_cb,
				      &consumer2_cb_data);
	zassert_equal(ret, 0, "Could not install callback");

	/* Reset clock tree to default state */
	ret = clock_mgmt_apply_state(consumer1, CLOCK_MGMT_STATE_DEFAULT);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	ret = clock_mgmt_apply_state(consumer2, CLOCK_MGMT_STATE_DEFAULT);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	/* Clear any old callback notifications */
	consumer1_cb_data.signalled = false;
	consumer2_cb_data.signalled = false;

	ret = clock_mgmt_apply_state(consumer1, CLOCK_MGMT_STATE_SHARED);
	zassert_equal(ret, 0, "Shared state should apply correctly");
	/* At this point only the first consumer should have a notification */
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	zassert_false(consumer2_cb_data.signalled,
		      "Consumer 2 should not have callback notification");

	/* Clear any old callback notifications */
	consumer1_cb_data.signalled = false;
	consumer2_cb_data.signalled = false;
	ret = clock_mgmt_apply_state(consumer2, CLOCK_MGMT_STATE_SHARED);
	zassert_equal(ret, 0, "Shared state should apply correctly");
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	zassert_true(consumer2_cb_data.signalled,
		     "Consumer 2 should have callback notification");
	/* Check rates */
	ret = clock_mgmt_get_rate(consumer1, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 1 shared clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), shared_freq),
		      "Consumer 1 has invalid clock rate");
	ret = clock_mgmt_get_rate(consumer2, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 2 shared clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev2), shared_freq),
		      "Consumer 2 has invalid clock rate");
}


ZTEST(clock_mgmt_api, test_setrate)
{
	int ret;
	/* Apply setrate clock state, verify frequencies */
	TC_PRINT("Try to apply setrate clock states\n");

	ret = clock_mgmt_apply_state(consumer1, CLOCK_MGMT_STATE_SETRATE);
	zassert_equal(ret, 0, "Failed to apply setrate clock management state");
	ret = clock_mgmt_apply_state(consumer2, CLOCK_MGMT_STATE_SETRATE);
	zassert_equal(ret, 0, "Failed to apply setrate clock management state");

	/* Check rates */
	ret = clock_mgmt_get_rate(consumer1, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 1 setrate clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), setrate_freq),
		      "Consumer 1 has invalid clock rate");
	ret = clock_mgmt_get_rate(consumer2, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 2 setrate clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev2), setrate_freq),
		      "Consumer 2 has invalid clock rate");

	/* Apply setrate1 clock state, verify frequencies */
	TC_PRINT("Try to apply setrate1 clock states\n");

	ret = clock_mgmt_apply_state(consumer1, CLOCK_MGMT_STATE_SETRATE1);
	zassert_equal(ret, 0, "Failed to apply setrate1 clock management state");
	ret = clock_mgmt_apply_state(consumer2, CLOCK_MGMT_STATE_SETRATE1);
	zassert_equal(ret, 0, "Failed to apply setrate1 clock management state");

	/* Check rates */
	ret = clock_mgmt_get_rate(consumer1, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 1 setrate1 clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), setrate1_freq),
		      "Consumer 1 has invalid clock rate");
	ret = clock_mgmt_get_rate(consumer2, CLOCK_MGMT_OUTPUT_DEFAULT);
	TC_PRINT("Consumer 2 setrate1 clock rate: %d\n", ret);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev2), setrate1_freq),
		      "Consumer 2 has invalid clock rate");
}

ZTEST_SUITE(clock_mgmt_api, NULL, NULL, NULL, NULL, NULL);
