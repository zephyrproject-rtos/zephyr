/*
 * Copyright (c) 2019-2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define TEST_TIME_MS 10000

#define HF_STARTUP_TIME_US 400

static bool test_end;

#include <hal/nrf_gpio.h>

static const struct device *entropy = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
static struct onoff_manager *hf_mgr;
static uint32_t iteration;

static void setup(void)
{
	zassert_true(device_is_ready(entropy), NULL);

	hf_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	zassert_true(hf_mgr, NULL);

	iteration = 0;
}

static void teardown(void)
{
	/* empty */
}

static void bt_timeout_handler(struct k_timer *timer)
{
	static bool on;

	nrf_gpio_cfg_output(27);
	if (on) {
		on = false;
		nrf_gpio_pin_clear(27);
		z_nrf_clock_bt_ctlr_hf_release();
	} else {
		nrf_gpio_pin_set(27);
		on = true;
		z_nrf_clock_bt_ctlr_hf_request();
	}

	if (!(test_end && !on)) {
		k_timeout_t timeout;
		static bool long_timeout;

		if (!on) {
			timeout = Z_TIMEOUT_US(200);
		} else {
			timeout = Z_TIMEOUT_US(long_timeout ? 300 : 100);
			long_timeout = !long_timeout;
		}
		k_timer_start(timer, timeout, K_NO_WAIT);
	}
}

K_TIMER_DEFINE(timer1, bt_timeout_handler, NULL);

static void check_hf_status(const struct device *dev, bool exp_on,
			    bool sw_check)
{
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

}

/* Test controls HF clock from two contexts: thread and timer interrupt.
 * In thread context clock is requested and released through standard onoff
 * API and in the timeout handler it is requested and released using API
 * dedicated to be used by Bluetooth Controller.
 *
 * Test runs in the loop to eventually lead to cases when clock controlling is
 * preempted by timeout handler. At certain points clock status is validated.
 */
static void test_onoff_interrupted(void)
{
	const struct device *clock_dev = DEVICE_DT_GET_ONE(nordic_nrf_clock);
	struct onoff_client cli;
	uint64_t start_time = k_uptime_get();
	uint64_t elapsed;
	uint64_t checkpoint = 1000;
	int err;
	uint8_t rand;
	int backoff;

	zassert_true(device_is_ready(clock_dev), "Device is not ready");

	k_timer_start(&timer1, K_MSEC(1), K_NO_WAIT);

	while (1) {
		iteration++;

		err = entropy_get_entropy(entropy, &rand, 1);
		zassert_equal(err, 0, NULL);
		backoff = 3 * rand;

		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(hf_mgr, &cli);
		zassert_true(err >= 0, NULL);

		k_busy_wait(backoff);

		if (backoff > HF_STARTUP_TIME_US) {
			check_hf_status(clock_dev, true, true);
		}

		err = onoff_cancel_or_release(hf_mgr, &cli);
		zassert_true(err >= 0, NULL);

		elapsed = k_uptime_get() - start_time;
		if (elapsed > checkpoint) {
			printk("test continues\n");
			checkpoint += 1000;
		}

		if (elapsed > TEST_TIME_MS) {
			test_end = true;
			break;
		}
	}

	k_msleep(100);
	check_hf_status(clock_dev, false, true);
}

static void onoff_timeout_handler(struct k_timer *timer)
{
	static bool on;
	static struct onoff_client cli;
	static uint32_t cnt;
	int err;

	cnt++;
	if (on) {
		on = false;
		err = onoff_cancel_or_release(hf_mgr, &cli);
		zassert_true(err >= 0, NULL);
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
			timeout = Z_TIMEOUT_US(200);
		} else {
			timeout = Z_TIMEOUT_US(long_timeout ? 300 : 100);
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
static void test_bt_interrupted(void)
{
	const struct device *clock_dev = DEVICE_DT_GET_ONE(nordic_nrf_clock);
	uint64_t start_time = k_uptime_get();
	uint64_t elapsed;
	uint64_t checkpoint = 1000;
	int err;
	uint8_t rand;
	int backoff;

	zassert_true(device_is_ready(clock_dev), "Device is not ready");

	k_timer_start(&timer2, K_MSEC(1), K_NO_WAIT);

	while (1) {
		iteration++;

		err = entropy_get_entropy(entropy, &rand, 1);
		zassert_equal(err, 0, NULL);
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

		if (elapsed > TEST_TIME_MS) {
			test_end = true;
			break;
		}
	}

	k_msleep(100);
	check_hf_status(clock_dev, false, true);
}

void test_main(void)
{
	ztest_test_suite(test_nrf_onoff_and_bt,
		ztest_unit_test_setup_teardown(test_onoff_interrupted,
					       setup, teardown),
		ztest_unit_test_setup_teardown(test_bt_interrupted,
					       setup, teardown)
			);
	ztest_run_test_suite(test_nrf_onoff_and_bt);
}
