/*
 * Copyright (c) 2025 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "trigger_src.h"

#define NUM_OF_CONTINUOUS_CAPTURES CONFIG_NUM_OF_CONTINUOUS_CAPTURES
#define CAPTURE_TOLERANCE_US       CONFIG_COUNTER_CAPTURE_TOLERANCE_US
#define EDGE_RISING                1
#define EDGE_FALLING               0
#define CAPTURE_COUNT \
	DT_PROP_LEN(DT_NODELABEL(counter_loopback_0), test_counter_captures)

/* clang-format off */
const struct counter_capture_dt_spec capture_dt_specs[] = {
	DT_FOREACH_PROP_ELEM_SEP(
		DT_NODELABEL(counter_loopback_0),
		test_counter_captures,
		COUNTER_CAPTURE_DT_SPEC_GET_BY_IDX, (,))
};
/* clang-format on */

K_SEM_DEFINE(capture_sem, 0, 1);

static struct {
	uint32_t expected_ticks;
	bool timing_ok;
} capture_ctx;

void capture_cb(const struct device *dev, uint8_t chan, counter_capture_flags_t flags,
		uint32_t ticks, void *user_data)
{
	uint32_t tolerance_ticks = counter_us_to_ticks(dev, CAPTURE_TOLERANCE_US);
	uint32_t diff = (ticks >= capture_ctx.expected_ticks)
				? (ticks - capture_ctx.expected_ticks)
				: (capture_ctx.expected_ticks - ticks);

	capture_ctx.timing_ok = (diff <= tolerance_ticks);

	TC_PRINT("Capture callback: channel %d, flags %d, ticks %u (%lluus), "
		 "expected ~%u, diff %u ticks (%lluus) [%s]\n",
		 chan, flags, ticks, counter_ticks_to_us(dev, ticks),
		 capture_ctx.expected_ticks, diff, counter_ticks_to_us(dev, diff),
		 capture_ctx.timing_ok ? "PASS" : "FAIL");

	k_sem_give(&capture_sem);
}

static int counter_capture_test_edge(size_t idx, int trigger_level)
{
	int ret;

	if (trigger_src_get(idx) == trigger_level) {
		ret = trigger_src_set(idx, !trigger_level);
		zassert_ok(ret, "idx %zu: failed to set trigger source to settle state", idx);
		k_msleep(2);
	}

	ret = counter_get_value(capture_dt_specs[idx].dev, &capture_ctx.expected_ticks);
	zassert_ok(ret, "idx %zu: failed to get counter value", idx);

	ret = trigger_src_set(idx, trigger_level);
	zassert_ok(ret, "idx %zu: failed to drive trigger source", idx);

	ret = k_sem_take(&capture_sem, K_MSEC(100));
	if (ret != 0) {
		return ret;
	}

	zassert_true(capture_ctx.timing_ok,
		     "idx %zu: capture timestamp outside tolerance (%d us)",
		     idx, CAPTURE_TOLERANCE_US);

	ret = trigger_src_get(idx);
	zassert_equal(ret, trigger_level,
		      "idx %zu: trigger source state does not match expected value of %d: %d",
		      idx, trigger_level, ret);

	return 0;
}

/***************************************************************
 * Per-channel test runners
 ***************************************************************/

static void run_rising_edge_continuous(size_t idx)
{
	const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];
	int ret;

	ret = counter_capture_configure(spec->dev, spec->chan_id,
					COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_CONTINUOUS,
					capture_cb, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "idx %zu channel %u: failed to configure capture", idx, spec->chan_id);

	zassert_ok(counter_enable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

	for (uint8_t i = 0; i < NUM_OF_CONTINUOUS_CAPTURES; i++) {
		ret = counter_capture_test_edge(idx, EDGE_RISING);
		zassert_ok(ret, "idx %zu channel %u: rising edge capture test failed", idx,
			   spec->chan_id);
	}

	zassert_ok(counter_disable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to disable capture", idx, spec->chan_id);
}

static void run_rising_edge_single(size_t idx)
{
	const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];
	int ret;

	ret = counter_capture_configure(spec->dev, spec->chan_id,
					COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_SINGLE_SHOT,
					capture_cb, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "idx %zu channel %u: failed to configure capture", idx, spec->chan_id);

	zassert_ok(counter_enable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

	ret = counter_capture_test_edge(idx, EDGE_RISING);
	zassert_ok(ret, "idx %zu channel %u: rising edge capture test failed", idx, spec->chan_id);

	ret = counter_capture_test_edge(idx, EDGE_RISING);
	zassert_equal(ret, -EAGAIN,
		      "idx %zu channel %u: capture callback should not be called after "
		      "single shot capture",
		      idx, spec->chan_id);

	zassert_ok(counter_disable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to disable capture", idx, spec->chan_id);
}

static void run_falling_edge_continuous(size_t idx)
{
	const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];
	int ret;

	ret = counter_capture_configure(spec->dev, spec->chan_id,
					COUNTER_CAPTURE_FALLING_EDGE | COUNTER_CAPTURE_CONTINUOUS,
					capture_cb, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "idx %zu channel %u: failed to configure capture", idx, spec->chan_id);

	zassert_ok(counter_enable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

	for (uint8_t i = 0; i < NUM_OF_CONTINUOUS_CAPTURES; i++) {
		ret = counter_capture_test_edge(idx, EDGE_FALLING);
		zassert_ok(ret, "idx %zu channel %u: falling edge capture test failed", idx,
			   spec->chan_id);
	}

	zassert_ok(counter_disable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to disable capture", idx, spec->chan_id);
}

static void run_falling_edge_single(size_t idx)
{
	const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];
	int ret;

	ret = counter_capture_configure(spec->dev, spec->chan_id,
					COUNTER_CAPTURE_FALLING_EDGE | COUNTER_CAPTURE_SINGLE_SHOT,
					capture_cb, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "idx %zu channel %u: failed to configure capture", idx, spec->chan_id);

	zassert_ok(counter_enable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

	ret = counter_capture_test_edge(idx, EDGE_FALLING);
	zassert_ok(ret, "idx %zu channel %u: falling edge capture test failed", idx,
		   spec->chan_id);

	ret = counter_capture_test_edge(idx, EDGE_FALLING);
	zassert_equal(ret, -EAGAIN,
		      "idx %zu channel %u: capture callback should not be called after "
		      "single shot capture",
		      idx, spec->chan_id);

	zassert_ok(counter_disable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to disable capture", idx, spec->chan_id);
}

static void run_both_edges_continuous(size_t idx)
{
	const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];
	int ret;

	ret = counter_capture_configure(spec->dev, spec->chan_id,
					COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_CONTINUOUS,
					capture_cb, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "idx %zu channel %u: failed to configure capture", idx, spec->chan_id);

	zassert_ok(counter_enable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

	int first_edge = (trigger_src_get(idx) == EDGE_FALLING)
				 ? EDGE_RISING
				 : EDGE_FALLING;

	for (uint8_t i = 0; i < NUM_OF_CONTINUOUS_CAPTURES; i++) {
		ret = counter_capture_test_edge(idx, first_edge);
		zassert_ok(ret, "idx %zu channel %u: edge capture test failed", idx,
			   spec->chan_id);

		ret = counter_capture_test_edge(
			idx, first_edge == EDGE_RISING ? EDGE_FALLING : EDGE_RISING);
		zassert_ok(ret, "idx %zu channel %u: edge capture test failed", idx,
			   spec->chan_id);
	}

	zassert_ok(counter_disable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to disable capture", idx, spec->chan_id);
}

static void run_both_edges_single(size_t idx)
{
	const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];
	int ret;

	ret = counter_capture_configure(spec->dev, spec->chan_id,
					COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_SINGLE_SHOT,
					capture_cb, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_ok(ret, "idx %zu channel %u: failed to configure capture", idx, spec->chan_id);

	zassert_ok(counter_enable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to enable capture", idx, spec->chan_id);

	int first_edge = (trigger_src_get(idx) == EDGE_FALLING)
				 ? EDGE_RISING
				 : EDGE_FALLING;

	ret = counter_capture_test_edge(idx, first_edge);
	zassert_ok(ret, "idx %zu channel %u: edge capture test failed", idx, spec->chan_id);

	ret = counter_capture_test_edge(
		idx, first_edge == EDGE_RISING ? EDGE_FALLING : EDGE_RISING);
	zassert_equal(ret, -EAGAIN,
		      "idx %zu channel %u: capture callback should not be called after "
		      "single shot capture",
		      idx, spec->chan_id);

	zassert_ok(counter_disable_capture(spec->dev, spec->chan_id),
		   "idx %zu channel %u: failed to disable capture", idx, spec->chan_id);
}

/***************************************************************
 * Generated per-channel ZTESTs
 ***************************************************************/

#define _DEFINE_RISING_CONTINUOUS(n, _) \
	ZTEST(counter_capture, test_rising_edge_continuous_capture_##n) \
	{ run_rising_edge_continuous(n); }

#define _DEFINE_RISING_SINGLE(n, _) \
	ZTEST(counter_capture, test_rising_edge_single_capture_##n) \
	{ run_rising_edge_single(n); }

#define _DEFINE_FALLING_CONTINUOUS(n, _) \
	ZTEST(counter_capture, test_falling_edge_continuous_capture_##n) \
	{ run_falling_edge_continuous(n); }

#define _DEFINE_FALLING_SINGLE(n, _) \
	ZTEST(counter_capture, test_falling_edge_single_capture_##n) \
	{ run_falling_edge_single(n); }

#define _DEFINE_BOTH_CONTINUOUS(n, _) \
	ZTEST(counter_capture, test_both_edges_continuous_capture_##n) \
	{ run_both_edges_continuous(n); }

#define _DEFINE_BOTH_SINGLE(n, _) \
	ZTEST(counter_capture, test_both_edges_single_capture_##n) \
	{ run_both_edges_single(n); }

LISTIFY(CAPTURE_COUNT, _DEFINE_RISING_CONTINUOUS, ())
LISTIFY(CAPTURE_COUNT, _DEFINE_RISING_SINGLE, ())
LISTIFY(CAPTURE_COUNT, _DEFINE_FALLING_CONTINUOUS, ())
LISTIFY(CAPTURE_COUNT, _DEFINE_FALLING_SINGLE, ())
LISTIFY(CAPTURE_COUNT, _DEFINE_BOTH_CONTINUOUS, ())
LISTIFY(CAPTURE_COUNT, _DEFINE_BOTH_SINGLE, ())

/***************************************************************
 * Suite fixtures
 ***************************************************************/

static void counter_before(void *f)
{
	int ret;

	ARG_UNUSED(f);

	k_sem_reset(&capture_sem);
	memset(&capture_ctx, 0, sizeof(capture_ctx));

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

	for (size_t idx = 0; idx < ARRAY_SIZE(capture_dt_specs); idx++) {
		const struct counter_capture_dt_spec *spec = &capture_dt_specs[idx];

		zassert_true(device_is_ready(spec->dev),
			     "idx %zu channel %u: counter device is not ready", idx, spec->chan_id);

		ret = trigger_src_setup(idx);
		zassert_ok(ret, "idx %zu channel %u: failed to set up trigger source", idx,
			   spec->chan_id);
	}

	return NULL;
}

ZTEST_SUITE(counter_capture, NULL, counter_setup, counter_before, counter_after, NULL);
