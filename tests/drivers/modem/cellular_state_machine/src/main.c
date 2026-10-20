/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/ztest.h>

#include <string.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_MODEM_CHAT));
BUILD_ASSERT(IS_ENABLED(CONFIG_MODEM_PIPELINK));
BUILD_ASSERT(!IS_ENABLED(CONFIG_MODEM_CMUX));
BUILD_ASSERT(!IS_ENABLED(CONFIG_MODEM_PPP));
BUILD_ASSERT(!IS_ENABLED(CONFIG_MODEM_BACKEND_UART));

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));
static const struct device *const modem_no_network = DEVICE_DT_GET(DT_NODELABEL(modem_no_network));
static const struct device *const modem_no_baudrate =
	DEVICE_DT_GET(DT_NODELABEL(modem_no_baudrate));
static const struct device *const modem_no_dlci_setup =
	DEVICE_DT_GET(DT_NODELABEL(modem_no_dlci_setup));

MODEM_PIPELINK_DT_DECLARE(DT_ALIAS(modem), user_pipe_0);
MODEM_PIPELINK_DT_DECLARE(DT_ALIAS(modem), user_pipe_1);

/* Answer the first periodic script and check the driver stays registered */
static void run_periodic_script(const struct device *dev)
{
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_state(dev, TEST_BACKEND_STATE_LINK_UP);
}

ZTEST(modem_cellular_state_machine, test_boot_with_network_script)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);
	zassert_equal(CONFIG_MODEM_CELLULAR_NEW_BAUDRATE, uart_baudrate(modem),
		      "UART baudrate was not switched");

	run_periodic_script(modem);
	suspend_and_shutdown(modem);

	/* The modem powers up at its default baudrate, so the UART must be restored first */
	zassert_ok(modem_cellular_pm_action(modem, PM_DEVICE_ACTION_RESUME), "resume failed");
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	zassert_equal(modem_power_up_baudrate(modem), uart_baudrate(modem),
		      "UART baudrate was not restored on power up");
	ok(TEST_BACKEND_PIPE_UART);

	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
	zassert_equal(CONFIG_MODEM_CELLULAR_NEW_BAUDRATE, uart_baudrate(modem),
		      "UART baudrate was not switched");
}

ZTEST(modem_cellular_state_machine, test_boot_without_network_script)
{
	modem_restart(modem_no_network);

	/* Dials straight after the APN script, then waits for registration */
	advance_to(modem_no_network, BOOT_LINK_UP);
	run_periodic_script(modem_no_network);
	suspend_and_shutdown(modem_no_network);
}

ZTEST(modem_cellular_state_machine, test_boot_without_set_baudrate_script)
{
	const uint32_t original = uart_baudrate(modem_no_baudrate);

	modem_restart(modem_no_baudrate);

	/* Boot goes straight to the init script and leaves the UART untouched */
	advance_to(modem_no_baudrate, BOOT_LINK_UP);
	zassert_equal(original, uart_baudrate(modem_no_baudrate), "UART baudrate changed");

	run_periodic_script(modem_no_baudrate);
	suspend_and_shutdown(modem_no_baudrate);
	zassert_equal(original, uart_baudrate(modem_no_baudrate), "UART baudrate changed");
}

ZTEST(modem_cellular_state_machine, test_boot_without_dlci_setup_script)
{
	modem_restart(modem_no_dlci_setup);

	/* The APN command is the first line once the CMUX is up */
	advance_to(modem_no_dlci_setup, BOOT_LINK_UP);
	run_periodic_script(modem_no_dlci_setup);
	suspend_and_shutdown(modem_no_dlci_setup);
}

/* PPP may never report the link dead, so losing registration must not park the driver */
ZTEST(modem_cellular_state_machine, test_deregistration_without_ppp_dead)
{
	modem_restart(modem);
	advance_to(modem, BOOT_DEREGISTERED);

	/* PPP is released once the wait for it times out */
	expect_state(modem, TEST_BACKEND_STATE_CMUX);
	registered(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
	ok(TEST_BACKEND_PIPE_DLCI2);
	expect_state(modem, TEST_BACKEND_STATE_LINK_UP);
}

/* User pipes are connected while the CMUX is up, each carrying its own DLCI */
ZTEST(modem_cellular_state_machine, test_user_pipes)
{
	static const char cmd[] = "AT+USER\r";
	const struct {
		struct modem_pipelink *link;
		enum test_backend_pipe_id pipe;
	} user_pipes[] = {
		{MODEM_PIPELINK_DT_GET(DT_ALIAS(modem), user_pipe_0), TEST_BACKEND_PIPE_USER3},
		{MODEM_PIPELINK_DT_GET(DT_ALIAS(modem), user_pipe_1), TEST_BACKEND_PIPE_USER4},
	};

	modem_restart(modem);
	advance_to(modem, BOOT_CMUX_PENDING);
	for (size_t i = 0; i < ARRAY_SIZE(user_pipes); i++) {
		zassert_false(modem_pipelink_is_connected(user_pipes[i].link),
			      "user pipe %zu connected before the CMUX", i);
	}

	test_backend_cmux_connect_finish(true);
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_DLCI_SETUP_CMD);
	for (size_t i = 0; i < ARRAY_SIZE(user_pipes); i++) {
		struct modem_pipe *pipe = modem_pipelink_get_pipe(user_pipes[i].link);

		zassert_true(modem_pipelink_is_connected(user_pipes[i].link),
			     "user pipe %zu not connected", i);
		zassert_ok(modem_pipe_open(pipe, K_SECONDS(2)));
		zassert_true(test_backend_pipe_is_open(user_pipes[i].pipe));
		zassert_equal(strlen(cmd),
			      modem_pipe_transmit(pipe, (const uint8_t *)cmd, strlen(cmd)));
		expect_line(user_pipes[i].pipe, "AT+USER");
	}

	ok(TEST_BACKEND_PIPE_DLCI1);
	suspend_and_shutdown(modem);
	for (size_t i = 0; i < ARRAY_SIZE(user_pipes); i++) {
		zassert_false(modem_pipelink_is_connected(user_pipes[i].link),
			      "user pipe %zu still connected after suspend", i);
	}
}

ZTEST_SUITE(modem_cellular_state_machine, NULL, modem_test_setup, modem_test_before,
	    modem_test_after, NULL);
