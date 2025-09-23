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
#include <zephyr/dt-bindings/clock/nrf-auxpll.h>

struct test_clk_context {
	const struct device *clk_dev;
	const struct nrf_clock_spec *clk_specs;
	size_t clk_specs_size;
};

#if defined(CONFIG_CLOCK_CONTROL_NRF_HSFLL_LOCAL) ||                                               \
	defined(CONFIG_CLOCK_CONTROL_NRF_IRON_HSFLL_LOCAL)
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
#endif

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
const struct nrf_clock_spec test_clk_specs_fll16m[] = {
	{
		.frequency = MHZ(16),
		.accuracy = 20000,
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

#if defined(CONFIG_CLOCK_CONTROL_NRF_HSFLL_GLOBAL)
const struct nrf_clock_spec test_clk_specs_global_hsfll[] = {
	{
		.frequency = MHZ(320),
	},
	{
		.frequency = MHZ(256),
	},
	{
		.frequency = MHZ(128),
	},
	{
		.frequency = MHZ(64),
	},
};

static const struct test_clk_context global_hsfll_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(hsfll120)),
		.clk_specs = test_clk_specs_global_hsfll,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_global_hsfll),
	},
};
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF_LFCLK)
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
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF_AUXPLL)

#define AUXPLL_COMPAT nordic_nrf_auxpll
#define AUXPLL_NODE DT_INST(0, AUXPLL_COMPAT)
#define AUXPLL_FREQ DT_PROP(AUXPLL_NODE, nordic_frequency)

/* Gets selected AUXPLL DIV and selects the expected frequency */
#if AUXPLL_FREQ == NRF_AUXPLL_FREQUENCY_DIV_MIN
#define AUXPLL_FREQ_OUT 80000000
#elif AUXPLL_FREQ == NRF_AUXPLL_FREQ_DIV_AUDIO_44K1
#define AUXPLL_FREQ_OUT 11289591
#elif AUXPLL_FREQ == NRF_AUXPLL_FREQ_DIV_USB_24M
#define AUXPLL_FREQ_OUT 24000000
#elif AUXPLL_FREQ == NRF_AUXPLL_FREQ_DIV_AUDIO_48K
#define AUXPLL_FREQ_OUT 12287963
#else
/*No use case for NRF_AUXPLL_FREQ_DIV_MAX or others yet*/
#error "Unsupported AUXPLL frequency selection"
#endif

const struct nrf_clock_spec test_clk_specs_auxpll[] = {
	{
		.frequency = AUXPLL_FREQ_OUT,
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
};

static const struct test_clk_context auxpll_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(AUXPLL_NODE),
		.clk_specs = test_clk_specs_auxpll,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_auxpll),
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
	if (ret != -ENOSYS) {
		zassert_ok(ret);
		zassert_equal(rate, clk_spec->frequency);
	}
	k_msleep(1000);
	ret = nrf_clock_control_release(clk_dev, clk_spec);
	zassert_equal(ret, ONOFF_STATE_ON);
}

static void test_clock_control_request(const struct test_clk_context *clk_contexts,
				       size_t contexts_size)
{
	int ret;
	const struct test_clk_context *clk_context;
	size_t clk_specs_size;
	const struct device *clk_dev;
	const struct nrf_clock_spec *req_spec;
	struct nrf_clock_spec res_spec;
	uint32_t startup_time_us;

	for (size_t i = 0; i < contexts_size; i++) {
		clk_context = &clk_contexts[i];
		clk_specs_size = clk_context->clk_specs_size;

		for (size_t u = 0; u < clk_specs_size; u++) {
			clk_dev = clk_context->clk_dev;
			req_spec = &clk_context->clk_specs[u];

			zassert_true(device_is_ready(clk_dev), "%s is not ready", clk_dev->name);

			TC_PRINT("Requested clock (%s) spec: frequency %d, accuracy %d, precision "
				 "%d\n",
				 clk_dev->name, req_spec->frequency, req_spec->accuracy,
				 req_spec->precision);

			ret = nrf_clock_control_resolve(clk_dev, req_spec, &res_spec);
			zassert(ret == 0 || ret == -ENOSYS,
				"minimum clock specs could not be resolved");
			if (ret == 0) {
				TC_PRINT("Resolved spec: frequency %d, accuracy %d, precision "
					 "%d\n",
					 res_spec.frequency, res_spec.accuracy, res_spec.precision);
			} else if (ret == -ENOSYS) {
				TC_PRINT("resolve not supported\n");
				res_spec.frequency = req_spec->frequency;
				res_spec.accuracy = req_spec->accuracy;
				res_spec.precision = req_spec->precision;
			}

			ret = nrf_clock_control_get_startup_time(clk_dev, &res_spec,
								 &startup_time_us);
			zassert(ret == 0 || ret == -ENOSYS, "failed to get startup time");
			if (ret == 0) {
				TC_PRINT("startup time for resloved spec: %uus\n", startup_time_us);
			} else if (ret == -ENOSYS) {
				TC_PRINT("get startup time not supported\n");
			}

			TC_PRINT("Applying spec: frequency %d, accuracy %d, precision "
				 "%d\n",
				 res_spec.frequency, res_spec.accuracy, res_spec.precision);
			test_request_release_clock_spec(clk_dev, &res_spec);
		}
	}
}

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
ZTEST(nrf2_clock_control, test_cpuapp_hsfll_control)
{
	TC_PRINT("APPLICATION DOMAIN HSFLL test\n");
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

			zassert_true(device_is_ready(clk_dev),
				     "%s is not ready", clk_dev->name);

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



#if defined(CONFIG_CLOCK_CONTROL_NRF_HSFLL_GLOBAL)
ZTEST(nrf2_clock_control, test_global_hsfll_control)
{
	TC_PRINT("Global HSFLL test\n");
	test_clock_control_request(global_hsfll_test_clk_contexts,
				   ARRAY_SIZE(global_hsfll_test_clk_contexts));
}
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF_LFCLK)
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

	zassert_true(device_is_ready(clk_dev),
		     "%s is not ready", clk_dev->name);

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
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF_AUXPLL)
ZTEST(nrf2_clock_control, test_auxpll_control)
{
	TC_PRINT("AUXPLL control test\n");
	test_clock_control_request(auxpll_test_clk_contexts,
				   ARRAY_SIZE(auxpll_test_clk_contexts));
}
#endif

static void *setup(void)
{
#if defined(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP)
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_hsfll));
	const struct nrf_clock_spec clk_spec = {
		.frequency = MHZ(64),
	};
	struct onoff_client cli;
	uint32_t start_uptime;
	const uint32_t timeout_ms = 3000;

	zassert_true(device_is_ready(clk_dev),
		     "%s is not ready", clk_dev->name);

	/* Constantly make requests to DVFS until one is successful (what also
	 * means that the service has finished its initialization). This loop
	 * also verifies that the clock control driver is able to recover after
	 * an unsuccesful attempt to start a clock (at least one initial request
	 * is expected to fail here due to DFVS not being initialized yet).
	 */
	TC_PRINT("Polling DVFS until it is ready\n");
	start_uptime = k_uptime_get_32();
	while (1) {
		int status;
		int ret;

		sys_notify_init_spinwait(&cli.notify);
		ret = nrf_clock_control_request(clk_dev, &clk_spec, &cli);
		/* The on-off manager for this clock controller is expected to
		 * always be in the off state when a request is done (its error
		 * state is expected to be cleared by the clock control driver).
		 */
		zassert_equal(ret, ONOFF_STATE_OFF, "request result: %d", ret);
		do {
			ret = sys_notify_fetch_result(&cli.notify, &status);
			k_yield();
		} while (ret == -EAGAIN);

		if (status == 0) {
			TC_PRINT("DVFS is ready\n");
			break;
		} else if ((k_uptime_get_32() - start_uptime) >= timeout_ms) {
			TC_PRINT("DVFS is not ready after %u ms\n", timeout_ms);
			ztest_test_fail();
			break;
		}
	}
#endif

	return NULL;
}

ZTEST_SUITE(nrf2_clock_control, NULL, setup, NULL, NULL, NULL);
