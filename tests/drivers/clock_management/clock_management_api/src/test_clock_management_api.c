/*
 * Copyright 2024 NXP
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_management.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test);

/* Define clock management states for both clock consumers */
CLOCK_MANAGEMENT_DT_DEFINE(DT_NODELABEL(emul_dev1));
CLOCK_MANAGEMENT_DT_DEFINE(DT_NODELABEL(emul_dev2));
const struct clock_management_data *dev1_data =
	CLOCK_MANAGEMENT_DT_GET(DT_NODELABEL(emul_dev1));
const struct clock_management_data *dev2_data =
	CLOCK_MANAGEMENT_DT_GET(DT_NODELABEL(emul_dev2));

/* Get references to each clock management state and output */
clock_output_t dev1_out =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev1), default);
clock_request_t dev1_default =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev1), default);
clock_request_t dev1_invalid =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev1), invalid);
clock_request_t dev1_ranked =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev1), ranked);

clock_output_t dev2_out =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT_BY_NAME(DT_NODELABEL(emul_dev2), default);
clock_request_t dev2_default =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev2), default);
clock_request_t dev2_invalid =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev2), invalid);
clock_request_t dev2_ranked =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev2), ranked);
clock_request_t dev2_shared =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev2), shared);
clock_request_t dev2_locked =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(DT_NODELABEL(emul_dev2), locked);


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
	ret = clock_management_request_state(dev1_data, dev1_default);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	ret = clock_management_get_rate(dev1_data, dev1_out);
	zassert_equal(ret, DT_PROP(DT_NODELABEL(emul_dev1), default_freq),
		      "Failed to apply default clock management state");
	ret = clock_management_request_state(dev2_data, dev2_default);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	ret = clock_management_get_rate(dev2_data, dev2_out);
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

	/* Request default clock states for both consumers, make sure
	 * that rates match what is expected
	 */
	TC_PRINT("Requesting default clock states\n");

	ret = clock_management_request_state(dev1_data, dev1_default);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	ret = clock_management_get_rate(dev1_data, dev1_out);
	TC_PRINT("Consumer 1 default clock rate: %d\n", ret);
	zassert_equal(ret, dev1_default_freq,
		      "Consumer 1 has invalid clock rate");

	ret = clock_management_request_state(dev2_data, dev2_default);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	ret = clock_management_get_rate(dev2_data, dev2_out);
	TC_PRINT("Consumer 2 default clock rate: %d\n", ret);
	zassert_equal(ret, dev2_default_freq,
		      "Consumer 2 has invalid clock rate");
}

ZTEST(clock_management_api, test_invalid_state)
{
	int ret;
	/* Apply invalid clock state, verify an error is returned */
	TC_PRINT("Try to apply invalid clock states\n");

	ret = clock_management_request_state(dev1_data, dev1_invalid);
	zassert_not_equal(ret, 0, "Invalid state should return an error");
	ret = clock_management_request_state(dev2_data, dev2_invalid);
	zassert_not_equal(ret, 0, "Invalid state should return an error");
}


ZTEST(clock_management_api, test_shared_notification)
{
	int ret;
	int dev1_ranked_freq = DT_PROP(DT_NODELABEL(emul_dev1), ranked_freq);
	int dev2_ranked_freq = DT_PROP(DT_NODELABEL(emul_dev2), ranked_freq);
	int dev1_shared_freq = DT_PROP(DT_NODELABEL(emul_dev1), shared_freq);
	int dev2_shared_freq = DT_PROP(DT_NODELABEL(emul_dev2), shared_freq);
	/* Apply invalid clock state, verify an error is returned */
	TC_PRINT("Try to apply shared clock states\n");

	ret = clock_management_set_callback(dev1_data, dev1_out, consumer_cb,
				      &consumer1_cb_data);
	zassert_equal(ret, 0, "Could not install callback");
	ret = clock_management_set_callback(dev2_data, dev2_out, consumer_cb,
				      &consumer2_cb_data);
	zassert_equal(ret, 0, "Could not install callback");


	ret = clock_management_request_state(dev1_data, dev1_ranked);
	zassert_equal(ret, 0, "Ranked state should apply correctly");
	ret = clock_management_get_rate(dev1_data, dev1_out);
	TC_PRINT("Consumer 1 ranked clock rate: %d\n", ret);
	zassert_equal(ret, dev1_ranked_freq,
		      "Consumer 1 has invalid clock rate");
	/* At this point only the first consumer should have a notification */
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	zassert_false(consumer2_cb_data.signalled,
		      "Consumer 2 should not have callback notification");

	/* Clear any old callback notifications */
	consumer1_cb_data.signalled = false;
	consumer2_cb_data.signalled = false;
	ret = clock_management_request_state(dev2_data, dev2_shared);
	zassert_equal(ret, 0,
		      "Shared state should apply correctly");
	ret = clock_management_get_rate(dev1_data, dev1_out);
	TC_PRINT("Consumer 1 shared clock rate: %d\n", ret);
	zassert_equal(ret, dev1_shared_freq,
			  "Consumer 1 has invalid clock rate");
	ret = clock_management_get_rate(dev2_data, dev2_out);
	TC_PRINT("Consumer 2 shared clock rate: %d\n", ret);
	zassert_equal(ret, dev2_shared_freq,
		      "Consumer 2 has invalid clock rate");
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	zassert_true(consumer2_cb_data.signalled,
		     "Consumer 2 should have callback notification");
	/* Clear callback for consumer 1 */
	consumer1_cb_data.signalled = false;
	/* Move consumer 2 to ranked clock state */
	ret = clock_management_request_state(dev2_data, dev2_ranked);
	zassert_equal(ret, 0,
		      "Ranked state should apply correctly");
	ret = clock_management_get_rate(dev2_data, dev2_out);
	TC_PRINT("Consumer 2 ranked clock rate: %d\n", ret);
	zassert_equal(ret, dev2_ranked_freq,
		      "Consumer 2 has invalid clock rate");
	/* Consumer 1 should have been notified and should be back at original frequency */
	zassert_true(consumer1_cb_data.signalled,
		     "Consumer 1 should have callback notification");
	ret = clock_management_get_rate(dev1_data, dev1_out);
	TC_PRINT("Consumer 1 ranked clock rate: %d\n", ret);
	zassert_equal(ret, dev1_ranked_freq,
		      "Consumer 1 has invalid clock rate");
}

ZTEST(clock_management_api, test_locking)
{
	int ret;

	/* Set first consumer to default clock state */
	ret = clock_management_request_state(dev1_data, dev1_default);
	zassert_equal(ret, 0, "Failed to apply default clock management state");
	/* Lock first consumer's clock */
	ret = clock_management_lock(dev1_data, dev1_out);
	zassert_equal(ret, 0, "Failed to lock clock");
	/* Try to set second consumer to locked clock state */
	ret = clock_management_request_state(dev2_data, dev2_locked);
	zassert_not_equal(ret, 0, "Locked state should not apply");
	/* Unlock first consumer's clock */
	ret = clock_management_unlock(dev1_data, dev1_out);
	zassert_equal(ret, 0, "Failed to unlock clock");
	/* Try to set second consumer to locked clock state again */
	ret = clock_management_request_state(dev2_data, dev2_locked);
	zassert_equal(ret, 0, "Locked state should apply after unlock");
}

ZTEST(clock_management_api, test_setrate)
{
	clock_freq_t dev1_freq_req = DT_PROP(DT_NODELABEL(emul_dev1), freq_req_1);
	clock_freq_t dev1_expect_freq = DT_PROP(DT_NODELABEL(emul_dev1), req_freq_1);
	clock_freq_t dev2_freq_req = DT_PROP(DT_NODELABEL(emul_dev2), freq_req_1);
	clock_freq_t dev2_expect_freq = DT_PROP(DT_NODELABEL(emul_dev2), req_freq_1);
	int ret;

	/* Request frequencies for device 1 */
	ret = clock_management_req_rate(dev1_data, dev1_out, dev1_freq_req);
	TC_PRINT("Consumer 1 Requested Frequency: %d\n", ret);
	zassert_equal(ret, dev1_expect_freq, "Consumer 1 has unexpected frequency from request");
	ret = clock_management_lock(dev1_data, dev1_out);
	zassert_equal(ret, 0, "Consumer 1 should be able to lock clock");
	/* Request frequencies for device 2 */
	ret = clock_management_req_rate(dev2_data, dev2_out, dev2_freq_req);
	TC_PRINT("Consumer 2 Requested Frequency: %d\n", ret);
	zassert_equal(ret, dev2_expect_freq, "Consumer 2 has unexpected frequency from request");

	/* Unlock consumer 1's clock */
	ret = clock_management_unlock(dev1_data, dev1_out);
	zassert_equal(ret, 0, "Consumer 1 should be able to unlock clock");
}

#if DT_HAS_COMPAT_STATUS_OKAY(vnd_emul_clock_gateable)
/* Only run this test if the gateable clock is present- this is all emulated,
 * so it likely only needs to run on native_sim
 */

CLOCK_MANAGEMENT_DT_DEFINE(DT_NODELABEL(emul_dev3));
static const struct clock_management_data *dev3_data =
	CLOCK_MANAGEMENT_DT_GET(DT_NODELABEL(emul_dev3));
clock_output_t dev3_out =
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
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PHANDLE(inst, input))) \
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
	clock_management_on(dev3_data, dev3_out);
	zassert_false(clock_is_gated, "Emulated clock is in use but gated");
	/* Make sure the clock doesn't turn off now, it is in use */
	clock_management_disable_unused();
	zassert_false(clock_is_gated, "Emulated clock is in use but gated during disabled_unused");
	/* Raise reference count to clock */
	clock_management_on(dev3_data, dev3_out);
	/* Lower reference count */
	clock_management_off(dev3_data, dev3_out);
	zassert_false(clock_is_gated, "Emulated clock should not gate, one reference still exists");
	/* Turn off the clock */
	clock_management_off(dev3_data, dev3_out);
	zassert_true(clock_is_gated, "Emulated clock is off but did not gate");
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(vnd_emul_clock_gateable) */

ZTEST_SUITE(clock_management_api, NULL, NULL, reset_clock_states, NULL, NULL);
