/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <zephyr/drivers/clock_control/clock_control_cavs.h>
#include <zephyr/drivers/clock_control.h>

static void check_clocks(struct cavs_clock_info *clocks, uint32_t freq_idx)
{
	int i;

	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		zassert_equal(clocks[i].current_freq, freq_idx, "");
	}
}

static void test_cavs_clock_driver(void)
{
	struct cavs_clock_info *clocks = cavs_clocks_get();

	zassert_not_null(clocks, "");

	cavs_clock_set_freq(CAVS_CLOCK_FREQ_LPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_LPRO);

	cavs_clock_set_freq(CAVS_CLOCK_FREQ_HPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_HPRO);

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	cavs_clock_set_freq(CAVS_CLOCK_FREQ_WOVCRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_WOVCRO);
#endif
}

static void test_cavs_clock_control(void)
{
	struct cavs_clock_info *clocks = cavs_clocks_get();
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(clkctl));

	zassert_not_null(clocks, "");

	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   CAVS_CLOCK_FREQ_LPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_LPRO);

	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   CAVS_CLOCK_FREQ_HPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_HPRO);

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V25
	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   CAVS_CLOCK_FREQ_WOVCRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_WOVCRO);
#endif
}

void test_main(void)
{
	ztest_test_suite(cavs_clock_control,
			 ztest_unit_test(test_cavs_clock_control),
			 ztest_unit_test(test_cavs_clock_driver)
			);

	ztest_run_test_suite(cavs_clock_control);
}
