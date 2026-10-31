/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>

#include <string.h>

struct test_modem {
	const struct device *dev;
	/* Baudrate the modem powers up at */
	uint32_t baudrate;
};

#define TEST_MODEM(node_id)                                                                        \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.baudrate = DT_PROP(DT_BUS(node_id), current_speed),                               \
	},

static const struct test_modem modems[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_modem_cellular_test, TEST_MODEM)};

static K_THREAD_STACK_DEFINE(suspend_stack, 1024);
static struct k_thread suspend_thread;
static int suspend_ret;

bool wait_for_state(const struct device *dev, enum test_backend_state state,
		    k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (!sys_timepoint_expired(end)) {
		if (test_backend_state(dev) == state) {
			return true;
		}
		k_msleep(1);
	}

	return test_backend_state(dev) == state;
}

void expect_state(const struct device *dev, enum test_backend_state state)
{
	zassert_true(wait_for_state(dev, state, K_SECONDS(2)),
		     "%s expected state %d, stopped in state %d", dev->name, state,
		     test_backend_state(dev));
}

void expect_gpio(const struct gpio_dt_spec *spec, int value)
{
	k_timepoint_t end = sys_timepoint_calc(K_SECONDS(2));

	while (gpio_emul_output_get_dt(spec) != value) {
		zassert_false(sys_timepoint_expired(end), "GPIO %u never went to %d", spec->pin,
			      value);
		k_msleep(1);
	}
}

void expect_line(enum test_backend_pipe_id pipe, const char *expected)
{
	struct test_backend_line line;

	zassert_true(test_backend_wait_for_line(&line, K_SECONDS(2)), "timed out waiting for '%s'",
		     expected);
	zassert_equal(pipe, line.pipe, "line '%s' was sent on pipe %d, expected %d", line.data,
		      line.pipe, pipe);
	zassert_equal(0, strcmp(expected, line.data), "expected '%s', got '%s'", expected,
		      line.data);
}

void expect_no_line(k_timeout_t timeout)
{
	struct test_backend_line line;

	zassert_false(test_backend_wait_for_line(&line, timeout), "unexpected '%s'", line.data);
}

static void expect_pipe_open(enum test_backend_pipe_id pipe)
{
	k_timepoint_t end = sys_timepoint_calc(K_SECONDS(2));

	while (!test_backend_pipe_is_open(pipe)) {
		zassert_false(sys_timepoint_expired(end), "pipe %d never opened", pipe);
		k_msleep(1);
	}
}

void expect_read(enum test_backend_pipe_id pipe)
{
	zassert_true(test_backend_wait_for_read(pipe, K_SECONDS(2)),
		     "response on pipe %d was not read", pipe);
}

void ok(enum test_backend_pipe_id pipe)
{
	test_backend_respond(pipe, "OK\r");
}

void respond_error(enum test_backend_pipe_id pipe)
{
	test_backend_respond(pipe, "ERROR\r");
}

void registered(enum test_backend_pipe_id pipe)
{
	test_backend_respond(pipe, "+CEREG: 1,1\r");
}

uint32_t modem_power_up_baudrate(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(modems); i++) {
		if (modems[i].dev == dev) {
			return modems[i].baudrate;
		}
	}

	zassert_unreachable("%s is not a test modem", dev->name);
	return 0;
}

uint32_t uart_baudrate(const struct device *dev)
{
	struct uart_config cfg;

	zassert_ok(uart_config_get(test_backend_uart(dev), &cfg));
	return cfg.baudrate;
}

void modem_reset_state(const struct device *dev)
{
	const struct device *uart = test_backend_uart(dev);
	struct uart_config cfg;

	modem_cellular_ztest_halt(dev);

	/* A previous test may have left the UART at the new baudrate */
	zassert_ok(uart_config_get(uart, &cfg));
	cfg.baudrate = modem_power_up_baudrate(dev);
	zassert_ok(uart_configure(uart, &cfg));
	test_backend_reset(dev);
}

void modem_restart(const struct device *dev)
{
	modem_reset_state(dev);
	zassert_ok(modem_cellular_init(dev), "failed to reinitialize %s", dev->name);
}

/* Answer the init command, then each modem info command the init script follows it with */
static void answer_init(const struct device *dev, enum boot_step target)
{
	static const struct {
		const char *cmd;
		const char *response;
	} modem_info[] = {
		{MODEM_CELLULAR_TEST_IMEI_SN_CMD, TEST_IMEI_SN "\r"},
		{MODEM_CELLULAR_TEST_IMEI_CMD, "+CGSN: \"" TEST_IMEI "\"\r"},
		{MODEM_CELLULAR_TEST_SERIAL_NUMBER_CMD, TEST_SERIAL_NUMBER "\r"},
		{MODEM_CELLULAR_TEST_MODEL_ID_CMD, TEST_MODEL_ID "\r"},
		{MODEM_CELLULAR_TEST_MANUFACTURER_CMD, TEST_MANUFACTURER "\r"},
		{MODEM_CELLULAR_TEST_FW_VERSION_CMD, TEST_FW_VERSION "\r"},
		{MODEM_CELLULAR_TEST_IMSI_CMD, TEST_IMSI "\r"},
		{MODEM_CELLULAR_TEST_ICCID_CMD, "+ICCID: " TEST_ICCID "\r"},
	};

	ok(TEST_BACKEND_PIPE_UART);
	if (!test_backend_has_modem_info_script(dev)) {
		zassert_not_equal(BOOT_MODEM_INFO, target, "%s has no modem info script",
				  dev->name);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(modem_info); i++) {
		expect_line(TEST_BACKEND_PIPE_UART, modem_info[i].cmd);
		if ((i == 0) && (target == BOOT_MODEM_INFO)) {
			return;
		}
		test_backend_respond(TEST_BACKEND_PIPE_UART, modem_info[i].response);
		ok(TEST_BACKEND_PIPE_UART);
	}
}

void advance_to(const struct device *dev, enum boot_step target)
{
	const bool has_network_script = test_backend_has_network_script(dev);

#define STOP_AT(step)                                                                              \
	if (target == BOOT_##step) {                                                               \
		return;                                                                            \
	}

	switch (target) {
	case BOOT_RESET_PULSE:
		expect_gpio(test_backend_reset_gpio(dev), 1);
		return;
	case BOOT_RESET_RELEASED:
		expect_gpio(test_backend_reset_gpio(dev), 1);
		expect_gpio(test_backend_reset_gpio(dev), 0);
		return;
	case BOOT_POWER_KEY:
		expect_gpio(test_backend_power_gpio(dev), 1);
		return;
	case BOOT_UART_OPENED:
		expect_state(dev, TEST_BACKEND_STATE_AT);
		return;
	default:
		break;
	}

	if (test_backend_has_set_baudrate_script(dev)) {
		expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
		STOP_AT(SET_BAUDRATE);
		ok(TEST_BACKEND_PIPE_UART);
	}

	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
	STOP_AT(INIT);
	if (target == BOOT_RECOVERY) {
		respond_error(TEST_BACKEND_PIPE_UART);
		expect_read(TEST_BACKEND_PIPE_UART);
		return;
	}
	if (target == BOOT_CMUX_PENDING) {
		test_backend_cmux_connect_hold(true);
		answer_init(dev, target);
		zassert_true(test_backend_wait_for_cmux_connect(K_SECONDS(2)),
			     "CMUX connect never started");
		return;
	}
	answer_init(dev, target);
	STOP_AT(MODEM_INFO);

	if (test_backend_has_dlci_setup_script(dev)) {
		expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_DLCI_SETUP_CMD);
		STOP_AT(DLCI1_SETUP);
		ok(TEST_BACKEND_PIPE_DLCI1);

		expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DLCI_SETUP_CMD);
		STOP_AT(DLCI2_SETUP);
		ok(TEST_BACKEND_PIPE_DLCI2);

		expect_read(TEST_BACKEND_PIPE_DLCI2);
	} else {
		expect_pipe_open(TEST_BACKEND_PIPE_DLCI2);
	}
	STOP_AT(AWAIT_APN);
	zassert_ok(cellular_set_apn(dev, TEST_APN));

	expect_line(TEST_BACKEND_PIPE_DLCI1, TEST_APN_CMD);
	STOP_AT(APN);
	ok(TEST_BACKEND_PIPE_DLCI1);

	/* With a network script registration is awaited before dialling, otherwise after */
	if (has_network_script) {
		expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_NETWORK_CMD);
		STOP_AT(NETWORK);
		ok(TEST_BACKEND_PIPE_DLCI1);

		expect_read(TEST_BACKEND_PIPE_DLCI1);
		STOP_AT(AWAIT_REGISTRATION);
		registered(TEST_BACKEND_PIPE_DLCI1);
	}

	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
	STOP_AT(DIAL);
	ok(TEST_BACKEND_PIPE_DLCI2);

	if (!has_network_script) {
		expect_state(dev, TEST_BACKEND_STATE_DIALED);
		STOP_AT(AWAIT_REGISTRATION);
		registered(TEST_BACKEND_PIPE_DLCI1);
	}

	expect_state(dev, TEST_BACKEND_STATE_LINK_UP);
	STOP_AT(LINK_UP);
	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, "+CEREG: 1,0\r");

	expect_state(dev, TEST_BACKEND_STATE_DIALED);
	STOP_AT(DEREGISTERED);

	zassert_unreachable("%s cannot reach step %d", __func__, target);
#undef STOP_AT
}

static void suspend_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	suspend_ret = modem_cellular_pm_action(arg1, PM_DEVICE_ACTION_SUSPEND);
}

static void flush_marker_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

void flush_driver_events(void)
{
	static struct k_work flush_marker;
	struct k_work_sync sync;

	/* The driver handles its events on the system workqueue, ahead of the marker */
	k_work_init(&flush_marker, flush_marker_handler);
	zassert_equal(1, k_work_submit(&flush_marker));
	zassert_true(k_work_flush(&flush_marker, &sync), "driver events were not handled");
}

void start_suspend(const struct device *dev)
{
	suspend_ret = -EINPROGRESS;
	k_thread_create(&suspend_thread, suspend_stack, K_THREAD_STACK_SIZEOF(suspend_stack),
			suspend_entry, (void *)dev, NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY - 1, 0,
			K_NO_WAIT);

	/* Outranks this cooperative thread, so runs until it blocks with the request queued */
	k_yield();
	flush_driver_events();
}

void finish_suspend(const struct device *dev)
{
	zassert_ok(k_thread_join(&suspend_thread, K_SECONDS(2)), "suspend did not complete");
	zassert_ok(suspend_ret, "suspend failed");
	zassert_equal(TEST_BACKEND_STATE_OFF, test_backend_state(dev),
		      "suspend stopped in state %d", test_backend_state(dev));
}

void suspend_and_shutdown(const struct device *dev)
{
	start_suspend(dev);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	ok(TEST_BACKEND_PIPE_UART);
	finish_suspend(dev);
}

static void halt_all_modems(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(modems); i++) {
		modem_cellular_ztest_halt(modems[i].dev);
	}
}

void *modem_test_setup(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(modems); i++) {
		zassert_true(device_is_ready(modems[i].dev), "%s is not ready",
			     modems[i].dev->name);
	}
	halt_all_modems();

	return NULL;
}

void modem_test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* All instances share the line queue, so stop any left running */
	halt_all_modems();
}

void modem_test_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* A failed test can leave the suspend thread blocked on a modem that never shuts down */
	if (suspend_ret == -EINPROGRESS) {
		k_thread_abort(&suspend_thread);
		suspend_ret = 0;
	}
	halt_all_modems();
}
