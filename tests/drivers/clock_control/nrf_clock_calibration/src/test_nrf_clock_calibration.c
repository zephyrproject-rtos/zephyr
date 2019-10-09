/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_clock.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test);

#ifndef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
#error "LFCLK must use RC source"
#endif

#if CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD != 1
#error "Expected 250ms calibration period"
#endif

static void turn_off_clock(struct device *dev)
{
	int err;

	do {
		err = clock_control_off(dev, 0);
	} while (err == 0);
}

static void lfclk_started_cb(struct device *dev, void *user_data)
{
	*(bool *)user_data = true;
}

/* Test checks if calibration clock is running and generates interrupt as
 * expected  and starts calibration. Validates that HF clock is turned on
 * for calibration and turned off once calibration is done.
 */
static void test_clock_calibration(void)
{
	struct device *hfclk_dev =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_16M");
	struct device *lfclk_dev =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_32K");
	volatile bool started = false;
	struct clock_control_async_data lfclk_data = {
		.cb = lfclk_started_cb,
		.user_data = (void *)&started
	};
	int key;
	u32_t cnt = 0;
	u32_t max_cnt = 1000;

	turn_off_clock(hfclk_dev);
	turn_off_clock(lfclk_dev);

	/* In case calibration needs to be completed. */
	k_busy_wait(100000);

	clock_control_async_on(lfclk_dev, NULL, &lfclk_data);

	while (started == false) {
	}

	k_busy_wait(35000);

	key = irq_lock();
	while (nrf_clock_event_check(NRF_CLOCK_EVENT_CTTO) == 0) {
		k_busy_wait(1000);
		cnt++;
		if (cnt == max_cnt) {
			irq_unlock(key);
			clock_control_off(lfclk_dev, NULL);
			zassert_true(false, "");
		}
	}

	zassert_within(cnt, 250, 10, "Expected 250ms period");

	irq_unlock(key);

	while (clock_control_get_status(hfclk_dev, NULL)
			!= CLOCK_CONTROL_STATUS_ON) {
	}

	key = irq_lock();
	cnt = 0;
	while (nrf_clock_event_check(NRF_CLOCK_EVENT_DONE) == 0) {
		k_busy_wait(1000);
		cnt++;
		if (cnt == max_cnt) {
			irq_unlock(key);
			clock_control_off(lfclk_dev, NULL);
			zassert_true(false, "");
		}
	}

	irq_unlock(key);

	zassert_equal(clock_control_get_status(hfclk_dev, NULL),
			CLOCK_CONTROL_STATUS_OFF,
			"Expected hfclk off after calibration.");

	key = irq_lock();
	cnt = 0;

	while (nrf_clock_event_check(NRF_CLOCK_EVENT_CTTO) == 0) {
		k_busy_wait(1000);
		cnt++;
		if (cnt == max_cnt) {
			irq_unlock(key);
			clock_control_off(lfclk_dev, NULL);
			zassert_true(false, "");
		}
	}

	zassert_within(cnt, 250, 10, "Expected 250ms period (got %d)", cnt);

	irq_unlock(key);

	clock_control_off(lfclk_dev, NULL);
}

/* Test checks that when calibration is active then LF clock is not stopped.
 * Stopping is deferred until calibration is done. Test validates that after
 * completing calibration LF clock is shut down.
 */
static void test_stopping_when_calibration(void)
{
	struct device *hfclk_dev =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_16M");
	struct device *lfclk_dev =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_32K");
	volatile bool started = false;
	struct clock_control_async_data lfclk_data = {
		.cb = lfclk_started_cb,
		.user_data = (void *)&started
	};
	int key;
	u32_t cnt = 0;
	u32_t max_cnt = 1000;

	turn_off_clock(hfclk_dev);
	turn_off_clock(lfclk_dev);

	/* In case calibration needs to be completed. */
	k_busy_wait(100000);

	clock_control_async_on(lfclk_dev, NULL, &lfclk_data);

	while (started == false) {
	}

	/* when lfclk is started first calibration is performed. Wait until it
	 * is done.
	 */
	k_busy_wait(35000);

	key = irq_lock();
	while (nrf_clock_event_check(NRF_CLOCK_EVENT_CTTO) == 0) {
		k_busy_wait(1000);
		cnt++;
		if (cnt == max_cnt) {
			irq_unlock(key);
			clock_control_off(lfclk_dev, NULL);
			zassert_true(false, "");
		}
	}

	zassert_within(cnt, 250, 10, "Expected 250ms period");

	irq_unlock(key);

	while (clock_control_get_status(hfclk_dev, NULL)
			!= CLOCK_CONTROL_STATUS_ON) {
	}

	/* calibration started */
	key = irq_lock();
	clock_control_off(lfclk_dev, NULL);

	zassert_true(nrf_clock_lf_is_running(), "Expected LF still on");

	while (nrf_clock_event_check(NRF_CLOCK_EVENT_DONE) == 0) {
		k_busy_wait(1000);
		cnt++;
		if (cnt == max_cnt) {
			irq_unlock(key);
			clock_control_off(lfclk_dev, NULL);
			zassert_true(false, "");
		}
	}

	irq_unlock(key);

	/* wait some time after which clock should be off. */
	k_busy_wait(300);

	zassert_false(nrf_clock_lf_is_running(), "Expected LF off");

	clock_control_off(lfclk_dev, NULL);
}

static u32_t pend_on_next_calibration(void)
{
	u32_t stamp = k_uptime_get_32();
	int cnt = z_nrf_clock_calibration_count();

	while (cnt == z_nrf_clock_calibration_count()) {
		if ((k_uptime_get_32() - stamp) > 300) {
			zassert_true(false, "Expected calibration");
			return UINT32_MAX;
		}
	}

	return k_uptime_get_32() - stamp;
}

static void test_clock_calibration_force(void)
{
	struct device *hfclk_dev =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_16M");
	struct device *lfclk_dev =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_32K");
	volatile bool started = false;
	struct clock_control_async_data lfclk_data = {
		.cb = lfclk_started_cb,
		.user_data = (void *)&started
	};
	u32_t cnt = 0;
	u32_t period;

	turn_off_clock(hfclk_dev);
	turn_off_clock(lfclk_dev);

	/* In case calibration needs to be completed. */
	k_busy_wait(100000);

	clock_control_async_on(lfclk_dev, NULL, &lfclk_data);

	while (started == false) {
	}

	cnt = z_nrf_clock_calibration_count();

	pend_on_next_calibration();

	zassert_equal(cnt + 1, z_nrf_clock_calibration_count(),
			"Unexpected number of calibrations %d (expected: %d)",
			z_nrf_clock_calibration_count(), cnt + 1);

	/* Next calibration should happen in 250ms, after forcing it should be
	 * done sooner.
	 */
	z_nrf_clock_calibration_force_start();
	period = pend_on_next_calibration();
	zassert_equal(cnt + 2, z_nrf_clock_calibration_count(),
			"Unexpected number of calibrations");
	zassert_true(period < 100, "Unexpected calibration period.");

	k_busy_wait(80000);

	/* Next calibration should happen as scheduled. */
	period = pend_on_next_calibration();
	zassert_equal(cnt + 3, z_nrf_clock_calibration_count(),
				"Unexpected number of calibrations");
	zassert_true(period < 200,
			"Unexpected calibration period %d ms.", period);

}

void test_main(void)
{
	ztest_test_suite(test_nrf_clock_calibration,
		ztest_unit_test(test_clock_calibration),
		ztest_unit_test(test_stopping_when_calibration),
		ztest_unit_test(test_clock_calibration_force)
			 );
	ztest_run_test_suite(test_nrf_clock_calibration);
}
