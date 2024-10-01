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

const struct nrf_clock_spec test_clk_specs_hsfll[] = {
	{
		.frequency = MHZ(128),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(320),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(64),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
};

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
const struct nrf_clock_spec test_clk_specs_fll16m[] = {
	{
		.frequency = MHZ(16),
		.accuracy = 20000,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(16),
		.accuracy = 5020,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(16),
		.accuracy = 30,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
};

static const struct test_clk_context fll16m_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m)),
		.clk_specs = test_clk_specs_fll16m,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_fll16m),
	},
};

const struct nrf_clock_spec invalid_test_clk_specs_fll16m[] = {
	{
		.frequency = MHZ(16),
		.accuracy = 20,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(19),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(16),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_HIGH,
	},
};

static const struct test_clk_context invalid_fll16m_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m)),
		.clk_specs = invalid_test_clk_specs_fll16m,
		.clk_specs_size = ARRAY_SIZE(invalid_test_clk_specs_fll16m),
	},
};

static const struct test_clk_context cpuapp_hsfll_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_hsfll)),
		.clk_specs = test_clk_specs_hsfll,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_hsfll),
	},
};
#elif defined(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD)
static const struct test_clk_context cpurad_hsfll_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(cpurad_hsfll)),
		.clk_specs = test_clk_specs_hsfll,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_hsfll),
	},
};
#endif

const struct nrf_clock_spec test_clk_specs_lfclk[] = {
	{
		.frequency = 32768,
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = 32768,
		.accuracy = 20,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = 32768,
		.accuracy = 20,
		.precision = NRF_CLOCK_CONTROL_PRECISION_HIGH,
	},
};

static const struct test_clk_context lfclk_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk)),
		.clk_specs = test_clk_specs_lfclk,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_lfclk),
	},
};

static void test_request_release_clock_spec(const struct device *clk_dev,
					    const struct nrf_clock_spec *clk_spec)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;

	TC_PRINT("Clock under test: %s\n", clk_dev->name);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	zassert_between_inclusive(ret, 0, 2);
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	TC_PRINT("Clock control request return value: %d\n", ret);
	TC_PRINT("Clock control request response code: %d\n", res);
	zassert_ok(ret);
	zassert_ok(res);
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	zassert_ok(ret);
	zassert_equal(rate, clk_spec->frequency);
	k_msleep(1000);
	ret = nrf_clock_control_release(clk_dev, clk_spec);
	zassert_equal(ret, ONOFF_STATE_ON);
}

static void test_clock_control_request(const struct test_clk_context *clk_contexts,
				       size_t contexts_size)
{
	const struct test_clk_context *clk_context;
	size_t clk_specs_size;
	const struct device *clk_dev;
	const struct nrf_clock_spec *clk_spec;

	for (size_t i = 0; i < contexts_size; i++) {
		clk_context = &clk_contexts[i];
		clk_specs_size = clk_context->clk_specs_size;

		for (size_t u = 0; u < clk_specs_size; u++) {
			clk_dev = clk_context->clk_dev;
			clk_spec = &clk_context->clk_specs[u];

			TC_PRINT("Applying clock (%s) spec: frequency %d, accuracy %d, precision "
				 "%d\n",
				 clk_dev->name, clk_spec->frequency, clk_spec->accuracy,
				 clk_spec->precision);
			test_request_release_clock_spec(clk_dev, clk_spec);
		}
	}
}

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
ZTEST(nrf2_clock_control, test_cpuapp_hsfll_control)
{

	TC_PRINT("APPLICATION DOMAIN HSFLL test\n");
	/* Wait for the DVFS init to complete */
	k_msleep(3000);
	test_clock_control_request(cpuapp_hsfll_test_clk_contexts,
				   ARRAY_SIZE(cpuapp_hsfll_test_clk_contexts));
}

ZTEST(nrf2_clock_control, test_fll16m_control)
{
	TC_PRINT("FLL16M test\n");
	test_clock_control_request(fll16m_test_clk_contexts, ARRAY_SIZE(fll16m_test_clk_contexts));
}

ZTEST(nrf2_clock_control, test_invalid_fll16m_clock_spec_response)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	const struct test_clk_context *clk_context;
	size_t clk_specs_size;
	const struct device *clk_dev;
	const struct nrf_clock_spec *clk_spec;

	TC_PRINT("FLL16M invalid clock specification test\n");

	for (size_t i = 0; i < ARRAY_SIZE(invalid_fll16m_test_clk_contexts); i++) {
		clk_context = &invalid_fll16m_test_clk_contexts[i];
		clk_specs_size = clk_context->clk_specs_size;

		for (size_t u = 0; u < clk_specs_size; u++) {
			clk_dev = clk_context->clk_dev;
			clk_spec = &clk_context->clk_specs[u];

			TC_PRINT("Applying clock (%s) spec: frequency %d, accuracy %d, precision "
				 "%d\n",
				 clk_dev->name, clk_spec->frequency, clk_spec->accuracy,
				 clk_spec->precision);

			sys_notify_init_spinwait(&cli.notify);
			ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
			TC_PRINT("Clock control request return value: %d\n", ret);
			TC_PRINT("Clock control request response code: %d\n", res);
			zassert_equal(ret, -EINVAL);
			zassert_ok(res);
		}
	}
}
#elif defined(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD)
ZTEST(nrf2_clock_control, test_cpurad_hsfll_control)
{
	TC_PRINT("RADIO DOMAIN HSFLL test\n");
	test_clock_control_request(cpurad_hsfll_test_clk_contexts,
				   ARRAY_SIZE(cpurad_hsfll_test_clk_contexts));
}
#endif

ZTEST(nrf2_clock_control, test_lfclk_control)
{
	TC_PRINT("LFCLK test\n");
	test_clock_control_request(lfclk_test_clk_contexts, ARRAY_SIZE(lfclk_test_clk_contexts));
}

ZTEST(nrf2_clock_control, test_safe_request_cancellation)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	const struct test_clk_context *clk_context = &lfclk_test_clk_contexts[0];
	const struct device *clk_dev = clk_context->clk_dev;
	const struct nrf_clock_spec *clk_spec = &test_clk_specs_lfclk[0];

	TC_PRINT("Safe clock request cancellation\n");
	TC_PRINT("Clock under test: %s\n", clk_dev->name);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	zassert_between_inclusive(ret, 0, 2);
	TC_PRINT("Clock control request return value: %d\n", ret);
	TC_PRINT("Clock control request response code: %d\n", res);
	zassert_ok(res);
	ret = nrf_clock_control_cancel_or_release(clk_dev, clk_spec, &cli);
	TC_PRINT("Clock control safe cancellation return value: %d\n", ret);
	zassert_between_inclusive(ret, ONOFF_STATE_ON, ONOFF_STATE_TO_ON);
}

ZTEST_SUITE(nrf2_clock_control, NULL, NULL, NULL, NULL, NULL);
