/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control/clock_control_adsp.h>
#include <zephyr/drivers/clock_control.h>

static void check_clocks(struct adsp_cpu_clock_info *clocks, uint32_t freq_idx)
{
	int i;
	unsigned int num_cpus = arch_num_cpus();

	for (i = 0; i < num_cpus; i++) {
		zassert_equal(clocks[i].current_freq, freq_idx, "");
	}
}

ZTEST(adsp_clock_control, test_adsp_clock_driver)
{
	struct adsp_cpu_clock_info *clocks = adsp_cpu_clocks_get();

	zassert_not_null(clocks, "");

	adsp_clock_set_cpu_freq(ADSP_CPU_CLOCK_FREQ_LPRO);
	check_clocks(clocks, ADSP_CPU_CLOCK_FREQ_LPRO);

	adsp_clock_set_cpu_freq(ADSP_CPU_CLOCK_FREQ_HPRO);
	check_clocks(clocks, ADSP_CPU_CLOCK_FREQ_HPRO);

#ifdef ADSP_CLOCK_HAS_WOVCRO
	adsp_clock_set_cpu_freq(ADSP_CPU_CLOCK_FREQ_WOVCRO);
	check_clocks(clocks, ADSP_CPU_CLOCK_FREQ_WOVCRO);
#endif
}

ZTEST(adsp_clock_control, test_adsp_clock_control)
{
	struct adsp_cpu_clock_info *clocks = adsp_cpu_clocks_get();
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(clkctl));

	zassert_not_null(clocks, "");

	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   ADSP_CPU_CLOCK_FREQ_LPRO);
	check_clocks(clocks, ADSP_CPU_CLOCK_FREQ_LPRO);

	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   ADSP_CPU_CLOCK_FREQ_HPRO);
	check_clocks(clocks, ADSP_CPU_CLOCK_FREQ_HPRO);

#ifdef ADSP_CLOCK_HAS_WOVCRO
	clock_control_set_rate(dev, NULL, (clock_control_subsys_rate_t)
					   ADSP_CPU_CLOCK_FREQ_WOVCRO);
	check_clocks(clocks, ADSP_CPU_CLOCK_FREQ_WOVCRO);
#endif
}
ZTEST_SUITE(adsp_clock_control, NULL, NULL, NULL, NULL, NULL);
