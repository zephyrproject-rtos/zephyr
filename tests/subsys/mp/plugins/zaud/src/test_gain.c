/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/zaud/mp_zaud_gain.h>
#include <zephyr/mp/zaud/mp_zaud_property.h>

#define TEST_GAIN_ID   1
#define TEST_BUF_BYTES 256

NET_BUF_POOL_FIXED_DEFINE(test_buf_pool, 2, TEST_BUF_BYTES, sizeof(struct mp_buffer_meta), NULL);

static struct mp_zaud_gain gain;

/**
 * Push one buffer of samples through the gain element's chain function and
 * return the in-place result. @a bit_width selects which sample-width path of
 * the element is exercised.
 */
static void run_gain(uint32_t bit_width, int gain_percent, const void *in, void *out, size_t len)
{
	struct net_buf *in_buf, *out_buf = NULL;
	int ret;

	MP_ELEMENT_INIT(&gain, mp_zaud_gain_init, TEST_GAIN_ID);

	ret = mp_object_set_properties((struct mp_object *)&gain, PROP_GAIN, &gain_percent,
				       PROP_LIST_END);
	zassert_ok(ret, "failed to set gain to %d%%", gain_percent);

	/*
	 * bit_width is normally set by caps negotiation (set_caps). Set it
	 * directly so each width can be exercised without a full pipeline.
	 */
	gain.bit_width = bit_width;

	in_buf = net_buf_alloc(&test_buf_pool, K_NO_WAIT);
	zassert_not_null(in_buf, "failed to allocate test buffer");

	memcpy(net_buf_add(in_buf, len), in, len);
	mp_buffer_get_meta(in_buf)->bytes_used = len;

	ret = gain.transform.sinkpad.chainfn(&gain.transform.sinkpad, in_buf, &out_buf);
	zassert_ok(ret, "chainfn failed");
	zassert_equal(out_buf, in_buf, "gain is in-place, expected the same buffer back");

	memcpy(out, out_buf->data, len);
	net_buf_unref(in_buf);
}

ZTEST(zaud_gain, test_unity_passes_through)
{
	const int16_t in[] = {0, 1, -1, 1234, -1234, INT16_MAX, INT16_MIN};
	int16_t out[ARRAY_SIZE(in)];

	run_gain(16, 100, in, out, sizeof(in));

	zassert_mem_equal(out, in, sizeof(in), "unity gain must not modify samples");
}

ZTEST(zaud_gain, test_mute_zeroes_buffer)
{
	const int16_t in[] = {123, -456, INT16_MAX, INT16_MIN};
	const int16_t want[ARRAY_SIZE(in)] = {0};
	int16_t out[ARRAY_SIZE(in)];

	run_gain(16, 0, in, out, sizeof(in));

	zassert_mem_equal(out, want, sizeof(want), "0%% gain must mute");
}

ZTEST(zaud_gain, test_attenuation_16bit)
{
	const int16_t in[] = {1000, -1000, INT16_MAX, INT16_MIN};
	const int16_t want[] = {500, -500, 16383, -16384};
	int16_t out[ARRAY_SIZE(in)];

	run_gain(16, 50, in, out, sizeof(in));

	zassert_mem_equal(out, want, sizeof(want), "50%% gain mismatch");
}

/*
 * Regression test: with a 32-bit intermediate, a full-scale 16-bit sample
 * multiplied by a Q16.16 gain above unity overflows and wraps, so INT16_MIN at
 * 200% produced 0 (silence) instead of saturating to INT16_MIN. Amplification
 * must saturate, never wrap.
 */
ZTEST(zaud_gain, test_amplification_16bit_saturates)
{
	const int16_t in[] = {INT16_MIN, -20000, -100, 0, 100, 16384, INT16_MAX};
	const int16_t want[] = {INT16_MIN, INT16_MIN, -200, 0, 200, INT16_MAX, INT16_MAX};
	int16_t out[ARRAY_SIZE(in)];

	run_gain(16, 200, in, out, sizeof(in));

	zassert_mem_equal(out, want, sizeof(want), "200%% gain must saturate, not wrap");
}

ZTEST(zaud_gain, test_max_amplification_16bit_saturates)
{
	const int16_t in[] = {INT16_MIN, -4000, 0, 4000, INT16_MAX};
	const int16_t want[] = {INT16_MIN, INT16_MIN, 0, INT16_MAX, INT16_MAX};
	int16_t out[ARRAY_SIZE(in)];

	run_gain(16, 1000, in, out, sizeof(in));

	zassert_mem_equal(out, want, sizeof(want), "1000%% gain must saturate, not wrap");
}

ZTEST(zaud_gain, test_amplification_32bit_saturates)
{
	const int32_t in[] = {INT32_MIN, -1000, 0, 1000, INT32_MAX};
	const int32_t want[] = {INT32_MIN, -2000, 0, 2000, INT32_MAX};
	int32_t out[ARRAY_SIZE(in)];

	run_gain(32, 200, in, out, sizeof(in));

	zassert_mem_equal(out, want, sizeof(want), "32-bit 200%% gain must saturate, not wrap");
}

ZTEST(zaud_gain, test_gain_property_round_trips)
{
	int gain_percent;

	MP_ELEMENT_INIT(&gain, mp_zaud_gain_init, TEST_GAIN_ID);

	gain_percent = 250;
	zassert_ok(mp_object_set_properties((struct mp_object *)&gain, PROP_GAIN, &gain_percent,
					    PROP_LIST_END));

	gain_percent = 0;
	zassert_ok(mp_object_get_properties((struct mp_object *)&gain, PROP_GAIN, &gain_percent,
					    PROP_LIST_END));
	zassert_equal(gain_percent, 250, "gain property should round-trip");
}

/*
 * An out-of-range gain is clamped to GAIN_PERCENT_MAX (1000%) internally, so
 * the resulting samples must match what 1000% produces rather than overflowing
 * the Q16.16 conversion.
 */
ZTEST(zaud_gain, test_out_of_range_gain_is_clamped)
{
	const int16_t in[] = {INT16_MIN, -4000, 0, 4000, INT16_MAX};
	const int16_t want[] = {INT16_MIN, INT16_MIN, 0, INT16_MAX, INT16_MAX};
	int16_t out[ARRAY_SIZE(in)];

	run_gain(16, 100000, in, out, sizeof(in));

	zassert_mem_equal(out, want, sizeof(want), "out-of-range gain must clamp to max");
}

ZTEST_SUITE(zaud_gain, NULL, NULL, NULL, NULL, NULL);
