/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control/clock_control_adsp.h>
#include <zephyr/drivers/clock_control.h>

static void check_clocks(struct cavs_clock_info *clocks, uint32_t freq_idx)
{
	int i;

	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		zassert_equal(clocks[i].current_freq, freq_idx, "");
	}
}

ZTEST(cavs_clock_control, test_cavs_clock_driver)
{
	struct cavs_clock_info *clocks = cavs_clocks_get();

	zassert_not_null(clocks, "");

	cavs_clock_set_freq(CAVS_CLOCK_FREQ_LPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_LPRO);

	cavs_clock_set_freq(CAVS_CLOCK_FREQ_HPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_HPRO);

#ifdef CAVS_CLOCK_HAS_WOVCRO
	cavs_clock_set_freq(CAVS_CLOCK_FREQ_WOVCRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_WOVCRO);
#endif
}

ZTEST(cavs_clock_control, test_cavs_clock_control)
{
	struct cavs_clock_info *clocks = cavs_clocks_get();
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(clkctl));

	zassert_not_null(clocks, "");

	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   CAVS_CLOCK_FREQ_LPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_LPRO);

	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   CAVS_CLOCK_FREQ_HPRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_HPRO);

#ifdef CAVS_CLOCK_HAS_WOVCRO
	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   CAVS_CLOCK_FREQ_WOVCRO);
	check_clocks(clocks, CAVS_CLOCK_FREQ_WOVCRO);
#endif
}
ZTEST_SUITE(cavs_clock_control, NULL, NULL, NULL, NULL, NULL);
