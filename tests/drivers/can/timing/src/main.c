/*
 * Copyright (c) 2022-2024 Vestas Wind Systems A/S
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
 * @brief Defines a set of CAN timing test values
 */
struct can_timing_test {
	/** Bitrate in bit/s */
	uint32_t bitrate;
	/** Desired sample point in permille */
	uint16_t sp;
	/** Historically safe bitrate test */
	bool historical;
};

/**
 * @brief List of CAN timing values to test.
 */
/* clang-format off */
static const struct can_timing_test can_timing_tests[] = {
	/* CiA 301 recommended bitrates */
	{   10000, 875, false },
	{   20000, 875, true },
	{   50000, 875, true },
	{  125000, 875, true },
	{  250000, 875, true },
	{  500000, 875, true },
	{  800000, 800, true },
	{ 1000000, 750, true },
};
/* clang-format on */

/**
 * @brief List of CAN FD data phase timing values to test.
 */
/* clang-format off */
static const struct can_timing_test can_timing_data_tests[] = {
	/* CiA 601-2 recommended data phase bitrates */
	{ 1000000, 750, true },
	{ 2000000, 750, false },
	{ 4000000, 750, false },
	{ 5000000, 750, false },
	{ 8000000, 750, false },
};
/* clang-format on */

/*
 * Maximum acceptable bitrate error: 5000 ppm = 0.5 %.
 * CiA 601 allows up to 1.58 %; 0.5 % gives comfortable margin while
 * accommodating core clocks that are not integer multiples of the bitrate.
 */
#define CAN_TIMING_TEST_BITRATE_PPM_MAX 5000

/**
 * @brief Assert that a CAN timing struct achieves the specified bitrate
 *
 * Assert that the values of a CAN timing struct achieve the specified bitrate
 * within @ref CAN_TIMING_TEST_BITRATE_PPM_MAX ppm for a given CAN controller
 * device instance.
 *
 * @param dev pointer to the device structure for the driver instance
 * @param timing pointer to the CAN timing struct
 * @param bitrate the CAN bitrate in bit/s
 */
static void assert_bitrate_correct(const struct device *dev,
				   struct can_timing *timing,
				   uint32_t bitrate)
{
	const uint32_t ts = 1 + timing->prop_seg + timing->phase_seg1 +
			    timing->phase_seg2;
	uint32_t core_clock;
	uint32_t bitrate_calc;
	int32_t ppm;
	int err;

	zassert_not_equal(timing->prescaler, 0, "prescaler is zero");

	err = can_get_core_clock(dev, &core_clock);
	zassert_ok(err, "failed to get core CAN clock");

	bitrate_calc = core_clock / timing->prescaler / ts;
	ppm = (int32_t)(((int64_t)bitrate_calc - (int64_t)bitrate) *
			1000000LL / (int64_t)bitrate);
	if (ppm < 0) {
		ppm = -ppm;
	}
	zassert_true(ppm <= CAN_TIMING_TEST_BITRATE_PPM_MAX,
		     "bitrate error %d ppm for %u bps (calc %u) exceeds %d",
		     ppm, bitrate, bitrate_calc,
		     CAN_TIMING_TEST_BITRATE_PPM_MAX);
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
static void assert_timing_within_bounds(struct can_timing *timing, const struct can_timing *min,
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
		       "sample point %d not within calculated sample point %d +/- %d", sp, sp_calc,
		       sp_margin);
}

/**
 * @brief Determine if a given bitrate test should be skipped.
 *
 * @param dev pointer to the device structure for the driver instance
 * @param test pointer to the set of CAN timing values
 */
bool skip_timing_value_test(const struct device *dev, const struct can_timing_test *test)
{
	uint32_t core_clock;
	int err;

	err = can_get_core_clock(dev, &core_clock);
	zassert_equal(err, 0, "failed to get core CAN clock");

	if (test->historical) {
		/* Historically safe test, never skip */
		return false;
	}

	if (core_clock == MHZ(80)) {
		/* No bitrates skipped when the CAN core clock is 80 MHz */
		return false;
	}

	if (IS_ENABLED(CONFIG_TEST_ALL_BITRATES)) {
		/* No bitrates skipped when all tests explicitly enabled */
		return false;
	}

	return true;
}

/**
 * @brief Test a set of CAN timing values
 *
 * Test a set of CAN timing values on a specified CAN controller device
 * instance.
 *
 * @param  dev pointer to the device structure for the driver instance
 * @param  test pointer to the set of CAN timing values
 * returns true if bitrate was supported, false otherwise
 */
static bool test_timing_values(const struct device *dev, const struct can_timing_test *test,
			       bool data_phase)
{
	const struct can_timing *max = NULL;
	const struct can_timing *min = NULL;
	struct can_timing timing = {0};
	int sp_err = -EINVAL;
	int err;

	printk("testing bitrate %u, sample point %u.%u%%: ", test->bitrate, test->sp / 10,
	       test->sp % 10);

	if (skip_timing_value_test(dev, test)) {
		printk("skipped\n");
		return false;
	}

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
		return false;
	} else {
		zassert_true(sp_err >= 0, "unknown error %d", sp_err);
		zassert_true(sp_err <= CONFIG_CAN_SAMPLE_POINT_MARGIN,
			     "sample point error %d too large", sp_err);

		printk("sjw = %u, prop_seg = %u, phase_seg1 = %u, phase_seg2 = %u, prescaler = %u ",
		       timing.sjw, timing.prop_seg, timing.phase_seg1, timing.phase_seg2,
		       timing.prescaler);

		assert_bitrate_correct(dev, &timing, test->bitrate);
		assert_timing_within_bounds(&timing, min, max);
		assert_sp_within_margin(&timing, test->sp, CONFIG_CAN_SAMPLE_POINT_MARGIN);

		if (IS_ENABLED(CONFIG_CAN_FD_MODE) && data_phase) {
			err = can_set_timing_data(dev, &timing);
		} else {
			err = can_set_timing(dev, &timing);
		}
		zassert_ok(err, "failed to set timing (err %d)", err);

		printk("OK, sample point error %d.%d%%\n", sp_err / 10, sp_err % 10);
	}

	return true;
}

/**
 * @brief Test all CAN timing values
 */
ZTEST_USER(can_timing, test_timing)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	int count = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(can_timing_tests); i++) {
		if (test_timing_values(dev, &can_timing_tests[i], false)) {
			count++;
		}
	}

	zassert_true(count > 0, "no bitrates supported");
}

/**
 * @brief Test all CAN timing values for the data phase.
 */
ZTEST_USER(can_timing, test_timing_data)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	can_mode_t cap;
	int count = 0;
	int err;
	int i;

	err = can_get_capabilities(dev, &cap);
	zassert_ok(err, "failed to get CAN controller capabilities (err %d)", err);

	if ((cap & CAN_MODE_FD) == 0) {
		ztest_test_skip();
	}

	for (i = 0; i < ARRAY_SIZE(can_timing_data_tests); i++) {
		if (test_timing_values(dev, &can_timing_data_tests[i], true)) {
			count++;
		}
	}

	zassert_true(count > 0, "no data phase bitrates supported");
}

/**
 * @brief Standard CiA 301 nominal bitrates for non-integer clock test.
 */
/* clang-format off */
static const uint32_t can_timing_cia_bitrates[] = {
	 125000,
	 250000,
	 500000,
	1000000,
};
/* clang-format on */

/**
 * @brief Test CAN timing calculation for non-integer-multiple core clocks.
 *
 * Some SoCs (e.g. NXP Kinetis KV58 at 120 MHz CAN clock from a 180 MHz PLL)
 * have a CAN peripheral clock that is not an integer multiple of the requested
 * bitrate for every prescaler value.  The old algorithm required exact
 * divisibility and returned -ENOTSUP for these configurations.
 *
 * This test verifies that can_calc_timing() succeeds for all CiA 301
 * recommended nominal bitrates and that the resulting timing achieves the
 * requested bitrate within @ref CAN_TIMING_TEST_BITRATE_PPM_MAX ppm,
 * regardless of whether the core clock is an integer multiple of the bitrate.
 */
ZTEST_USER(can_timing, test_timing_non_integer_clock)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	const struct can_timing *tmin = can_get_timing_min(dev);
	const struct can_timing *tmax = can_get_timing_max(dev);
	uint32_t core_clock;
	uint32_t bitrate;
	int err;
	int i;

	err = can_get_core_clock(dev, &core_clock);
	zassert_ok(err, "failed to get core CAN clock");

	for (i = 0; i < ARRAY_SIZE(can_timing_cia_bitrates); i++) {
		struct can_timing timing = {0};
		uint32_t ts;
		uint32_t actual_bitrate;
		int32_t ppm;
		int sp_err;

		bitrate = can_timing_cia_bitrates[i];

		if (bitrate < can_get_bitrate_min(dev) ||
		    bitrate > can_get_bitrate_max(dev)) {
			printk("bitrate %u bps outside device limits,"
			       " skipping\n", bitrate);
			continue;
		}

		sp_err = can_calc_timing(dev, &timing, bitrate, 0);
		zassert_true(sp_err >= 0,
			     "can_calc_timing failed for %u bps"
			     " (core clock %u Hz, err %d)",
			     bitrate, core_clock, sp_err);

		assert_timing_within_bounds(&timing, tmin, tmax);

		ts = 1U + timing.prop_seg + timing.phase_seg1 +
		     timing.phase_seg2;
		actual_bitrate = core_clock / timing.prescaler / ts;
		ppm = (int32_t)(((int64_t)actual_bitrate -
				 (int64_t)bitrate) *
				1000000LL / (int64_t)bitrate);
		if (ppm < 0) {
			ppm = -ppm;
		}

		printk("bitrate %u bps: prescaler=%u ts=%u"
		       " actual=%u bps err=%d ppm"
		       " sp_err=%d.%d%%\n",
		       bitrate, timing.prescaler, ts,
		       actual_bitrate, ppm,
		       sp_err / 10, sp_err % 10);

		zassert_true(ppm <= CAN_TIMING_TEST_BITRATE_PPM_MAX,
			     "bitrate %u bps error %d ppm exceeds"
			     " %d ppm (core clock %u Hz)",
			     bitrate, ppm,
			     CAN_TIMING_TEST_BITRATE_PPM_MAX,
			     core_clock);
	}
}

void *can_timing_setup(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	uint32_t core_clock;
	int err;

	zassert_true(device_is_ready(dev), "CAN device not ready");
	k_object_access_grant(dev, k_current_get());

	err = can_get_core_clock(dev, &core_clock);
	zassert_ok(err, "failed to get core CAN clock");

	printk("testing on device %s @ %u Hz, sample point margin +/-%u permille\n", dev->name,
	       core_clock, CONFIG_CAN_SAMPLE_POINT_MARGIN);

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		can_mode_t cap;

		err = can_get_capabilities(dev, &cap);
		zassert_ok(err, "failed to get CAN controller capabilities (err %d)", err);

		if ((cap & CAN_MODE_FD) != 0) {
			switch (core_clock) {
			case MHZ(20):
				break;
			case MHZ(40):
				break;
			case MHZ(80):
				break;
			default:
				TC_PRINT("Warning: CiA 601-3 recommends a CAN FD core clock of "
					 "20, 40, or 80 MHz for good node interoperability\n");
				break;
			}
		}
	}

	if (IS_ENABLED(CONFIG_TEST_ALL_BITRATES) && core_clock != MHZ(80)) {
		TC_PRINT("Warning: Testing all bitrates with CAN core clock of %u Hz "
			 "(CONFIG_TEST_ALL_BITRATES=y)\n",
			 core_clock);
	}

	return NULL;
}

ZTEST_SUITE(can_timing, NULL, can_timing_setup, NULL, NULL, NULL);
