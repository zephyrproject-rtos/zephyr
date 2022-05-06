/*
 * Copyright (c) 2019-2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_clock.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#ifndef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
#error "LFCLK must use RC source"
#endif

#define CALIBRATION_PROCESS_TIME_MS 35

extern void mock_temp_nrf5_value_set(struct sensor_value *val);

static void turn_on_clock(const struct device *dev,
			  clock_control_subsys_t subsys)
{
	int err;
	int res;
	struct onoff_client cli;
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(subsys);

	sys_notify_init_spinwait(&cli.notify);
	err = onoff_request(mgr, &cli);
	if (err < 0) {
		zassert_false(true, "Failed to start clock");
	}
	while (sys_notify_fetch_result(&cli.notify, &res) != 0) {
	}
}

static void turn_off_clock(const struct device *dev,
			   clock_control_subsys_t subsys)
{
	int err;
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(subsys);

	do {
		err = onoff_release(mgr);
	} while (err >= 0);

	while (clock_control_get_status(dev, subsys) !=
		CLOCK_CONTROL_STATUS_OFF) {
	}
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

	cal_cnt = z_nrf_clock_calibration_count();
	skip_cnt = z_nrf_clock_calibration_skips_count();

	k_sleep(K_MSEC(sleep_ms));

	cal_cnt = z_nrf_clock_calibration_count() - cal_cnt;
	skip_cnt = z_nrf_clock_calibration_skips_count() - skip_cnt;

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
static void test_basic_clock_calibration(void)
{
	int wait_ms = CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD *
		(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP + 1) +
		CALIBRATION_PROCESS_TIME_MS;
	struct sensor_value value = { .val1 = 0, .val2 = 0 };

	mock_temp_nrf5_value_set(&value);
	sync_just_after_calibration();

	TEST_CALIBRATION(1, CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP,
			 wait_ms);
}

/* Test checks if calibration happens just after clock is enabled. */
static void test_calibration_after_enabling_lfclk(void)
{
	if (IS_ENABLED(CONFIG_SOC_NRF52832)) {
		/* On nrf52832 LF clock cannot be stopped because it leads
		 * to RTC COUNTER register reset and that is unexpected by
		 * system clock which is disrupted and may hang in the test.
		 */
		ztest_test_skip();
	}

	const struct device *clk_dev =
		device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	struct sensor_value value = { .val1 = 0, .val2 = 0 };

	mock_temp_nrf5_value_set(&value);

	turn_off_clock(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_LF);

	k_busy_wait(10000);

	turn_on_clock(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_LF);

	TEST_CALIBRATION(1, 0,
			 CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD);
}

/* Test checks if temperature change triggers calibration. */
static void test_temp_change_triggers_calibration(void)
{
	struct sensor_value value = { .val1 = 0, .val2 = 0 };

	mock_temp_nrf5_value_set(&value);
	sync_just_after_calibration();

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
}

/* Test checks if z_nrf_clock_calibration_force_start() results in immediate
 * calibration.
 */
static void test_force_calibration(void)
{
	sync_just_after_calibration();

	z_nrf_clock_calibration_force_start();

	/*expect immediate calibration */
	TEST_CALIBRATION(1, 0,
		CALIBRATION_PROCESS_TIME_MS + 5);

	/* and back to scheduled operation. */
	TEST_CALIBRATION(1, CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP,
		CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD *
		(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP + 1) +
		CALIBRATION_PROCESS_TIME_MS);

}

void test_main(void)
{
	ztest_test_suite(test_nrf_clock_calibration,
		ztest_unit_test(test_basic_clock_calibration),
		ztest_unit_test(test_calibration_after_enabling_lfclk),
		ztest_unit_test(test_temp_change_triggers_calibration),
		ztest_unit_test(test_force_calibration)
			 );
	ztest_run_test_suite(test_nrf_clock_calibration);
}
