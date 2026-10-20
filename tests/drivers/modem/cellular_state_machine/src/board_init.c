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
#include <zephyr/modem/chat.h>
#include <zephyr/ztest.h>

#define BOARD_INIT_CMD_1 "AT+BOARDINIT=1"
#define BOARD_INIT_CMD_2 "AT+BOARDINIT=2"

static const struct device *const modem_board_init =
	DEVICE_DT_GET(DT_NODELABEL(modem_board_init));

MODEM_CHAT_MATCH_DEFINE(board_init_ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(board_init_abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));
MODEM_CHAT_SCRIPT_CMDS_DEFINE(board_init_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(BOARD_INIT_CMD_1, board_init_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP(BOARD_INIT_CMD_2, board_init_ok_match));
/* The driver replaces the callback with its own */
MODEM_CHAT_SCRIPT_DEFINE(board_init_script, board_init_cmds, board_init_abort_matches, NULL, 1);

MODEM_CELLULAR_BOARD_INIT_DEFINE(DT_NODELABEL(modem_board_init), &board_init_script);

/* Boot until the board init script is about to start, after the DLCI setup */
static void boot_to_board_init(void)
{
	modem_restart(modem_board_init);
	advance_to(modem_board_init, BOOT_DLCI2_SETUP);
	ok(TEST_BACKEND_PIPE_DLCI2);
}

/* Answer the whole board init script */
static void run_board_init(void)
{
	expect_line(TEST_BACKEND_PIPE_DLCI1, BOARD_INIT_CMD_1);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI1, BOARD_INIT_CMD_2);
	ok(TEST_BACKEND_PIPE_DLCI1);
}

/* With the script done, the driver moves on to the APN */
static void expect_apn(void)
{
	expect_read(TEST_BACKEND_PIPE_DLCI1);
	expect_no_line(K_MSEC(100));
	zassert_ok(cellular_set_apn(modem_board_init, TEST_APN));
	expect_line(TEST_BACKEND_PIPE_DLCI1, TEST_APN_CMD);
}

/* The script runs on DLCI1 once the DLCI setup is done, before the APN is configured */
ZTEST(modem_cellular_board_init, test_runs_before_apn)
{
	boot_to_board_init();
	run_board_init();
	expect_apn();
}

/* A failed script is run again from the start after the periodic interval */
ZTEST(modem_cellular_board_init, test_retried)
{
	int64_t start;

	boot_to_board_init();
	expect_line(TEST_BACKEND_PIPE_DLCI1, BOARD_INIT_CMD_1);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_line(TEST_BACKEND_PIPE_DLCI1, BOARD_INIT_CMD_2);

	start = k_uptime_get();
	respond_error(TEST_BACKEND_PIPE_DLCI1);
	expect_no_line(K_MSEC(CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS / 2));
	run_board_init();
	zassert_true(k_uptime_delta(&start) >= CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS,
		     "retried before the periodic interval");
	expect_apn();
}

/* Spending the failure budget powers the modem down */
ZTEST(modem_cellular_board_init, test_failures_shut_down)
{
	boot_to_board_init();
	for (int i = 0; i < CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES; i++) {
		expect_line(TEST_BACKEND_PIPE_DLCI1, BOARD_INIT_CMD_1);
		respond_error(TEST_BACKEND_PIPE_DLCI1);
	}

	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	expect_state(modem_board_init, TEST_BACKEND_STATE_OFF);
	expect_no_line(K_MSEC(CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS * 2));
}

ZTEST(modem_cellular_board_init, test_suspend)
{
	boot_to_board_init();
	expect_line(TEST_BACKEND_PIPE_DLCI1, BOARD_INIT_CMD_1);
	suspend_and_shutdown(modem_board_init);
}

ZTEST_SUITE(modem_cellular_board_init, NULL, modem_test_setup, modem_test_before,
	    modem_test_after, NULL);
