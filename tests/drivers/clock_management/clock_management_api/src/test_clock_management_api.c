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
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	/* This request is designed to conflict with dev1_req0 */
	const struct clock_management_rate_req invalid_req = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_0, 1) + 1,
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_0, 1) + 1,
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	const struct clock_management_rate_req dev1_req1 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_1, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_1, 1),
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	const struct clock_management_rate_req dev2_req0 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_0, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_0, 1),
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	const struct clock_management_rate_req dev2_req1 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_1, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_1, 1),
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	/* Request that effectively clears any restrictions on the clock */
	const struct clock_management_rate_req loose_req = {
		.min_freq = 0,
		.max_freq = INT32_MAX,
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
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

ZTEST(clock_management_api, test_ranked)
{
	int dev1_req_freq2 = DT_PROP(DT_NODELABEL(emul_dev1), req_freq_2);
	int dev2_req_freq2 = DT_PROP(DT_NODELABEL(emul_dev2), req_freq_2);
	const struct clock_management_rate_req dev2_req2 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_2, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_2, 1),
		.max_rank = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev2), freq_constraints_2, 2),
	};
	const struct clock_management_rate_req dev1_req2 = {
		.min_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_2, 0),
		.max_freq = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_2, 1),
		.max_rank = DT_PROP_BY_IDX(DT_NODELABEL(emul_dev1), freq_constraints_2, 2),
	};
	/* Request that effectively clears any restrictions on the clock */
	const struct clock_management_rate_req loose_req = {
		.min_freq = 0,
		.max_freq = INT32_MAX,
		.max_rank = CLOCK_MANAGEMENT_ANY_RANK,
	};
	int ret;

	/* Make ranked request for first consumer */
	ret = clock_management_req_ranked(dev1_out, &dev1_req2);
	zassert_equal(ret, dev1_req_freq2,
		      "Consumer 1 got incorrect frequency for ranked request");
	ret = clock_management_req_ranked(dev2_out, &dev2_req2);
	zassert_equal(ret, dev2_req_freq2,
		      "Consumer 2 got incorrect frequency for ranked request");
	/* Clear restrictions on clock outputs */
	ret = clock_management_req_rate(dev1_out, &loose_req);
	zassert_true(ret > 0, "Consumer 1 could not remove clock restrictions");
	ret = clock_management_req_rate(dev2_out, &loose_req);
	zassert_true(ret > 0, "Consumer 2 could not remove clock restrictions");
}

#if DT_HAS_COMPAT_STATUS_OKAY(vnd_emul_clock_gateable)
/* Only run this test if the gateable clock is present- this is all emulated,
 * so it likely only needs to run on native_sim
 */

CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev3), default);

static const struct clock_output *dev3_out =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev3), default);

/*
 * Define a basic driver here for the gateable clock
 */

#define DT_DRV_COMPAT vnd_emul_clock_gateable

static bool clock_is_gated;

struct gateable_clock_data {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
};

static clock_freq_t gateable_clock_recalc_rate(const struct clk *clk_hw, clock_freq_t parent_rate)
{
	return clock_is_gated ? parent_rate : 0;
}

static int gateable_clock_onoff(const struct clk *clk_hw, bool on)
{
	clock_is_gated = !on;
	return 0;
}

const struct clock_management_standard_api gateable_clock_api = {
	.recalc_rate = gateable_clock_recalc_rate,
	.shared.on_off = gateable_clock_onoff,
};

#define GATEABLE_CLOCK_DEFINE(inst)                                            \
	static struct gateable_clock_data gate_clk_##inst = {                  \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
	};                                                                     \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &gate_clk_##inst,                                 \
			     &gateable_clock_api);

DT_INST_FOREACH_STATUS_OKAY(GATEABLE_CLOCK_DEFINE)

ZTEST(clock_management_api, test_onoff)
{
	/* First disable all unused clocks. We should see the gateable one switch off. */
	clock_management_disable_unused();
	zassert_true(clock_is_gated, "Emulated clock is unused but did not gate");
	/* Now enable the clock for dev3 */
	clock_management_on(dev3_out);
	zassert_false(clock_is_gated, "Emulated clock is in use but gated");
	/* Make sure the clock doesn't turn off now, it is in use */
	clock_management_disable_unused();
	zassert_false(clock_is_gated, "Emulated clock is in use but gated during disabled_unused");
	/* Raise reference count to clock */
	clock_management_on(dev3_out);
	/* Lower reference count */
	clock_management_off(dev3_out);
	zassert_false(clock_is_gated, "Emulated clock should not gate, one reference still exists");
	/* Turn off the clock */
	clock_management_off(dev3_out);
	zassert_true(clock_is_gated, "Emulated clock is off but did not gate");
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(vnd_emul_clock_gateable) */

ZTEST_SUITE(clock_management_api, NULL, NULL, reset_clock_states, NULL, NULL);
