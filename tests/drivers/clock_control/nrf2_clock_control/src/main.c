/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

struct test_clk_context {
	const struct device *clk_dev;
	const struct nrf_clock_spec *clk_specs;
	size_t clk_specs_size;
};

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
const struct nrf_clock_spec test_clk_specs_fll16m[] = {
	{
		.frequency = MHZ(16),
		.accuracy = 20000,
		.precision = 0,
	},
	{
		.frequency = MHZ(16),
		.accuracy = 5000,
		.precision = 0,
	},
	{
		.frequency = MHZ(16),
		.accuracy = 30,
		.precision = 0,
	},
};

static const struct test_clk_context test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m)),
		.clk_specs = test_clk_specs_fll16m,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_fll16m),
	},
};
#endif

static void test_request_release_clock_spec(const struct device *clk_dev,
					    const struct nrf_clock_spec *clk_spec)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;

	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	zassert_between_inclusive(ret, 0, 2);
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	zassert_ok(ret);
	zassert_ok(res);
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	zassert_ok(ret);
	zassert_equal(rate, clk_spec->frequency);
	k_msleep(1000);
	ret = nrf_clock_control_release(clk_dev, clk_spec);
	zassert_equal(ret, ONOFF_STATE_ON);
}

ZTEST(nrf2_clock_control, test_request)
{
	const struct test_clk_context *clk_context;
	size_t clk_specs_size;
	const struct device *clk_dev;
	const struct nrf_clock_spec *clk_spec;

	for (size_t i = 0; i < ARRAY_SIZE(test_clk_contexts); i++) {
		clk_context = &test_clk_contexts[i];
		clk_specs_size = clk_context->clk_specs_size;

		for (size_t u = 0; u < clk_specs_size; u++) {
			clk_dev = clk_context->clk_dev;
			clk_spec = &clk_context->clk_specs[u];

			test_request_release_clock_spec(clk_dev, clk_spec);
		}
	}
}

ZTEST_SUITE(nrf2_clock_control, NULL, NULL, NULL, NULL, NULL);
