/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/ztest.h>

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));
static const struct device *const modem_no_power = DEVICE_DT_GET(DT_NODELABEL(modem_no_power));

struct test_event {
	enum cellular_event event;
	bool success;
};

K_MSGQ_DEFINE(test_events, sizeof(struct test_event), 16, 4);

static void event_handler(const struct device *dev, enum cellular_event event,
			  const void *payload, void *user_data)
{
	struct test_event evt = {.event = event};

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (event == CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT) {
		const struct cellular_evt_modem_comms_check_result *result = payload;

		evt.success = result->success;
	}
	(void)k_msgq_put(&test_events, &evt, K_NO_WAIT);
}

static void listen_for_events(const struct device *dev, cellular_event_mask_t mask)
{
	k_msgq_purge(&test_events);
	zassert_ok(cellular_set_callback(dev, mask, event_handler, NULL));
}

static struct test_event expect_event(enum cellular_event event)
{
	struct test_event evt;

	zassert_ok(k_msgq_get(&test_events, &evt, K_SECONDS(2)), "timed out waiting for event 0x%x",
		   event);
	zassert_equal(event, evt.event, "got event 0x%x, expected 0x%x", evt.event, event);
	return evt;
}

static void expect_comms_check_result(bool success)
{
	struct test_event evt = expect_event(CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT);

	zassert_equal(success, evt.success, "comms check reported success=%d", evt.success);
}

/*
 * Fail a script that is retried on a timer until its failure budget is spent. The
 * first attempt has already been sent. The driver then powers the modem down.
 */
static void fail_until_shutdown(const struct device *dev, enum test_backend_pipe_id pipe,
				const char *cmd)
{
	listen_for_events(dev, CELLULAR_EVENT_MODEM_SUSPENDED);

	for (int i = 0; i < CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES; i++) {
		if (i > 0) {
			expect_line(pipe, cmd);
		}
		respond_error(pipe);
	}

	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	expect_event(CELLULAR_EVENT_MODEM_SUSPENDED);
	expect_state(dev, TEST_BACKEND_STATE_OFF);
	expect_no_line(K_MSEC(100));
}

/* A modem that saved a previous baudrate change will not answer at the old one */
ZTEST(modem_cellular_script_errors, test_set_baudrate)
{
	modem_restart(modem);
	advance_to(modem, BOOT_SET_BAUDRATE);

	respond_error(TEST_BACKEND_PIPE_UART);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
}

/* Fail the init script @p count times, checking each failure restarts the modem */
static void fail_init_and_recover(int count)
{
	for (int i = 0; i < count; i++) {
		expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
		respond_error(TEST_BACKEND_PIPE_UART);
		expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
		ok(TEST_BACKEND_PIPE_UART);
	}
}

/* A boot that recovers within the budget carries on to CMUX */
ZTEST(modem_cellular_script_errors, test_init_recovers)
{
	modem_restart(modem);
	advance_to(modem, BOOT_SET_BAUDRATE);
	ok(TEST_BACKEND_PIPE_UART);
	fail_init_and_recover(CONFIG_MODEM_CELLULAR_MAX_RECOVERIES);

	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_DLCI_SETUP_CMD);
}

ZTEST(modem_cellular_script_errors, test_init_exhausts_recoveries)
{
	modem_restart(modem);
	advance_to(modem, BOOT_SET_BAUDRATE);
	ok(TEST_BACKEND_PIPE_UART);
	listen_for_events(modem, CELLULAR_EVENT_MODEM_SUSPENDED);
	fail_init_and_recover(CONFIG_MODEM_CELLULAR_MAX_RECOVERIES);

	/* Parked in idle rather than resetting forever */
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
	respond_error(TEST_BACKEND_PIPE_UART);
	expect_event(CELLULAR_EVENT_MODEM_SUSPENDED);
	expect_state(modem, TEST_BACKEND_STATE_OFF);
	expect_no_line(K_MSEC(CONFIG_MODEM_CELLULAR_RECOVERY_BACKOFF_MS * 2));

	/* Parking restores the full recovery budget */
	zassert_ok(modem_cellular_pm_action(modem, PM_DEVICE_ACTION_RESUME), "resume failed");
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	fail_init_and_recover(CONFIG_MODEM_CELLULAR_MAX_RECOVERIES);
}

/* DLCI setup failures are not critical, so bring-up continues */
ZTEST(modem_cellular_script_errors, test_dlci_setup)
{
	modem_restart(modem);
	advance_to(modem, BOOT_DLCI1_SETUP);

	respond_error(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DLCI_SETUP_CMD);
	respond_error(TEST_BACKEND_PIPE_DLCI2);
	expect_read(TEST_BACKEND_PIPE_DLCI2);
	zassert_ok(cellular_set_apn(modem, TEST_APN));
	expect_line(TEST_BACKEND_PIPE_DLCI1, TEST_APN_CMD);
}

ZTEST(modem_cellular_script_errors, test_apn_retries)
{
	modem_restart(modem);
	advance_to(modem, BOOT_APN);

	respond_error(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI1, TEST_APN_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_NETWORK_CMD);
}

ZTEST(modem_cellular_script_errors, test_apn_shuts_down)
{
	modem_restart(modem);
	advance_to(modem, BOOT_APN);

	fail_until_shutdown(modem, TEST_BACKEND_PIPE_DLCI1, TEST_APN_CMD);
}

ZTEST(modem_cellular_script_errors, test_network_retries)
{
	modem_restart(modem);
	advance_to(modem, BOOT_NETWORK);

	respond_error(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_NETWORK_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
	registered(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
}

ZTEST(modem_cellular_script_errors, test_network_shuts_down)
{
	modem_restart(modem);
	advance_to(modem, BOOT_NETWORK);

	fail_until_shutdown(modem, TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_NETWORK_CMD);
}

ZTEST(modem_cellular_script_errors, test_dial_retries)
{
	modem_restart(modem);
	advance_to(modem, BOOT_DIAL);

	respond_error(TEST_BACKEND_PIPE_DLCI2);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
	ok(TEST_BACKEND_PIPE_DLCI2);
	expect_state(modem, TEST_BACKEND_STATE_LINK_UP);
}

ZTEST(modem_cellular_script_errors, test_dial_shuts_down)
{
	modem_restart(modem);
	advance_to(modem, BOOT_DIAL);

	fail_until_shutdown(modem, TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
}

static void fail_periodic(int count)
{
	for (int i = 0; i < count; i++) {
		expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
		respond_error(TEST_BACKEND_PIPE_DLCI1);
		expect_comms_check_result(false);
	}
}

static void pass_periodic(void)
{
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_comms_check_result(true);
}

/* A success between failures resets the budget, so the link stays up */
ZTEST(modem_cellular_script_errors, test_periodic_reports_failures)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);
	listen_for_events(modem, CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT);

	fail_periodic(CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES - 1);
	pass_periodic();
	fail_periodic(CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES - 1);
	pass_periodic();
	zassert_equal(TEST_BACKEND_STATE_LINK_UP, test_backend_state(modem));
}

/* Spending the budget restarts the modem, without powering it down first */
ZTEST(modem_cellular_script_errors, test_periodic_restarts_modem)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);
	listen_for_events(modem, CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT);

	fail_periodic(CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
}

ZTEST(modem_cellular_script_errors, test_periodic_restarts_modem_while_awaiting_registration)
{
	modem_restart(modem);
	advance_to(modem, BOOT_AWAIT_REGISTRATION);

	/* Comms check results are only reported once registered */
	for (int i = 0; i < CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES; i++) {
		expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
		respond_error(TEST_BACKEND_PIPE_DLCI1);
	}
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
}

/* With a power key the failure falls back to it, see test_shutdown_script_failure */
ZTEST(modem_cellular_script_errors, test_shutdown_without_power_key)
{
	modem_restart(modem_no_power);
	advance_to(modem_no_power, BOOT_LINK_UP);

	start_suspend(modem_no_power);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	respond_error(TEST_BACKEND_PIPE_UART);
	finish_suspend(modem_no_power);
	expect_no_line(K_MSEC(100));
}

static void script_errors_after(void *fixture)
{
	(void)cellular_set_callback(modem, 0, NULL, NULL);
	(void)cellular_set_callback(modem_no_power, 0, NULL, NULL);
	modem_test_after(fixture);
}

ZTEST_SUITE(modem_cellular_script_errors, NULL, modem_test_setup, modem_test_before,
	    script_errors_after, NULL);
