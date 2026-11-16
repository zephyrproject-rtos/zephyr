/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <math.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>

#define RATIO_TOLERANCE 1e-12

static int fake_ret;
static int fake_calls;
static int64_t fake_scaled_ppm;
static double fake_ratio;

static int fake_set(const struct device *dev __unused, struct net_ptp_time *tm __unused)
{
	return 0;
}

static int fake_get(const struct device *dev __unused, struct net_ptp_time *tm __unused)
{
	return 0;
}

static int fake_adjust(const struct device *dev __unused, int increment __unused)
{
	return 0;
}

static int fake_adjust_rate(const struct device *dev __unused, int64_t scaled_ppm)
{
	fake_calls++;
	fake_scaled_ppm = scaled_ppm;

	return fake_ret;
}

static int fake_rate_adjust(const struct device *dev __unused, double ratio)
{
	fake_calls++;
	fake_ratio = ratio;

	return fake_ret;
}

static DEVICE_API(ptp_clock, scaled_ppm_api) = {
	.set = fake_set,
	.get = fake_get,
	.adjust = fake_adjust,
	.adjust_rate = fake_adjust_rate,
};

/* The rate ratio operation is kept under test for as long as it is supported. */
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)

static DEVICE_API(ptp_clock, ratio_api) = {
	.set = fake_set,
	.get = fake_get,
	.adjust = fake_adjust,
	.rate_adjust = fake_rate_adjust,
};

static int rate_adjust(const struct device *dev, double ratio)
{
	return ptp_clock_rate_adjust(dev, ratio);
}

TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)

DEVICE_DEFINE(scaled_ppm_clock, "scaled_ppm_clock", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &scaled_ppm_api);
DEVICE_DEFINE(ratio_clock, "ratio_clock", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ratio_api);

static const struct device *const scaled_ppm_dev = DEVICE_GET(scaled_ppm_clock);
static const struct device *const ratio_dev = DEVICE_GET(ratio_clock);

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	fake_ret = 0;
	fake_calls = 0;
	fake_scaled_ppm = INT64_MIN;
	fake_ratio = 0.0;
}

ZTEST(ptp_clock_api, test_adjust_rate_passes_scaled_ppm)
{
	zassert_ok(ptp_clock_adjust_rate(scaled_ppm_dev, 3 * PTP_CLOCK_SCALED_PPM_ONE + 1));
	zassert_equal(fake_scaled_ppm, 3 * PTP_CLOCK_SCALED_PPM_ONE + 1);

	zassert_ok(ptp_clock_adjust_rate(scaled_ppm_dev, INT64_MIN + 1));
	zassert_equal(fake_scaled_ppm, INT64_MIN + 1);

	zassert_equal(fake_calls, 2);
}

ZTEST(ptp_clock_api, test_adjust_rate_converts_to_ratio)
{
	zassert_ok(ptp_clock_adjust_rate(ratio_dev, 2 * PTP_CLOCK_SCALED_PPM_ONE));
	zassert_within(fake_ratio, 1.000002, RATIO_TOLERANCE);

	zassert_ok(ptp_clock_adjust_rate(ratio_dev, 0));
	zassert_equal(fake_ratio, 1.0);

	zassert_ok(ptp_clock_adjust_rate(ratio_dev, -(3 * PTP_CLOCK_SCALED_PPM_ONE / 2)));
	zassert_within(fake_ratio, 0.9999985, RATIO_TOLERANCE);

	zassert_equal(fake_calls, 3);
}

ZTEST(ptp_clock_api, test_rate_adjust_passes_ratio)
{
	zassert_ok(rate_adjust(ratio_dev, 1.0001));
	zassert_equal(fake_ratio, 1.0001);
	zassert_equal(fake_calls, 1);
}

ZTEST(ptp_clock_api, test_rate_adjust_converts_to_scaled_ppm)
{
	zassert_ok(rate_adjust(scaled_ppm_dev, 1.0));
	zassert_equal(fake_scaled_ppm, 0);

	zassert_ok(rate_adjust(scaled_ppm_dev, 1.000002));
	zassert_within(fake_scaled_ppm, 2 * PTP_CLOCK_SCALED_PPM_ONE, 1);

	zassert_ok(rate_adjust(scaled_ppm_dev, 0.9999985));
	zassert_within(fake_scaled_ppm, -(3 * PTP_CLOCK_SCALED_PPM_ONE / 2), 1);

	/* 1 ppb is 65.536 in scaled ppm and rounds to nearest. */
	zassert_ok(rate_adjust(scaled_ppm_dev, 1.000000001));
	zassert_equal(fake_scaled_ppm, 66);

	zassert_ok(rate_adjust(scaled_ppm_dev, 0.999999999));
	zassert_equal(fake_scaled_ppm, -66);

	zassert_equal(fake_calls, 5);
}

ZTEST(ptp_clock_api, test_rate_adjust_rejects_unrepresentable_ratio)
{
	zassert_equal(rate_adjust(scaled_ppm_dev, 1e30), -ERANGE);
	zassert_equal(rate_adjust(scaled_ppm_dev, -1e30), -ERANGE);
	zassert_equal(rate_adjust(scaled_ppm_dev, (double)NAN), -ERANGE);
	zassert_equal(rate_adjust(scaled_ppm_dev, (double)INFINITY), -ERANGE);
	zassert_equal(fake_calls, 0);
}

ZTEST(ptp_clock_api, test_driver_error_is_returned)
{
	fake_ret = -EIO;

	zassert_equal(ptp_clock_adjust_rate(scaled_ppm_dev, 0), -EIO);
	zassert_equal(ptp_clock_adjust_rate(ratio_dev, 0), -EIO);
	zassert_equal(rate_adjust(scaled_ppm_dev, 1.0), -EIO);
	zassert_equal(rate_adjust(ratio_dev, 1.0), -EIO);
	zassert_equal(fake_calls, 4);
}

ZTEST(ptp_clock_api, test_scaled_ppm_to_ppb)
{
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(0), 0);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(PTP_CLOCK_SCALED_PPM_ONE), 1000);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(-PTP_CLOCK_SCALED_PPM_ONE), -1000);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(100000 * PTP_CLOCK_SCALED_PPM_ONE), 100000000);

	/* Rounds to nearest, 1 ppb is 65.536 in scaled ppm. */
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(32), 0);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(33), 1);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(98), 1);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(99), 2);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(-32), 0);
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(-33), -1);

	/* Does not overflow. */
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(INT64_MAX), INT64_C(140737488355328000));
	zassert_equal(ptp_clock_scaled_ppm_to_ppb(-INT64_MAX), -INT64_C(140737488355328000));
}

ZTEST(ptp_clock_api, test_scaled_ppm_to_ppb_restores_ppb)
{
	/* A ppb value truncated to scaled ppm converts back to the same ppb value. */
	for (int64_t ppb = -100000; ppb <= 100000; ppb++) {
		int64_t scaled_ppm = ppb * PTP_CLOCK_SCALED_PPM_ONE / 1000;

		zassert_equal(ptp_clock_scaled_ppm_to_ppb(scaled_ppm), ppb, "ppb %lld",
			      (long long)ppb);
	}
}

ZTEST(ptp_clock_api, test_adjust_by_scaled_ppm)
{
	const int64_t full = 1000000 * PTP_CLOCK_SCALED_PPM_ONE;
	uint32_t result = 0U;

	zassert_ok(ptp_clock_adjust_by_scaled_ppm(0x80000000U, 0, &result));
	zassert_equal(result, 0x80000000U);

	zassert_ok(ptp_clock_adjust_by_scaled_ppm(1000000U, PTP_CLOCK_SCALED_PPM_ONE, &result));
	zassert_equal(result, 1000001U);

	zassert_ok(ptp_clock_adjust_by_scaled_ppm(1000000U, -PTP_CLOCK_SCALED_PPM_ONE, &result));
	zassert_equal(result, 999999U);

	/* The difference to the nominal value is rounded to nearest, half away from zero. */
	zassert_ok(ptp_clock_adjust_by_scaled_ppm(3U, full / 2, &result));
	zassert_equal(result, 5U);
	zassert_ok(ptp_clock_adjust_by_scaled_ppm(3U, -(full / 2), &result));
	zassert_equal(result, 1U);
	zassert_ok(ptp_clock_adjust_by_scaled_ppm(3U, full / 2 - 1, &result));
	zassert_equal(result, 4U);
	zassert_ok(ptp_clock_adjust_by_scaled_ppm(UINT32_MAX, 1, &result));
	zassert_equal(result, UINT32_MAX);

	/* Limits of the accepted range */
	zassert_ok(ptp_clock_adjust_by_scaled_ppm(0x7fffffffU, full, &result));
	zassert_equal(result, 0xfffffffeU);
	zassert_ok(ptp_clock_adjust_by_scaled_ppm(UINT32_MAX, -full, &result));
	zassert_equal(result, 0U);
}

ZTEST(ptp_clock_api, test_adjust_by_scaled_ppm_rejects_out_of_range)
{
	const int64_t full = 1000000 * PTP_CLOCK_SCALED_PPM_ONE;
	uint32_t result = 1234U;

	zassert_equal(ptp_clock_adjust_by_scaled_ppm(1U, full + 1, &result), -ERANGE);
	zassert_equal(ptp_clock_adjust_by_scaled_ppm(1U, -full - 1, &result), -ERANGE);
	zassert_equal(ptp_clock_adjust_by_scaled_ppm(1U, INT64_MAX, &result), -ERANGE);
	zassert_equal(ptp_clock_adjust_by_scaled_ppm(1U, INT64_MIN, &result), -ERANGE);

	/* Result does not fit in 32 bits */
	zassert_equal(ptp_clock_adjust_by_scaled_ppm(0x80000000U, full, &result), -ERANGE);
	zassert_equal(ptp_clock_adjust_by_scaled_ppm(UINT32_MAX, PTP_CLOCK_SCALED_PPM_ONE, &result),
		      -ERANGE);

	zassert_equal(result, 1234U);
}

ZTEST(ptp_clock_api, test_adjust_by_scaled_ppm_matches_ratio)
{
	static const uint32_t bases[] = {1U, 12345U, 0x20000000U, 0x80000000U, 0xe0000000U};
	static const int64_t offsets[] = {
		1,
		65,
		PTP_CLOCK_SCALED_PPM_ONE,
		100 * PTP_CLOCK_SCALED_PPM_ONE + 12345,
		50000 * PTP_CLOCK_SCALED_PPM_ONE + 4194303,
		4194304,
	};

	ARRAY_FOR_EACH(bases, i) {
		ARRAY_FOR_EACH(offsets, j) {
			for (int sign = -1; sign <= 1; sign += 2) {
				int64_t scaled_ppm = sign * offsets[j];
				double ratio = 1.0 + (double)scaled_ppm /
						     (1000000.0 * PTP_CLOCK_SCALED_PPM_ONE);
				double expected = (double)bases[i] * ratio;
				uint32_t result = 0U;

				zassert_ok(ptp_clock_adjust_by_scaled_ppm(bases[i], scaled_ppm,
									  &result));
				zassert_within((double)result, expected, 0.500001,
					       "base %u scaled_ppm %lld", bases[i],
					       (long long)scaled_ppm);
			}
		}
	}
}

ZTEST_SUITE(ptp_clock_api, NULL, NULL, before, NULL, NULL);
