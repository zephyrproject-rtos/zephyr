/*
 * Copyright (c) 2019-2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_clock.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(test);

#if ((defined(CONFIG_CLOCK_CONTROL_NRF) && !defined(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC)) ||        \
	(!defined(CONFIG_CLOCK_CONTROL_NRF) &&                                                     \
	!DT_ENUM_HAS_VALUE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_lfclk), k32src, rc)))
#error "LFCLK must use RC source"
#endif

#define CALIBRATION_PROCESS_TIME_MS 35

extern void mock_temp_nrf5_value_set(struct sensor_value *val);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static void turn_on_clock(const struct device *dev,
			  clock_control_subsys_t subsys)
#else
static void turn_on_clock(const struct device *dev)
#endif
{
	int err;
	int res;
	struct onoff_client cli;
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(subsys);
#endif

	sys_notify_init_spinwait(&cli.notify);
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	err = onoff_request(mgr, &cli);
#else
	err = nrf_clock_control_request(dev, NULL, &cli);
#endif
	if (err < 0) {
		zassert_false(true, "Failed to start clock");
	}
	while (sys_notify_fetch_result(&cli.notify, &res) != 0) {
	}
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static void turn_off_clock(const struct device *dev,
			   clock_control_subsys_t subsys)
#else
static void turn_off_clock(const struct device *dev)
#endif
{
	int err;
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(subsys);
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	do {
		err = onoff_release(mgr);
	} while (err >= 0);

	while (clock_control_get_status(dev, subsys) != CLOCK_CONTROL_STATUS_OFF) {
	}
#else
	do {
		err = nrf_clock_control_release(dev, NULL);
	} while (err >= 0);

	while (clock_control_get_status(dev, NULL) != CLOCK_CONTROL_STATUS_OFF) {
	}
#endif
}

#define TEST_CALIBRATION(exp_cal, exp_skip, sleep_ms) \
	test_calibration(exp_cal, exp_skip, sleep_ms, __LINE__)

/* Function tests if during given time expected number of calibrations and
 * skips occurs.
 */
static void test_calibration(uint32_t exp_cal, uint32_t exp_skip,
				uint32_t sleep_ms, uint32_t line)
{
	int cal_cnt;
	int skip_cnt;

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	const struct device *const clk_dev = DEVICE_DT_GET_ONE(nordic_nrf_clock);

	turn_on_clock(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_HF);
#else
	const struct device *const clk_dev = COND_CODE_1(NRF_CLOCK_HAS_HFCLK,
							DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk),
							DEVICE_DT_GET_ONE(nordic_nrf_clock_xo));

	turn_on_clock(clk_dev);
#endif

	cal_cnt = z_nrf_clock_calibration_count();
	skip_cnt = z_nrf_clock_calibration_skips_count();

	k_sleep(K_MSEC(sleep_ms));

	cal_cnt = z_nrf_clock_calibration_count() - cal_cnt;
	skip_cnt = z_nrf_clock_calibration_skips_count() - skip_cnt;

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	turn_off_clock(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_HF);
#else
	turn_off_clock(clk_dev);
#endif

	zassert_equal(cal_cnt, exp_cal,
			"%d: Unexpected number of calibrations (%d, exp:%d)",
			line, cal_cnt, exp_cal);
	zassert_equal(skip_cnt, exp_skip,
			"%d: Unexpected number of skips (%d, exp:%d)",
			line, skip_cnt, exp_skip);
}

/* Function pends until calibration counter is performed. When function leaves,
 * it is just after calibration.
 */
static void sync_just_after_calibration(void)
{
	uint32_t cal_cnt = z_nrf_clock_calibration_count();

	/* wait until calibration is performed. */
	while (z_nrf_clock_calibration_count() == cal_cnt) {
		k_sleep(K_MSEC(1));
	}
}

/* Test checks if calibration and calibration skips are performed according
 * to timing configuration.
 */
ZTEST(nrf_clock_calibration, test_basic_clock_calibration)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	int wait_ms = CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD *
		(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP + 1) +
#else
	int wait_ms = CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD *
		(CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP + 1) +
#endif
		CALIBRATION_PROCESS_TIME_MS;
	struct sensor_value value = { .val1 = 0, .val2 = 0 };

	mock_temp_nrf5_value_set(&value);
	sync_just_after_calibration();

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	TEST_CALIBRATION(1, CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP,
			 wait_ms);
#else
	TEST_CALIBRATION(1, CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP,
			 wait_ms);
#endif
}

/* Test checks if calibration happens just after clock is enabled. */
ZTEST(nrf_clock_calibration, test_calibration_after_enabling_lfclk)
{
	if (IS_ENABLED(CONFIG_SOC_NRF52832)) {
		/* On nrf52832 LF clock cannot be stopped because it leads
		 * to RTC COUNTER register reset and that is unexpected by
		 * system clock which is disrupted and may hang in the test.
		 */
		ztest_test_skip();
	}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	const struct device *const clk_dev = DEVICE_DT_GET_ONE(nordic_nrf_clock);
#else
	const struct device *const clk_dev = DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk);
#endif
	struct sensor_value value = { .val1 = 0, .val2 = 0 };

	zassert_true(device_is_ready(clk_dev), "Device is not ready");

	mock_temp_nrf5_value_set(&value);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	turn_off_clock(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_LF);
#else
	turn_off_clock(clk_dev);
#endif

	k_busy_wait(10000);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	turn_on_clock(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_LF);
#else
	turn_on_clock(clk_dev);
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	TEST_CALIBRATION(1, 0, CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD);
#else
	TEST_CALIBRATION(1, 0, CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD);
#endif
}

/* Test checks if temperature change triggers calibration. */
ZTEST(nrf_clock_calibration, test_temp_change_triggers_calibration)
{
	struct sensor_value value = { .val1 = 0, .val2 = 0 };

	mock_temp_nrf5_value_set(&value);
	sync_just_after_calibration();

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	/* change temperature by 0.25'C which should not trigger calibration */
	value.val2 += ((CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF - 1) *
			250000);

	mock_temp_nrf5_value_set(&value);

	/* expected one skip */
	TEST_CALIBRATION(0, CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP,
				CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP *
				CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD +
				CALIBRATION_PROCESS_TIME_MS);

	TEST_CALIBRATION(1, 0,
			 CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD + 40);

	value.val2 += (CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF * 250000);
	mock_temp_nrf5_value_set(&value);

	/* expect calibration due to temp change. */
	TEST_CALIBRATION(1, 0,
			 CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD + 40);
#else
	/* change temperature by 0.25'C which should not trigger calibration */
	value.val2 += ((CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_TEMP_DIFF - 1) *
			250000);

	mock_temp_nrf5_value_set(&value);

	/* expected one skip */
	TEST_CALIBRATION(0, CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP,
				CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP *
				CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD +
				CALIBRATION_PROCESS_TIME_MS);

	TEST_CALIBRATION(1, 0,
			 CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD + 40);

	value.val2 += (CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_TEMP_DIFF * 250000);
	mock_temp_nrf5_value_set(&value);

	/* expect calibration due to temp change. */
	TEST_CALIBRATION(1, 0,
			 CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD + 40);
#endif
}

/* Test checks if z_nrf_clock_calibration_force_start() results in immediate
 * calibration.
 */
ZTEST(nrf_clock_calibration, test_force_calibration)
{
	sync_just_after_calibration();

	z_nrf_clock_calibration_force_start();

	/*expect immediate calibration */
	TEST_CALIBRATION(1, 0,
		CALIBRATION_PROCESS_TIME_MS + 5);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	/* and back to scheduled operation. */
	TEST_CALIBRATION(1, CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP,
		CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD *
		(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP + 1) +
		CALIBRATION_PROCESS_TIME_MS);
#else
	/* and back to scheduled operation. */
	TEST_CALIBRATION(1, CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP,
		CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD *
		(CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP + 1) +
		CALIBRATION_PROCESS_TIME_MS);
#endif

}
ZTEST_SUITE(nrf_clock_calibration, NULL, NULL, NULL, NULL, NULL);
