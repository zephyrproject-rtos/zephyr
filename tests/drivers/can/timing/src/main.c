/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>
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
#define SAMPLE_POINT_MARGIN 50

/**
 * @brief Defines a set of CAN timing test values
 */
struct can_timing_test {
	/** Bitrate in bit/s */
	uint32_t bitrate;
	/** Desired sample point in permille */
	uint16_t sp;
};

/**
 * @brief List of CAN timing values to test.
 */
static const struct can_timing_test can_timing_tests[] = {
	/** Standard bitrates. */
	{   20000, 875 },
	{   50000, 875 },
	{  125000, 875 },
	{  250000, 875 },
	{  500000, 875 },
	{  800000, 800 },
	{ 1000000, 750 },
};

/**
 * @brief List of CAN FD data phase timing values to test.
 */
static const struct can_timing_test can_timing_data_tests[] = {
	/** Standard bitrates. */
	{  500000, 875 },
	{ 1000000, 750 },
};

/**
 * @brief Assert that a CAN timing struct matches the specified bitrate
 *
 * Assert that the values of a CAN timing struct matches the specified bitrate
 * for a given CAN controller device instance.
 *
 * @param dev pointer to the device structure for the driver instance
 * @param timing pointer to the CAN timing struct
 * @param bitrate the CAN bitrate in bit/s
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
static void assert_timing_within_bounds(struct can_timing *timing,
					const struct can_timing *min,
					const struct can_timing *max)
{
	zassert_true(timing->sjw <= max->sjw, "sjw exceeds max");
	zassert_true(timing->prop_seg <= max->prop_seg, "prop_seg exceeds max");
	zassert_true(timing->phase_seg1 <= max->phase_seg1, "phase_seg1 exceeds max");
	zassert_true(timing->phase_seg2 <= max->phase_seg2, "phase_seg2 exceeds max");
	zassert_true(timing->prescaler <= max->prescaler, "prescaler exceeds max");

	zassert_true(timing->sjw >= min->sjw, "sjw lower than min");
	zassert_true(timing->prop_seg >= min->prop_seg, "prop_seg lower than min");
	zassert_true(timing->phase_seg1 >= min->phase_seg1, "phase_seg1 lower than min");
	zassert_true(timing->phase_seg2 >= min->phase_seg2, "phase_seg2 lower than min");
	zassert_true(timing->prescaler >= min->prescaler, "prescaler lower than min");
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
static void test_timing_values(const struct device *dev, const struct can_timing_test *test,
			       bool data_phase)
{
	const struct can_timing *max = NULL;
	const struct can_timing *min = NULL;
	struct can_timing timing = { 0 };
	int sp_err;
	int err;

	printk("testing bitrate %u, sample point %u.%u%%: ",
		test->bitrate, test->sp / 10, test->sp % 10);

	if (data_phase) {
		if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
			min = can_get_timing_data_min(dev);
			max = can_get_timing_data_max(dev);
			sp_err = can_calc_timing_data(dev, &timing, test->bitrate, test->sp);
		} else {
			zassert_unreachable("data phase timing test without CAN FD support");
		}
	} else {
		min = can_get_timing_min(dev);
		max = can_get_timing_max(dev);
		sp_err = can_calc_timing(dev, &timing, test->bitrate, test->sp);
	}

	if (sp_err == -ENOTSUP) {
		printk("bitrate not supported\n");
	} else {
		zassert_true(sp_err >= 0, "unknown error %d", sp_err);
		zassert_true(sp_err <= SAMPLE_POINT_MARGIN, "sample point error %d too large",
			     sp_err);

		printk("sjw = %u, prop_seg = %u, phase_seg1 = %u, phase_seg2 = %u, prescaler = %u ",
			timing.sjw, timing.prop_seg, timing.phase_seg1, timing.phase_seg2,
			timing.prescaler);

		assert_bitrate_correct(dev, &timing, test->bitrate);
		assert_timing_within_bounds(&timing, min, max);
		assert_sp_within_margin(&timing, test->sp, SAMPLE_POINT_MARGIN);

		if (IS_ENABLED(CONFIG_CAN_FD_MODE) && data_phase) {
			err = can_set_timing_data(dev, &timing);
		} else {
			err = can_set_timing(dev, &timing);
		}
		zassert_equal(err, 0, "failed to set timing (err %d)", err);

		printk("OK, sample point error %d.%d%%\n", sp_err / 10, sp_err % 10);
	}
}

/**
 * @brief Test all CAN timing values
 */
ZTEST_USER(can_timing, test_timing)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	int i;

	for (i = 0; i < ARRAY_SIZE(can_timing_tests); i++) {
		test_timing_values(dev, &can_timing_tests[i], false);
	}
}

/**
 * @brief Test all CAN timing values for the data phase.
 */
ZTEST_USER(can_timing, test_timing_data)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	can_mode_t cap;
	int err;
	int i;

	err = can_get_capabilities(dev, &cap);
	zassert_equal(err, 0, "failed to get CAN controller capabilities (err %d)", err);

	if ((cap & CAN_MODE_FD) == 0) {
		ztest_test_skip();
	}

	for (i = 0; i < ARRAY_SIZE(can_timing_data_tests); i++) {
		test_timing_values(dev, &can_timing_data_tests[i], true);
	}
}

void *can_timing_setup(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	uint32_t core_clock;
	int err;

	zassert_true(device_is_ready(dev), "CAN device not ready");

	err = can_get_core_clock(dev, &core_clock);
	zassert_equal(err, 0, "failed to get core CAN clock");

	printk("testing on device %s @ %u Hz\n", dev->name, core_clock);

	k_object_access_grant(dev, k_current_get());

	return NULL;
}

ZTEST_SUITE(can_timing, NULL, can_timing_setup, NULL, NULL, NULL);
