/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/ztest.h>

/*
 * Conversion-math tests for the counter.h time/ticks helpers. The helpers
 * only dereference dev->config (frequency, max top value) and dev->api, so
 * hand-built devices with a null-op API are sufficient and no counter
 * hardware or devicetree instance is involved.
 */

static DEVICE_API(counter, fake_counter_api);

#define FAKE_COUNTER_DEVICE(_name, _freq)                                                          \
	static const struct counter_config_info _name##_config = {                                 \
		.freq = (_freq),                                                                   \
		.max_top_value = UINT32_MAX,                                                       \
	};                                                                                         \
	static const struct device _name = {                                                       \
		.name = STRINGIFY(_name),                                                          \
		.config = &_name##_config,                                                         \
		.api = &fake_counter_api,                                                          \
	}

FAKE_COUNTER_DEVICE(dev_freq_zero, 0);
FAKE_COUNTER_DEVICE(dev_freq_32k, 32768);
FAKE_COUNTER_DEVICE(dev_freq_1m, 1000000);
FAKE_COUNTER_DEVICE(dev_freq_80m, 80000000);
FAKE_COUNTER_DEVICE(dev_freq_1g, 1000000000);

ZTEST(counter_conversions, test_us_to_ticks_exact)
{
	zassert_equal(counter_us_to_ticks(&dev_freq_32k, 0), 0U);
	zassert_equal(counter_us_to_ticks(&dev_freq_32k, USEC_PER_SEC), 32768U);
	/* Truncation, not rounding: 999999 us * 32768 / 1e6 = 32767.96... */
	zassert_equal(counter_us_to_ticks(&dev_freq_32k, USEC_PER_SEC - 1), 32767U);
	/* Sub-second remainder: 1.5 s * 32768 = 49152. */
	zassert_equal(counter_us_to_ticks(&dev_freq_32k, 3 * USEC_PER_SEC / 2), 49152U);
	zassert_equal(counter_us_to_ticks(&dev_freq_1m, 123456U), 123456U);
	/* Result equal to UINT32_MAX is exact, not saturated. */
	zassert_equal(counter_us_to_ticks(&dev_freq_1m, UINT32_MAX), UINT32_MAX);
}

ZTEST(counter_conversions, test_us_to_ticks_saturation)
{
	/* True result 4294967360 just exceeds UINT32_MAX; no intermediate wrap. */
	zassert_equal(counter_us_to_ticks(&dev_freq_80m, 53687092ULL), UINT32_MAX);
	/*
	 * 230584300922 us * 80 MHz wraps the uint64_t intermediate product,
	 * making the unfixed single multiply-then-divide return 50 instead of
	 * the documented saturation value.
	 */
	zassert_equal(counter_us_to_ticks(&dev_freq_80m, 230584300922ULL), UINT32_MAX);
	zassert_equal(counter_us_to_ticks(&dev_freq_80m, UINT64_MAX), UINT32_MAX);
	zassert_equal(counter_us_to_ticks(&dev_freq_32k, UINT64_MAX), UINT32_MAX);
}

ZTEST(counter_conversions, test_ns_to_ticks_exact)
{
	zassert_equal(counter_ns_to_ticks(&dev_freq_1g, 0), 0U);
	zassert_equal(counter_ns_to_ticks(&dev_freq_1g, 123456789ULL), 123456789U);
	/* Truncation, not rounding: 1500 ns at 1 MHz is 1.5 ticks. */
	zassert_equal(counter_ns_to_ticks(&dev_freq_1m, 1500ULL), 1U);
	zassert_equal(counter_ns_to_ticks(&dev_freq_1m, 999ULL), 0U);
	zassert_equal(counter_ns_to_ticks(&dev_freq_32k, NSEC_PER_SEC), 32768U);
	/* Result equal to UINT32_MAX is exact, not saturated. */
	zassert_equal(counter_ns_to_ticks(&dev_freq_1g, UINT32_MAX), UINT32_MAX);
}

ZTEST(counter_conversions, test_ns_to_ticks_saturation)
{
	/* True result 2^32 just exceeds UINT32_MAX; no intermediate wrap. */
	zassert_equal(counter_ns_to_ticks(&dev_freq_1g, 4294967296ULL), UINT32_MAX);
	/*
	 * 18446744074 ns * 1 GHz wraps the uint64_t intermediate product,
	 * making the unfixed single multiply-then-divide return 0 instead of
	 * the documented saturation value.
	 */
	zassert_equal(counter_ns_to_ticks(&dev_freq_1g, 18446744074ULL), UINT32_MAX);
	zassert_equal(counter_ns_to_ticks(&dev_freq_1g, UINT64_MAX), UINT32_MAX);
}

ZTEST(counter_conversions, test_zero_freq)
{
	/* Zero frequency converts to zero ticks and must not divide by zero. */
	zassert_equal(counter_us_to_ticks(&dev_freq_zero, USEC_PER_SEC), 0U);
	zassert_equal(counter_ns_to_ticks(&dev_freq_zero, NSEC_PER_SEC), 0U);
	zassert_equal(counter_us_to_ticks(&dev_freq_zero, UINT64_MAX), 0U);
	zassert_equal(counter_ns_to_ticks(&dev_freq_zero, UINT64_MAX), 0U);
}

ZTEST(counter_conversions, test_64bit_variants_unchanged)
{
	/* The _64 variants were fixed by commit 27b346b1187 and must not regress. */
	zassert_equal(counter_us_to_ticks_64(&dev_freq_32k, 3 * USEC_PER_SEC / 2), 49152ULL);
	zassert_equal(counter_us_to_ticks_64(&dev_freq_80m, 230584300922ULL), 18446744073760ULL);
	zassert_equal(counter_ns_to_ticks_64(&dev_freq_1g, 18446744074ULL), 18446744074ULL);
	zassert_equal(counter_ticks_to_us_64(&dev_freq_32k, 32768ULL), USEC_PER_SEC);
}

ZTEST(counter_conversions, test_uint64_getters_no_enotsup)
{
	/*
	 * uint64_t-returning getters must degrade to the 32-bit values when
	 * the wide config is disabled instead of smuggling -ENOTSUP through
	 * the unsigned return type as 0xFFFFFFFFFFFFFF8A.
	 */
	zassert_equal(counter_get_frequency_64(&dev_freq_32k), 32768ULL);
	zassert_equal(counter_get_max_top_value_64(&dev_freq_32k), (uint64_t)UINT32_MAX);
	zassert_equal(counter_get_guard_period_64(&dev_freq_32k, 0), 0ULL);
}

#ifdef CONFIG_COUNTER_64BITS_FREQ
FAKE_COUNTER_DEVICE(dev_freq_5g, 5000000000ULL);

ZTEST(counter_conversions, test_freq_above_32bit)
{
	zassert_equal(counter_ns_to_ticks(&dev_freq_5g, 100ULL), 500U);
	zassert_equal(counter_us_to_ticks(&dev_freq_5g, 100ULL), 500000U);
	/* True result 5e9 exceeds UINT32_MAX; no intermediate wrap. */
	zassert_equal(counter_us_to_ticks(&dev_freq_5g, USEC_PER_SEC), UINT32_MAX);
	/*
	 * 4 s * 5 GHz wraps the uint64_t intermediate product to a
	 * plausible-looking 1553255926 instead of the documented saturation.
	 */
	zassert_equal(counter_ns_to_ticks(&dev_freq_5g, 4ULL * NSEC_PER_SEC), UINT32_MAX);
	zassert_equal(counter_get_frequency_64(&dev_freq_5g), 5000000000ULL);
}
#endif /* CONFIG_COUNTER_64BITS_FREQ */

ZTEST_SUITE(counter_conversions, NULL, NULL, NULL, NULL, NULL);
