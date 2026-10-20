/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/ztest.h>

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));

static K_THREAD_STACK_DEFINE(signal_stack, 1024);
static struct k_thread signal_thread;
static int signal_ret;
static int16_t signal_value;

static void signal_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg3);

	signal_ret = cellular_get_signal(arg1, (enum cellular_signal_type)POINTER_TO_INT(arg2),
					 &signal_value);
}

/* Query @p type from a separate thread, as the query blocks until the modem answers */
static void start_get_signal(enum cellular_signal_type type)
{
	signal_ret = -EINPROGRESS;
	k_thread_create(&signal_thread, signal_stack, K_THREAD_STACK_SIZEOF(signal_stack),
			signal_entry, (void *)modem, INT_TO_POINTER(type), NULL,
			CONFIG_ZTEST_THREAD_PRIORITY - 1, 0, K_NO_WAIT);
}

/* Wait for the query started by start_get_signal() and return its result */
static int finish_get_signal(void)
{
	zassert_ok(k_thread_join(&signal_thread, K_SECONDS(3)), "signal query did not complete");
	return signal_ret;
}

/* Query @p type, answering the command @p cmd with @p response */
static int get_signal(enum cellular_signal_type type, const char *cmd, const char *response)
{
	start_get_signal(type);
	expect_line(TEST_BACKEND_PIPE_DLCI1, cmd);
	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, response);
	ok(TEST_BACKEND_PIPE_DLCI1);
	return finish_get_signal();
}

/* Boot to link up. Each query then runs well inside the first periodic interval, so the
 * chat instance is free.
 */
static void link_up(void)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);
}

ZTEST(modem_cellular_signal, test_rssi)
{
	link_up();
	zassert_ok(get_signal(CELLULAR_SIGNAL_RSSI, "AT+CSQ", "+CSQ: 20,99\r"));
	zassert_equal(-73, signal_value, "RSSI %d dBm", signal_value);
}

ZTEST(modem_cellular_signal, test_rssi_unknown)
{
	link_up();
	zassert_equal(-EINVAL, get_signal(CELLULAR_SIGNAL_RSSI, "AT+CSQ", "+CSQ: 99,99\r"));
}

/* A response without the bit error rate leaves the previous RSSI in place */
ZTEST(modem_cellular_signal, test_rssi_malformed)
{
	link_up();
	zassert_ok(get_signal(CELLULAR_SIGNAL_RSSI, "AT+CSQ", "+CSQ: 20,99\r"));
	zassert_ok(get_signal(CELLULAR_SIGNAL_RSSI, "AT+CSQ", "+CSQ: 5\r"));
	zassert_equal(-73, signal_value, "RSSI %d dBm", signal_value);
}

ZTEST(modem_cellular_signal, test_rsrp_rsrq)
{
	link_up();
	zassert_ok(get_signal(CELLULAR_SIGNAL_RSRP, "AT+CESQ", "+CESQ: 99,99,255,255,20,50\r"));
	zassert_equal(-90, signal_value, "RSRP %d dBm", signal_value);
	zassert_ok(get_signal(CELLULAR_SIGNAL_RSRQ, "AT+CESQ", "+CESQ: 99,99,255,255,20,50\r"));
	zassert_equal(-10, signal_value, "RSRQ %d dB", signal_value);
}

ZTEST(modem_cellular_signal, test_rsrp_rsrq_unknown)
{
	link_up();
	zassert_equal(-EINVAL, get_signal(CELLULAR_SIGNAL_RSRP, "AT+CESQ",
					  "+CESQ: 99,99,255,255,255,255\r"));
	zassert_equal(-EINVAL, get_signal(CELLULAR_SIGNAL_RSRQ, "AT+CESQ",
					  "+CESQ: 99,99,255,255,255,255\r"));
}

/* The modem cannot be queried before it is waiting for registration */
ZTEST(modem_cellular_signal, test_not_registered)
{
	int16_t value;

	modem_restart(modem);
	advance_to(modem, BOOT_AWAIT_APN);
	zassert_equal(-ENODATA, cellular_get_signal(modem, CELLULAR_SIGNAL_RSSI, &value));
	expect_no_line(K_MSEC(100));
}

static void signal_after(void *fixture)
{
	/* A failed test can leave the query blocked on a modem that never answers */
	if (signal_ret == -EINPROGRESS) {
		k_thread_abort(&signal_thread);
		signal_ret = 0;
	}
	modem_test_after(fixture);
}

ZTEST_SUITE(modem_cellular_signal, NULL, modem_test_setup, modem_test_before, signal_after, NULL);
