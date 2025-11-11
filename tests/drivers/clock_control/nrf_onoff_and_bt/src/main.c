/*
 * Copyright (c) 2019-2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define TEST_TIME_MS 10000

#ifdef CONFIG_SOC_SERIES_NRF54LX
#define HF_STARTUP_TIME_US 600
#else
#define HF_STARTUP_TIME_US 400
#endif

static bool test_end;

static const struct device *const entropy = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
static const struct device *const clock_dev = DEVICE_DT_GET_ONE(nordic_nrf_clock);
static struct onoff_manager *hf_mgr;
static struct onoff_client cli;
static uint32_t iteration;

static void *setup(void)
{
	zassert_true(device_is_ready(entropy));
	zassert_true(device_is_ready(clock_dev));

	hf_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	zassert_true(hf_mgr);

	return NULL;
}

static void bt_timeout_handler(struct k_timer *timer)
{
	static bool on;

	if (on) {
		on = false;
		z_nrf_clock_bt_ctlr_hf_release();
	} else {
		on = true;
		z_nrf_clock_bt_ctlr_hf_request();
	}

	if (!(test_end && !on)) {
		k_timeout_t timeout;
		static bool long_timeout;

		if (!on) {
			timeout = K_USEC(200);
		} else {
			timeout = long_timeout ? K_USEC(300) : K_USEC(100);
			long_timeout = !long_timeout;
		}
		k_timer_start(timer, timeout, K_NO_WAIT);
	}
}

K_TIMER_DEFINE(timer1, bt_timeout_handler, NULL);

static void check_hf_status(const struct device *dev, bool exp_on,
			    bool sw_check)
{
	uint32_t key = irq_lock();
	nrf_clock_hfclk_t type;

	nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_HFCLK, &type);
	zassert_equal(type, exp_on ? NRF_CLOCK_HFCLK_HIGH_ACCURACY :
				NRF_CLOCK_HFCLK_LOW_ACCURACY,
			"%d: Clock expected to be %s",
			iteration, exp_on ? "on" : "off");

	if (sw_check) {
		enum clock_control_status status =
		     clock_control_get_status(dev, CLOCK_CONTROL_NRF_SUBSYS_HF);

		zassert_equal(status, exp_on ? CLOCK_CONTROL_STATUS_ON :
						CLOCK_CONTROL_STATUS_OFF,
				"%d: Unexpected status: %d", iteration, status);
	}

	irq_unlock(key);
}

/* Test controls HF clock from two contexts: thread and timer interrupt.
 * In thread context clock is requested and released through standard onoff
 * API and in the timeout handler it is requested and released using API
 * dedicated to be used by Bluetooth Controller.
 *
 * Test runs in the loop to eventually lead to cases when clock controlling is
 * preempted by timeout handler. At certain points clock status is validated.
 */
ZTEST(nrf_onoff_and_bt, test_onoff_interrupted)
{
	uint64_t start_time = k_uptime_get();
	uint64_t elapsed;
	uint64_t checkpoint = 1000;
	int err;
	uint8_t rand;
	int backoff;

	iteration = 0;
	test_end = false;

	k_timer_start(&timer1, K_MSEC(1), K_NO_WAIT);

	do {
		iteration++;

		err = entropy_get_entropy(entropy, &rand, 1);
		zassert_equal(err, 0);
		backoff = 3 * rand;

		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(hf_mgr, &cli);
		zassert_true(err >= 0);

		k_busy_wait(backoff);

		if (backoff > HF_STARTUP_TIME_US) {
			check_hf_status(clock_dev, true, true);
		}

		err = onoff_cancel_or_release(hf_mgr, &cli);
		zassert_true(err >= 0);

		elapsed = k_uptime_get() - start_time;
		if (elapsed > checkpoint) {
			printk("test continues\n");
			checkpoint += 1000;
		}
	} while (elapsed <= TEST_TIME_MS);

	test_end = true;
	k_msleep(100);
	check_hf_status(clock_dev, false, true);
}

static void onoff_timeout_handler(struct k_timer *timer)
{
	static bool on;
	static uint32_t cnt;
	int err;

	cnt++;
	if (on) {
		on = false;
		err = onoff_cancel_or_release(hf_mgr, &cli);
		zassert_true(err >= 0);
	} else {
		on = true;
		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(hf_mgr, &cli);
		zassert_true(err >= 0, "%d: Unexpected err: %d", cnt, err);
	}

	if (!(test_end && !on)) {
		k_timeout_t timeout;
		static bool long_timeout;

		if (!on) {
			timeout = K_USEC(200);
		} else {
			timeout = long_timeout ? K_USEC(300) : K_USEC(100);
			long_timeout = !long_timeout;
		}
		k_timer_start(timer, timeout, K_NO_WAIT);
	}
}

K_TIMER_DEFINE(timer2, onoff_timeout_handler, NULL);

/* Test controls HF clock from two contexts: thread and timer interrupt.
 * In thread context clock is requested and released through API
 * dedicated to be used by Bluetooth Controller and in the timeout handler it is
 * requested and released using standard onoffAPI .
 *
 * Test runs in the loop to eventually lead to cases when clock controlling is
 * preempted by timeout handler. At certain points clock status is validated.
 */
ZTEST(nrf_onoff_and_bt, test_bt_interrupted)
{
	uint64_t start_time = k_uptime_get();
	uint64_t elapsed;
	uint64_t checkpoint = 1000;
	int err;
	uint8_t rand;
	int backoff;

	iteration = 0;
	test_end = false;

	k_timer_start(&timer2, K_MSEC(1), K_NO_WAIT);

	do {
		iteration++;

		err = entropy_get_entropy(entropy, &rand, 1);
		zassert_equal(err, 0);
		backoff = 3 * rand;

		z_nrf_clock_bt_ctlr_hf_request();

		k_busy_wait(backoff);

		if (backoff > HF_STARTUP_TIME_US) {
			check_hf_status(clock_dev, true, false);
		}

		z_nrf_clock_bt_ctlr_hf_release();

		elapsed = k_uptime_get() - start_time;
		if (elapsed > checkpoint) {
			printk("test continues\n");
			checkpoint += 1000;
		}
	} while (elapsed <= TEST_TIME_MS);

	test_end = true;
	k_msleep(100);
	check_hf_status(clock_dev, false, true);
}

ZTEST(nrf_onoff_and_bt, test_onoff_following_bt)
{
	int err;
	int res = -EIO;

	z_nrf_clock_bt_ctlr_hf_request();

	/* First start can take longer on some platforms due to the tuning. */
	k_busy_wait(HF_STARTUP_TIME_US + 6000);
	check_hf_status(clock_dev, true, false);

	z_nrf_clock_bt_ctlr_hf_release();

	for (int i = 0; i < 5; i++) {
		z_nrf_clock_bt_ctlr_hf_request();

		k_busy_wait(HF_STARTUP_TIME_US + 1200);
		check_hf_status(clock_dev, true, false);

		z_nrf_clock_bt_ctlr_hf_release();

		check_hf_status(clock_dev, false, false);

		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(hf_mgr, &cli);
		zassert_true(err >= 0, "Unexpected err: %d", err);

		k_busy_wait(HF_STARTUP_TIME_US);
		err = sys_notify_fetch_result(&cli.notify, &res);
		zassert_equal(err, 0, "Unexpected err: %d", err);
		zassert_equal(res, 0, "Unexpected result: %d", res);
		check_hf_status(clock_dev, true, false);

		err = onoff_release(hf_mgr);
		zassert_true(err >= 0, "Unexpected err: %d", err);

		check_hf_status(clock_dev, false, false);
	}
}

ZTEST_SUITE(nrf_onoff_and_bt, NULL, setup, NULL, NULL, NULL);
