/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/can.h>
#include <ztest.h>
#include <strings.h>

/*
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_timing test_basic_can_timing
 * @brief TestPurpose: verify timing algorithm
 * @details
 * - Test Steps
 *   -# Calculate timing for a sample
 *   -# Verify sample point
 *   -# verify bitrate
 * - Expected Results
 *   -# All tests MUST pass
 * @}
 */

#if defined(CONFIG_CAN_LOOPBACK_DEV_NAME)
#define CAN_DEVICE_NAME CONFIG_CAN_LOOPBACK_DEV_NAME
#else
#define CAN_DEVICE_NAME DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL
#endif

const struct device *can_dev;

struct timing_samples {
	uint32_t bitrate;
	uint16_t sp;
	bool inval;
};

const struct timing_samples samples[] = {
	{125000, 875, false},
	{500000, 875, false},
	{1000000, 875, false},
	{125000, 900, false},
	{125000, 800, false},
#ifdef CONFIG_CAN_FD_MODE
	{1000000 + 1, 875, true},
#else
	{8000000 + 1, 875, true},
#endif
	{125000, 1000, true},
};

/*
 * Bitrate must match exactly
 */
static void verify_bitrate(struct can_timing  *timing, uint32_t bitrate)
{
	const uint32_t ts = 1 + timing->prop_seg + timing->phase_seg1 +
			    timing->phase_seg2;
	uint32_t core_clock;
	uint32_t bitrate_calc;
	int ret;

	ret = can_get_core_clock(can_dev, &core_clock);
	zassert_equal(ret, 0, "Unable to get core clock");

	bitrate_calc = core_clock / timing->prescaler / ts;
	zassert_equal(bitrate, bitrate_calc, "Bitrate missmatch");
}

/*
 * SP must be withing the margin and in bound of the limits
 */
static void verify_sp(struct can_timing  *timing, uint16_t sp,
		      uint16_t sp_margin)
{
	const struct can_driver_api *api =
		(const struct can_driver_api *)can_dev->api;
	const struct can_timing *max = &api->timing_max;
	const struct can_timing *min = &api->timing_min;
	uint32_t ts = 1 + timing->prop_seg + timing->phase_seg1 +
		      timing->phase_seg2;
	uint16_t sp_calc =
		((1 + timing->prop_seg + timing->phase_seg1) * 1000) / ts;

	zassert_true(timing->prop_seg <= max->prop_seg, "prop_seg exceeds max");
	zassert_true(timing->phase_seg1 <= max->phase_seg1,
		     "phase_seg1 exceeds max");
	zassert_true(timing->phase_seg2 <= max->phase_seg2,
		     "phase_seg2 exceeds max");
	zassert_true(timing->prop_seg >= min->prop_seg,
		     "prop_seg lower than min");
	zassert_true(timing->phase_seg1 >= min->phase_seg1,
		     "phase_seg1 lower than min");
	zassert_true(timing->phase_seg2 >= min->phase_seg2,
		     "phase_seg2 lower than min");

	zassert_within(sp, sp_calc, sp_margin, "SP error %d [%d] not within %d",
		       sp_calc, sp, sp_margin);
}

/*
 * Verify the result of the algorithm
 */
static void test_verify_algo(void)
{
	struct can_timing  timing = {0};
	int ret;

	for (int i = 0; i < ARRAY_SIZE(samples); ++i) {
		ret = can_calc_timing(can_dev, &timing, samples[i].bitrate,
				      samples[i].sp);
		if (samples[i].inval) {
			zassert_equal(ret, -EINVAL,
				      "ret value %d not -EINVAL", ret);
			continue;
		}

		zassert_true(ret >= 0, "Unknown error %d", ret);
		/* For the given values, we expect a sp error < 10% */
		zassert_true(ret < 100, "Huge sample point error %d", ret);
		verify_sp(&timing, samples[i].sp, ret);
		verify_bitrate(&timing, samples[i].bitrate);
	}
}

void test_main(void)
{
	can_dev = device_get_binding(CAN_DEVICE_NAME);
	zassert_not_null(can_dev, "Device not found");

	ztest_test_suite(can_timing,
			 ztest_unit_test(test_verify_algo));
	ztest_run_test_suite(can_timing);
}
