/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_management.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test);

/* Define clock management states for both clock consumers */
CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), default);
CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev2), default);

/* Get references to each clock management state and output */
static const struct clock_output *dev1_out =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), default);
static clock_management_state_t dev1_default =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), default, default);
static clock_management_state_t dev1_invalid =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), default, invalid);
static clock_management_state_t dev1_shared =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), default, shared);
static clock_management_state_t dev1_locking =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev1), default, locking);

static const struct clock_output *dev2_out =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev2), default);
static clock_management_state_t dev2_default =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev2), default, default);
static clock_management_state_t dev2_invalid =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev2), default, invalid);
static clock_management_state_t dev2_shared =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev2), default, shared);
static clock_management_state_t dev2_locking =
	CLOCK_MANAGEMENT_DT_GET_STATE(DT_NODELABEL(emul_dev2), default, locking);

/* Define a second output using the same clock as emul_dev1 */
CLOCK_MANAGEMENT_DEFINE_OUTPUT(DT_PHANDLE_BY_IDX(DT_NODELABEL(emul_dev1), clock_outputs,
		DT_CLOCK_OUTPUT_NAME_IDX(DT_NODELABEL(emul_dev1), default)),
		sw_clock_consumer);
static const struct clock_output *dev1_sw_consumer =
	CLOCK_MANAGEMENT_GET_OUTPUT(DT_PHANDLE_BY_IDX(DT_NODELABEL(emul_dev1),
		clock_outputs, DT_CLOCK_OUTPUT_NAME_IDX(DT_NODELABEL(emul_dev1),
		default)), sw_clock_consumer);

struct consumer_cb_data {
	uint32_t rate;
	bool signalled;
};

static struct consumer_cb_data consumer1_cb_data;
static struct consumer_cb_data consumer2_cb_data;

static int consumer_cb(const struct clock_management_event *ev, const void *data)
{
	struct consumer_cb_data *cb_data = (struct consumer_cb_data *)data;

	if (ev->type == CLOCK_MANAGEMENT_POST_RATE_CHANGE) {
		cb_data->rate = ev->new_rate;
		cb_data->signalled = true;
	}
	return 0;
}

/* Runs before every test, resets clocks to default state */
void reset_clock_states(void *unused)
{
	ARG_UNUSED(unused);
	int ret;

	/* Reset clock tree to default state */
	ret = clock_management_apply_state(dev1_out, dev1_default);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), default_freq),
		      "Failed to apply default clock management state");
	ret = clock_management_apply_state(dev2_out, dev2_default);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev2), default_freq),
		      "Failed to apply default clock management state");
	/* Clear any old callback notifications */
	consumer1_cb_data.signalled = false;
	consumer2_cb_data.signalled = false;
}

ZTEST(clock_management_api, test_basic_state)
{
	int ret;
	int dev1_default_freq = DT_PROP(DT_NODELABEL(emul_dev1), default_freq);
	int dev2_default_freq = DT_PROP(DT_NODELABEL(emul_dev2), default_freq);

	/* Apply default clock states for both consumers, make sure
	 * that rates match what is expected
	 */
	TC_PRINT("Applying default clock states\n");

	ret = clock_management_apply_state(dev1_out, dev1_default);
	zassert_equal(ret, dev1_default_freq,
		      "Failed to apply default clock management state");
	ret = clock_management_get_rate(dev1_out);
	TC_PRINT("Consumer 1 default clock rate: %d\n", ret);
	zassert_equal(ret, dev1_default_freq,
		      "Consumer 1 has invalid clock rate");

	ret = clock_management_apply_state(dev2_out, dev2_default);
	zassert_equal(ret, dev2_default_freq,
		      "Failed to apply default clock management state");
	ret = clock_management_get_rate(dev2_out);
	TC_PRINT("Consumer 2 default clock rate: %d\n", ret);
	zassert_equal(ret, dev2_default_freq,
		      "Consumer 2 has invalid clock rate");
}

ZTEST(clock_management_api, test_invalid_state)
{
	int ret;
	/* Apply invalid clock state, verify an error is returned */
	TC_PRINT("Try to apply invalid clock states\n");

	ret = clock_management_apply_state(dev1_out, dev1_invalid);
	zassert_not_equal(ret, 0, "Invalid state should return an error");
	ret = clock_management_apply_state(dev2_out, dev2_invalid);
	zassert_not_equal(ret, 0, "Invalid state should return an error");
}


ZTEST(clock_management_api, test_shared_notification)
{
	int ret;
	int dev1_shared_freq = DT_PROP(DT_NODELABEL(emul_dev1), shared_freq);
	int dev2_shared_freq = DT_PROP(DT_NODELABEL(emul_dev2), shared_freq);
	/* Apply invalid clock state, verify an error is returned */
	TC_PRINT("Try to apply shared clock states\n");

	ret = clock_management_set_callback(dev1_out, consumer_cb,
				      &consumer1_cb_data);
	zassert_equal(ret, 0, "Could not install callback");
	ret = clock_management_set_callback(dev2_out, consumer_cb,
				      &consumer2_cb_data);
	zassert_equal(ret, 0, "Could not install callback");


	ret = clock_management_apply_state(dev1_out, dev1_shared);
	/*
	 * Note- here the return value is not guaranteed to match shared-freq
	 * property, since the state being applied is independent of the state
	 * applied for dev2_out
	 */
	zassert_true(ret > 0, "Shared state should apply correctly");
	/* At this point only the first consumer should have a notification */
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	zassert_false(consumer2_cb_data.signalled,
		      "Consumer 2 should not have callback notification");

	/* Clear any old callback notifications */
	consumer1_cb_data.signalled = false;
	consumer2_cb_data.signalled = false;
	ret = clock_management_apply_state(dev2_out, dev2_shared);
	zassert_equal(ret, dev2_shared_freq,
		      "Shared state should apply correctly");
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	zassert_true(consumer2_cb_data.signalled,
		     "Consumer 2 should have callback notification");
	/* Check rates */
	ret = clock_management_get_rate(dev1_out);
	TC_PRINT("Consumer 1 shared clock rate: %d\n", ret);
	zassert_equal(ret, dev1_shared_freq,
		      "Consumer 1 has invalid clock rate");
	ret = clock_management_get_rate(dev2_out);
	TC_PRINT("Consumer 2 shared clock rate: %d\n", ret);
	zassert_equal(ret, dev2_shared_freq,
		      "Consumer 2 has invalid clock rate");
}

ZTEST(clock_management_api, test_locking)
{
	int ret;

	ret = clock_management_apply_state(dev1_out, dev1_locking);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), locking_freq),
		      "Failed to apply locking state for first consumer");
	ret = clock_management_apply_state(dev2_out, dev2_locking);
	zassert(ret < 0, "Locking state for second consumer should "
		"fail to apply");
}

ZTEST(clock_management_api, test_setrate)
{
	const struct clock_management_rate_req dev1_req0 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_0, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_0, 1),
	};
	/* This request is designed to conflict with dev1_req0 */
	const struct clock_management_rate_req invalid_req = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_0, 1) + 1,
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_0, 1) + 1,
	};
	const struct clock_management_rate_req dev1_req1 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_1, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_1, 1),
	};
	const struct clock_management_rate_req dev2_req0 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_0, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_0, 1),
	};
	const struct clock_management_rate_req dev2_req1 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_1, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_1, 1),
	};
	/* Request that effectively clears any restrictions on the clock */
	const struct clock_management_rate_req loose_req = {
		.min_freq = 0,
		.max_freq = INT32_MAX,
	};
	int dev1_req_freq0 = DT_PROP(DT_NODELABEL(emul_dev1), req_freq_0);
	int dev2_req_freq0 = DT_PROP(DT_NODELABEL(emul_dev2), req_freq_0);
	int dev2_req_freq1 = DT_PROP(DT_NODELABEL(emul_dev2), req_freq_1);
	int ret;

	ret = clock_management_set_callback(dev1_out, consumer_cb,
				      &consumer1_cb_data);
	zassert_equal(ret, 0, "Could not install callback");
	ret = clock_management_set_callback(dev2_out, consumer_cb,
				      &consumer2_cb_data);
	zassert_equal(ret, 0, "Could not install callback");

	/* Apply constraints for first consumer */
	ret = clock_management_req_rate(dev1_out, &dev1_req0);
	zassert_equal(ret, dev1_req_freq0,
		      "Consumer 1 got incorrect frequency for first request");
	ret = clock_management_req_rate(dev1_sw_consumer, &invalid_req);
	zassert_true(ret < 0,
		     "Conflicting software consumer request should be denied");
	/* Clear any old callback notifications */
	consumer1_cb_data.signalled = false;
	ret = clock_management_req_rate(dev2_out, &dev2_req0);
	zassert_equal(ret, dev2_req_freq0,
		      "Consumer 2 got incorrect frequency for first request");
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	ret = clock_management_req_rate(dev1_out, &dev1_req1);
	zassert_true(ret < 0, "Consumer 1 second request should be denied");
	ret = clock_management_req_rate(dev2_out, &dev2_req1);
	zassert_equal(ret, dev2_req_freq1,
		      "Consumer 2 got incorrect frequency for second request");
	/* Clear restrictions on clock outputs */
	ret = clock_management_req_rate(dev1_out, &loose_req);
	zassert_true(ret > 0, "Consumer 1 could not remove clock restrictions");
	ret = clock_management_req_rate(dev2_out, &loose_req);
	zassert_true(ret > 0, "Consumer 2 could not remove clock restrictions");
}

ZTEST_SUITE(clock_management_api, NULL, NULL, reset_clock_states, NULL, NULL);
