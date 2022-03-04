/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <ztest.h>
#include <strings.h>

/**
 * @addtogroup t_driver_can
 * @{
 * @defgroup t_can_timing test_can_timing
 * @}
 */

/**
 * @brief Allowed sample point calculation margin in permille.
 */
#define SAMPLE_POINT_MARGIN 100

/**
 * @brief Defines a set of CAN timing test values
 */
struct can_timing_test {
	/** Desired bitrate in bits/s */
	uint32_t bitrate;
	/** Desired sample point in permille */
	uint16_t sp;
	/** Do these values represent an invalid CAN timing? */
	bool invalid;
};

/**
 * @brief List of CAN timing values to test.
 */
static const struct can_timing_test can_timing_tests[] = {
	/** Standard bitrates. */
	{  125000, 875, false },
	{  500000, 875, false },
	{ 1000000, 875, false },
	/** Additional, valid sample points. */
	{  125000, 900, false },
	{  125000, 800, false },
	/** Valid bitrate, invalid sample point. */
	{  125000, 1000, true },
#ifdef CONFIG_CAN_FD_MODE
	/** Invalid CAN-FD bitrate, valid sample point. */
	{ 8000000 + 1, 875, true },
#else /* CONFIG_CAN_FD_MODE */
	/** Invalid classical bitrate, valid sample point. */
	{ 1000000 + 1, 875, true },
#endif /* CONFIG_CAN_FD_MODE */
};

/**
 * @brief CAN timing test fixture
 */
struct can_timing_tests_fixture {
	/** CAN device. */
	const struct device *dev;
	/** List of CAN timing test values. */
	const struct can_timing_test *tests;
	/** Number of CAN timing test list entries. */
	const size_t test_count;
};

/**
 * @brief CAN timing test fixture instance common to all test cases
 */
static struct can_timing_tests_fixture test_fixture = {
	.dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.tests = can_timing_tests,
	.test_count = ARRAY_SIZE(can_timing_tests),
};

/**
 * @brief CAN timing test setup function
 *
 * Asserts that the CAN device is ready before starting tests.
 *
 * @return Pointer to CAN timing test fixture instance
 */
static void *can_timing_test_setup(void)
{
	zassert_true(device_is_ready(test_fixture.dev), "CAN device not ready");
	printk("testing on device %s\n", test_fixture.dev->name);

	return &test_fixture;
}

ZTEST_SUITE(can_timing_tests, NULL, can_timing_test_setup, NULL, NULL, NULL);

/**
 * @brief Assert that a CAN timing struct matches the specified bitrate
 *
 * Assert that the values of a CAN timing struct matches the specified bitrate
 * for a given CAN controller device instance.
 *
 * @param dev pointer to the device structure for the driver instance
 * @param timing pointer to the CAN timing struct
 * @param bitrate the CAN bitrate in bits/s
 */
static void assert_bitrate_correct(const struct device *dev, struct can_timing *timing,
				   uint32_t bitrate)
{
	const uint32_t ts = 1 + timing->prop_seg + timing->phase_seg1 + timing->phase_seg2;
	uint32_t core_clock;
	uint32_t bitrate_calc;
	int err;

	zassert_not_equal(timing->prescaler, 0, "prescaler is zero");

	err = can_get_core_clock(dev, &core_clock);
	zassert_equal(err, 0, "failed to get core CAN clock");

	bitrate_calc = core_clock / timing->prescaler / ts;
	zassert_equal(bitrate, bitrate_calc, "bitrate mismatch");
}

/**
 * @brief Assert that a CAN timing struct is within the bounds
 *
 * Assert that the values of a CAN timing struct are within the bounds for a
 * given CAN controller device instance.
 *
 * @param dev pointer to the device structure for the driver instance
 * @param timing pointer to the CAN timing struct
 */
static void assert_timing_within_bounds(const struct device *dev, struct can_timing *timing)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;
	const struct can_timing *max = &api->timing_max;
	const struct can_timing *min = &api->timing_min;

	zassert_true(timing->prop_seg <= max->prop_seg, "prop_seg exceeds max");
	zassert_true(timing->phase_seg1 <= max->phase_seg1, "phase_seg1 exceeds max");
	zassert_true(timing->phase_seg2 <= max->phase_seg2, "phase_seg2 exceeds max");

	zassert_true(timing->prop_seg >= min->prop_seg, "prop_seg lower than min");
	zassert_true(timing->phase_seg1 >= min->phase_seg1, "phase_seg1 lower than min");
	zassert_true(timing->phase_seg2 >= min->phase_seg2, "phase_seg2 lower than min");
}

/**
 * @brief Assert that a sample point is within a specified margin
 *
 * Assert that values of a CAN timing struct results in a specified sample point
 * within a given margin.
 *
 * @param timing pointer to the CAN timing struct
 * @param sp sample point in permille
 * @param sp_margin sample point margin in permille
 */
static void assert_sp_within_margin(struct can_timing *timing, uint16_t sp, uint16_t sp_margin)
{
	const uint32_t ts = 1 + timing->prop_seg + timing->phase_seg1 + timing->phase_seg2;
	const uint16_t sp_calc = ((1 + timing->prop_seg + timing->phase_seg1) * 1000) / ts;

	zassert_within(sp, sp_calc, sp_margin,
		       "sample point %d not within calculated sample point %d +/- %d",
		       sp, sp_calc, sp_margin);
}

/**
 * @brief Test a set of CAN timing values
 *
 * Test a set of CAN timing values on a specified CAN controller device
 * instance.
 *
 * @param dev pointer to the device structure for the driver instance
 * @param test pointer to the set of CAN timing values
 */
static void test_timing_values(const struct device *dev, const struct can_timing_test *test)
{
	struct can_timing timing = { 0 };
	int err;

	printk("testing bitrate %u, sample point %u.%u%% (%s): ",
		test->bitrate, test->sp / 10, test->sp % 10, test->invalid ? "invalid" : "valid");

	err = can_calc_timing(dev, &timing, test->bitrate, test->sp);
	if (test->invalid) {
		zassert_equal(err, -EINVAL, "err %d, expected -EINVAL", err);
		printk("OK\n");
	} else {
		zassert_true(err >= 0, "unknown error %d", err);
		zassert_true(err <= SAMPLE_POINT_MARGIN, "sample point error %d too large", err);

		assert_bitrate_correct(dev, &timing, test->bitrate);
		assert_timing_within_bounds(dev, &timing);
		assert_sp_within_margin(&timing, test->sp, SAMPLE_POINT_MARGIN);

		printk("OK, sample point error %d.%d%%\n", err / 10, err % 10);
	}
}

/**
 * @brief Test all CAN timing values
 *
 * @param this Pointer to a CAN timing test fixture
 */
ZTEST_F(can_timing_tests, test_timing)
{
	int i;

	for (i = 0; i < this->test_count; i++) {
		test_timing_values(this->dev, &this->tests[i]);
	}
}
