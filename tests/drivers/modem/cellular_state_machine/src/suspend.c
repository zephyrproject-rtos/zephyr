/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/ztest.h>

static const struct device *const modem_slow = DEVICE_DT_GET(DT_NODELABEL(modem_slow));
static const struct device *const modem_no_power = DEVICE_DT_GET(DT_NODELABEL(modem_no_power));

/* Suspend completed, leaving both control lines released and the modem silent */
static void finish_clean_suspend(const struct device *dev)
{
	const struct gpio_dt_spec *power_gpio = test_backend_power_gpio(dev);
	const struct gpio_dt_spec *reset_gpio = test_backend_reset_gpio(dev);

	finish_suspend(dev);
	if (power_gpio->port != NULL) {
		zassert_equal(0, gpio_emul_output_get_dt(power_gpio), "power key left asserted");
	}
	if (reset_gpio->port != NULL) {
		zassert_equal(0, gpio_emul_output_get_dt(reset_gpio), "reset left asserted");
	}
	expect_no_line(K_MSEC(100));
}

/* Suspend from @p target, answering the shutdown script when the modem is running */
static void suspend_from(enum boot_step target, bool shutdown_script)
{
	modem_restart(modem_slow);
	advance_to(modem_slow, target);

	start_suspend(modem_slow);
	if (shutdown_script) {
		expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
		ok(TEST_BACKEND_PIPE_UART);
	}
	finish_clean_suspend(modem_slow);
}

/* Assert that @p dev stays powered down, as a leftover timer would restart it */
static void expect_stays_idle(const struct device *dev)
{
	const struct gpio_dt_spec *power_gpio = test_backend_power_gpio(dev);
	const struct gpio_dt_spec *reset_gpio = test_backend_reset_gpio(dev);
	/* Outlasts the longest recovery backoff the driver can have pending */
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(CONFIG_MODEM_CELLULAR_RECOVERY_BACKOFF_MS *
						      (CONFIG_MODEM_CELLULAR_MAX_RECOVERIES + 1)));

	while (!sys_timepoint_expired(end)) {
		zassert_equal(TEST_BACKEND_STATE_OFF, test_backend_state(dev),
			      "restarted into state %d", test_backend_state(dev));
		if (power_gpio->port != NULL) {
			zassert_equal(0, gpio_emul_output_get_dt(power_gpio), "power key pressed");
		}
		if (reset_gpio->port != NULL) {
			zassert_equal(0, gpio_emul_output_get_dt(reset_gpio), "reset asserted");
		}
		k_msleep(1);
	}
	expect_no_line(K_NO_WAIT);
}

/* The init script failed, so the modem is not answering and is left as it is */
ZTEST(modem_cellular_suspend, test_from_recovery)
{
	suspend_from(BOOT_RECOVERY, false);
	expect_stays_idle(modem_slow);
}

/* The modem is likely still in CMUX mode, so power it off as test_from_connect_cmux_failed */
ZTEST(modem_cellular_suspend, test_from_recovery_after_cmux_failure)
{
	modem_restart(modem_slow);
	advance_to(modem_slow, BOOT_CMUX_PENDING);
	test_backend_cmux_connect_finish(false);

	start_suspend(modem_slow);
	expect_gpio(test_backend_power_gpio(modem_slow), 1);
	finish_clean_suspend(modem_slow);
	expect_stays_idle(modem_slow);
}

/* Without a power key the shutdown script is tried over the UART instead */
ZTEST(modem_cellular_suspend, test_from_recovery_after_cmux_failure_no_power_key)
{
	modem_restart(modem_no_power);
	advance_to(modem_no_power, BOOT_CMUX_PENDING);
	test_backend_cmux_connect_finish(false);

	start_suspend(modem_no_power);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	finish_clean_suspend(modem_no_power);
	expect_stays_idle(modem_no_power);
}

/* Resuming starts afresh, with the full recovery budget */
ZTEST(modem_cellular_suspend, test_resume_from_recovery)
{
	suspend_from(BOOT_RECOVERY, false);
	zassert_ok(modem_cellular_pm_action(modem_slow, PM_DEVICE_ACTION_RESUME), "resume failed");

	for (int i = 0; i < CONFIG_MODEM_CELLULAR_MAX_RECOVERIES; i++) {
		expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
		ok(TEST_BACKEND_PIPE_UART);
		expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
		respond_error(TEST_BACKEND_PIPE_UART);
	}
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
}

ZTEST(modem_cellular_suspend, test_from_reset_pulse)
{
	suspend_from(BOOT_RESET_PULSE, false);
}

ZTEST(modem_cellular_suspend, test_from_reset_released)
{
	suspend_from(BOOT_RESET_RELEASED, false);
}

ZTEST(modem_cellular_suspend, test_from_power_key)
{
	suspend_from(BOOT_POWER_KEY, false);
}

ZTEST(modem_cellular_suspend, test_from_uart_opened)
{
	suspend_from(BOOT_UART_OPENED, false);
}

ZTEST(modem_cellular_suspend, test_from_set_baudrate)
{
	suspend_from(BOOT_SET_BAUDRATE, false);
}

ZTEST(modem_cellular_suspend, test_from_init)
{
	suspend_from(BOOT_INIT, false);
}

/* A suspend while the CMUX connects waits for it, then runs the shutdown script */
ZTEST(modem_cellular_suspend, test_from_cmux_pending)
{
	modem_restart(modem_slow);
	advance_to(modem_slow, BOOT_CMUX_PENDING);

	start_suspend(modem_slow);
	test_backend_cmux_connect_finish(true);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	finish_clean_suspend(modem_slow);
}

/* A failed CMUX connect is a fault, recovered by power cycling the modem */
ZTEST(modem_cellular_suspend, test_cmux_connect_failure)
{
	modem_restart(modem_slow);
	advance_to(modem_slow, BOOT_CMUX_PENDING);

	test_backend_cmux_connect_finish(false);
	expect_gpio(test_backend_power_gpio(modem_slow), 1);
}

/* A suspend waiting on a failed CMUX connect powers off with the power key */
ZTEST(modem_cellular_suspend, test_from_connect_cmux_failed)
{
	modem_restart(modem_slow);
	advance_to(modem_slow, BOOT_CMUX_PENDING);

	start_suspend(modem_slow);
	test_backend_cmux_connect_finish(false);
	expect_gpio(test_backend_power_gpio(modem_slow), 1);
	finish_clean_suspend(modem_slow);
}

/* Without a power key the shutdown script is tried over the UART instead */
ZTEST(modem_cellular_suspend, test_from_connect_cmux_failed_no_power_key)
{
	modem_restart(modem_no_power);
	advance_to(modem_no_power, BOOT_CMUX_PENDING);

	start_suspend(modem_no_power);
	test_backend_cmux_connect_finish(false);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	finish_clean_suspend(modem_no_power);
}

ZTEST(modem_cellular_suspend, test_from_dlci1_setup)
{
	suspend_from(BOOT_DLCI1_SETUP, true);
}

ZTEST(modem_cellular_suspend, test_from_dlci2_setup)
{
	suspend_from(BOOT_DLCI2_SETUP, true);
}

ZTEST(modem_cellular_suspend, test_from_await_apn)
{
	suspend_from(BOOT_AWAIT_APN, true);
}

ZTEST(modem_cellular_suspend, test_from_apn)
{
	suspend_from(BOOT_APN, true);
}

ZTEST(modem_cellular_suspend, test_from_network)
{
	suspend_from(BOOT_NETWORK, true);
}

ZTEST(modem_cellular_suspend, test_from_await_registration)
{
	suspend_from(BOOT_AWAIT_REGISTRATION, true);
}

ZTEST(modem_cellular_suspend, test_from_dial)
{
	suspend_from(BOOT_DIAL, true);
}

ZTEST(modem_cellular_suspend, test_from_deregistered)
{
	suspend_from(BOOT_DEREGISTERED, true);
}

/* Suspend from link up, then again once idle */
ZTEST(modem_cellular_suspend, test_from_idle)
{
	suspend_from(BOOT_LINK_UP, true);

	/* Already suspended, so this must return without touching the modem */
	start_suspend(modem_slow);
	finish_clean_suspend(modem_slow);
}

/*
 * The driver powers the modem down by itself once a script exceeds its
 * failure budget. A suspend request arriving part way through must still complete
 * once the modem reaches idle.
 */
static void begin_internal_shutdown(void)
{
	modem_restart(modem_slow);
	advance_to(modem_slow, BOOT_APN);

	for (int i = 0; i < CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES; i++) {
		if (i > 0) {
			expect_line(TEST_BACKEND_PIPE_DLCI1, TEST_APN_CMD);
		}
		respond_error(TEST_BACKEND_PIPE_DLCI1);
	}
}

ZTEST(modem_cellular_suspend, test_from_cmux_disconnected)
{
	begin_internal_shutdown();

	/* CMUX disconnected, waiting for the modem to return to AT mode */
	expect_state(modem_slow, TEST_BACKEND_STATE_AT);

	start_suspend(modem_slow);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	finish_clean_suspend(modem_slow);
}

ZTEST(modem_cellular_suspend, test_from_shutdown_cmd)
{
	begin_internal_shutdown();
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);

	start_suspend(modem_slow);
	ok(TEST_BACKEND_PIPE_UART);
	finish_clean_suspend(modem_slow);
}

ZTEST(modem_cellular_suspend, test_from_power_off_key)
{
	begin_internal_shutdown();
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	respond_error(TEST_BACKEND_PIPE_UART);
	expect_gpio(test_backend_power_gpio(modem_slow), 1);

	start_suspend(modem_slow);
	finish_clean_suspend(modem_slow);
}

ZTEST(modem_cellular_suspend, test_from_power_off_released)
{
	begin_internal_shutdown();
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	respond_error(TEST_BACKEND_PIPE_UART);
	expect_gpio(test_backend_power_gpio(modem_slow), 1);
	expect_gpio(test_backend_power_gpio(modem_slow), 0);

	start_suspend(modem_slow);
	finish_clean_suspend(modem_slow);
}

ZTEST_SUITE(modem_cellular_suspend, NULL, modem_test_setup, modem_test_before, modem_test_after,
	    NULL);
