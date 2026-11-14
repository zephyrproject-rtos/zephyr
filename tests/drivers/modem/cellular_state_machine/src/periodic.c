/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/ztest.h>

/* Long enough for at least one periodic run to fall due */
#define MISSED_RUN_WINDOW K_MSEC(2 * CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS)

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));
static const struct device *const modem_no_periodic =
	DEVICE_DT_GET(DT_NODELABEL(modem_no_periodic));

/* Resume the paused periodic script, which runs at once as a run was skipped */
static void resume_skipped(const struct device *dev)
{
	int64_t start;

	start = k_uptime_get();
	zassert_ok(cellular_modem_resume_periodic_script(dev));
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	zassert_true(k_uptime_delta(&start) < CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS,
		     "skipped run waited for the next interval");
	ok(TEST_BACKEND_PIPE_DLCI1);
}

ZTEST(modem_cellular_periodic, test_pause_resume)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	zassert_ok(cellular_modem_pause_periodic_script(modem));
	zassert_equal(-EINVAL, cellular_modem_pause_periodic_script(modem),
		      "paused an already paused script");
	expect_no_line(MISSED_RUN_WINDOW);
	expect_state(modem, TEST_BACKEND_STATE_LINK_UP);

	resume_skipped(modem);
	zassert_equal(-EINVAL, cellular_modem_resume_periodic_script(modem),
		      "resumed a script that was not paused");

	/* Back to the regular interval */
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_state(modem, TEST_BACKEND_STATE_LINK_UP);
}

/* Without a skipped run, resuming does not run the script early */
ZTEST(modem_cellular_periodic, test_resume_before_due)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	zassert_ok(cellular_modem_pause_periodic_script(modem));
	zassert_ok(cellular_modem_resume_periodic_script(modem));
	expect_no_line(K_MSEC(CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS / 2));

	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
}

/* A run already in flight completes, the pause applies from the next one */
ZTEST(modem_cellular_periodic, test_pause_in_flight)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	zassert_ok(cellular_modem_pause_periodic_script(modem));
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_read(TEST_BACKEND_PIPE_DLCI1);

	expect_no_line(MISSED_RUN_WINDOW);
	expect_state(modem, TEST_BACKEND_STATE_LINK_UP);
	resume_skipped(modem);
}

/* The periodic script also runs while registration is awaited, and pauses there too */
ZTEST(modem_cellular_periodic, test_pause_awaiting_registration)
{
	modem_restart(modem);
	advance_to(modem, BOOT_AWAIT_REGISTRATION);

	zassert_ok(cellular_modem_pause_periodic_script(modem));
	expect_no_line(MISSED_RUN_WINDOW);
	resume_skipped(modem);

	registered(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
}

ZTEST(modem_cellular_periodic, test_no_periodic_script)
{
	modem_restart(modem_no_periodic);

	zassert_equal(-ENOTSUP, cellular_modem_pause_periodic_script(modem_no_periodic));
	zassert_equal(-ENOTSUP, cellular_modem_resume_periodic_script(modem_no_periodic));

	advance_to(modem_no_periodic, BOOT_LINK_UP);
	expect_no_line(MISSED_RUN_WINDOW);
	expect_state(modem_no_periodic, TEST_BACKEND_STATE_LINK_UP);
	suspend_and_shutdown(modem_no_periodic);
}

ZTEST_SUITE(modem_cellular_periodic, NULL, modem_test_setup, modem_test_before, modem_test_after,
	    NULL);
