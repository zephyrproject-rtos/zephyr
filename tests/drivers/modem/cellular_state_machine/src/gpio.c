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

#define AUTOSTART_STARTUP_TIME_MS DT_PROP(DT_NODELABEL(modem_autostart), startup_time_ms)

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));
static const struct device *const modem_hold = DEVICE_DT_GET(DT_NODELABEL(modem_hold));
static const struct device *const modem_resume = DEVICE_DT_GET(DT_NODELABEL(modem_resume));
static const struct device *const modem_recovery = DEVICE_DT_GET(DT_NODELABEL(modem_recovery));
static const struct device *const modem_autostart =
	DEVICE_DT_GET(DT_NODELABEL(modem_autostart));
static const struct device *const modem_status = DEVICE_DT_GET(DT_NODELABEL(modem_status));
static const struct device *const modem_no_shutdown =
	DEVICE_DT_GET(DT_NODELABEL(modem_no_shutdown));

static const struct gpio_dt_spec status_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(modem_status), mdm_status_gpios);

enum gpio_role {
	GPIO_ROLE_POWER,
	GPIO_ROLE_RESET,
};

/* A level change on a monitored pin, and the state of the link when it happened */
struct gpio_edge {
	enum gpio_role role;
	int value;
	enum test_backend_state state;
};

K_MSGQ_DEFINE(gpio_edges, sizeof(struct gpio_edge), 16, 4);

static struct {
	const struct device *dev;
	struct gpio_callback cb;
	int last[2];
	bool overflow;
} monitor;

static const char *const gpio_role_names[] = {
	[GPIO_ROLE_POWER] = "power",
	[GPIO_ROLE_RESET] = "reset",
};

static void gpio_monitor_record(enum gpio_role role, const struct gpio_dt_spec *spec)
{
	struct gpio_edge edge = {
		.role = role,
		.value = gpio_emul_output_get_dt(spec),
		.state = test_backend_state(monitor.dev),
	};

	if (edge.value == monitor.last[role]) {
		return;
	}
	monitor.last[role] = edge.value;
	/* Runs on the driver's thread, so leave asserting to the test thread */
	if (k_msgq_put(&gpio_edges, &edge, K_NO_WAIT) != 0) {
		monitor.overflow = true;
	}
}

/* The emulated GPIO controller fires callbacks synchronously on output changes, so every
 * level the driver drives is seen, however short the pulse.
 */
static void gpio_monitor_handler(const struct device *port, struct gpio_callback *cb,
				 gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	gpio_monitor_record(GPIO_ROLE_POWER, test_backend_power_gpio(monitor.dev));
	gpio_monitor_record(GPIO_ROLE_RESET, test_backend_reset_gpio(monitor.dev));
}

static void gpio_monitor_start(const struct device *dev)
{
	const struct gpio_dt_spec *power_gpio = test_backend_power_gpio(dev);
	const struct gpio_dt_spec *reset_gpio = test_backend_reset_gpio(dev);

	/* Start every test from released lines so the first recorded edge is the driver's */
	zassert_ok(gpio_pin_configure_dt(power_gpio, GPIO_OUTPUT_INACTIVE));
	zassert_ok(gpio_pin_configure_dt(reset_gpio, GPIO_OUTPUT_INACTIVE));

	monitor.dev = dev;
	monitor.last[GPIO_ROLE_POWER] = 0;
	monitor.last[GPIO_ROLE_RESET] = 0;
	monitor.overflow = false;
	k_msgq_purge(&gpio_edges);

	gpio_init_callback(&monitor.cb, gpio_monitor_handler,
			   BIT(power_gpio->pin) | BIT(reset_gpio->pin));
	zassert_ok(gpio_add_callback_dt(reset_gpio, &monitor.cb));
}

static void gpio_monitor_stop(void)
{
	if (monitor.dev == NULL) {
		return;
	}

	(void)gpio_remove_callback_dt(test_backend_reset_gpio(monitor.dev), &monitor.cb);
	monitor.dev = NULL;
}

/* Wait for the next edge, which must be @p role going to @p value in @p state */
static void expect_edge(enum gpio_role role, int value, enum test_backend_state state)
{
	struct gpio_edge edge;

	zassert_false(monitor.overflow, "GPIO edge queue overflowed");
	zassert_ok(k_msgq_get(&gpio_edges, &edge, K_SECONDS(1)), "timed out waiting for %s=%d",
		   gpio_role_names[role], value);
	zassert_equal(role, edge.role, "%s went to %d, expected %s=%d", gpio_role_names[edge.role],
		      edge.value, gpio_role_names[role], value);
	zassert_equal(value, edge.value, "%s went to %d, expected %d", gpio_role_names[role],
		      edge.value, value);
	zassert_equal(state, edge.state, "%s went to %d in state %d, expected state %d",
		      gpio_role_names[role], value, edge.state, state);
}

/* Expect @p role to be asserted and released again, both in @p state */
static void expect_pulse(enum gpio_role role, enum test_backend_state state)
{
	expect_edge(role, 1, state);
	expect_edge(role, 0, state);
}

/* Assert that neither pin changed since the last expected edge */
static void expect_no_edge(void)
{
	struct gpio_edge edge;

	zassert_false(monitor.overflow, "GPIO edge queue overflowed");
	if (k_msgq_get(&gpio_edges, &edge, K_NO_WAIT) == 0) {
		zassert_unreachable("unexpected %s=%d in state %d", gpio_role_names[edge.role],
				    edge.value, edge.state);
	}
}

static int64_t boot_modem(const struct device *dev)
{
	int64_t start;

	modem_reset_state(dev);
	gpio_monitor_start(dev);

	start = k_uptime_get();
	zassert_ok(modem_cellular_init(dev), "failed to reinitialize %s", dev->name);
	return start;
}

ZTEST(modem_cellular_gpio, test_hold_on_suspend)
{
	boot_modem(modem_hold);

	/* Held through init, released before the power key and the UART */
	expect_edge(GPIO_ROLE_RESET, 1, TEST_BACKEND_STATE_OFF);
	expect_edge(GPIO_ROLE_RESET, 0, TEST_BACKEND_STATE_OFF);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	expect_no_edge();

	/* Reasserted on suspend */
	zassert_ok(modem_cellular_pm_action(modem_hold, PM_DEVICE_ACTION_SUSPEND),
		   "suspend failed");
	expect_edge(GPIO_ROLE_RESET, 1, TEST_BACKEND_STATE_AT);
	expect_no_edge();

	/* Released again on resume, before the power key and the UART */
	zassert_ok(modem_cellular_pm_action(modem_hold, PM_DEVICE_ACTION_RESUME), "resume failed");
	expect_edge(GPIO_ROLE_RESET, 0, TEST_BACKEND_STATE_OFF);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_toggle_on_resume)
{
	boot_modem(modem_resume);

	expect_pulse(GPIO_ROLE_RESET, TEST_BACKEND_STATE_OFF);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	expect_no_edge();

	/* Untouched on suspend */
	zassert_ok(modem_cellular_pm_action(modem_resume, PM_DEVICE_ACTION_SUSPEND),
		   "suspend failed");
	expect_no_edge();

	/* Pulsed again on resume, before the power key and the UART */
	zassert_ok(modem_cellular_pm_action(modem_resume, PM_DEVICE_ACTION_RESUME),
		   "resume failed");
	expect_pulse(GPIO_ROLE_RESET, TEST_BACKEND_STATE_OFF);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_toggle_on_recovery)
{
	boot_modem(modem_recovery);

	/* Untouched on boot */
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	expect_no_edge();
	ok(TEST_BACKEND_PIPE_UART);

	/* Fail the init script to push the state machine into recovery */
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
	respond_error(TEST_BACKEND_PIPE_UART);

	expect_pulse(GPIO_ROLE_RESET, TEST_BACKEND_STATE_AT);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_AT);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_autostarts_ready_message)
{
	struct test_backend_line line;
	int64_t start = boot_modem(modem_autostart);

	/* No AT commands until the modem reports it is ready */
	zassert_false(test_backend_wait_for_line(&line, K_MSEC(100)), "'%s' sent before ready",
		      line.data);
	zassert_equal(TEST_BACKEND_STATE_AT, test_backend_state(modem_autostart),
		      "boot stopped in state %d", test_backend_state(modem_autostart));
	test_backend_respond(TEST_BACKEND_PIPE_UART, "Ready\r");

	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	zassert_true(k_uptime_delta(&start) < AUTOSTART_STARTUP_TIME_MS,
		     "ready message did not cut the startup wait short");
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_autostarts_ready_timeout)
{
	int64_t start = boot_modem(modem_autostart);
	int64_t elapsed;

	/* Without a ready message the full startup time elapses before AT commands */
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	elapsed = k_uptime_delta(&start);
	zassert_true(elapsed >= AUTOSTART_STARTUP_TIME_MS,
		     "AT commands sent %lld ms after boot, before startup time elapsed",
		     (long long)elapsed);

	/* Autostarting modems go straight to waiting for the modem to come up */
	expect_no_edge();
}

static void set_modem_status(int value)
{
	zassert_ok(gpio_pin_configure_dt(&status_gpio, GPIO_INPUT));
	zassert_ok(gpio_emul_input_set_dt(&status_gpio, value));
}

ZTEST(modem_cellular_gpio, test_status_unpowered)
{
	set_modem_status(0);
	boot_modem(modem_status);

	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_status_powered)
{
	set_modem_status(1);
	boot_modem(modem_status);

	/* Pulsing the power key would turn the running modem off */
	advance_to(modem_status, BOOT_DLCI1_SETUP);
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_status_powered_unresponsive)
{
	int64_t start;

	set_modem_status(1);
	boot_modem(modem_status);

	advance_to(modem_status, BOOT_INIT);
	expect_no_edge();

	/* The status line was wrong, so power the modem on without the recovery backoff */
	start = k_uptime_get();
	respond_error(TEST_BACKEND_PIPE_UART);
	expect_edge(GPIO_ROLE_POWER, 1, TEST_BACKEND_STATE_AT);
	zassert_true(k_uptime_delta(&start) < CONFIG_MODEM_CELLULAR_RECOVERY_BACKOFF_MS,
		     "recovery was entered");
	expect_edge(GPIO_ROLE_POWER, 0, TEST_BACKEND_STATE_AT);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	zassert_equal(modem_power_up_baudrate(modem_status), uart_baudrate(modem_status),
		      "UART baudrate was not restored on power up");
	expect_no_edge();
}

/* A restart that skips the power-on pulse still configures the baudrate from the original */
ZTEST(modem_cellular_gpio, test_status_powered_restart)
{
	set_modem_status(1);
	boot_modem(modem_status);
	advance_to(modem_status, BOOT_LINK_UP);

	/* Spend the periodic script budget to restart the still powered modem */
	for (int i = 0; i < CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES; i++) {
		expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
		respond_error(TEST_BACKEND_PIPE_DLCI1);
	}
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	zassert_equal(modem_power_up_baudrate(modem_status), uart_baudrate(modem_status),
		      "baudrate script sent at the new baudrate");
	expect_no_edge();
	ok(TEST_BACKEND_PIPE_UART);

	/* The modem does not respond, so it is powered on at its power up baudrate */
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_INIT_CMD);
	respond_error(TEST_BACKEND_PIPE_UART);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_AT);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
	zassert_equal(modem_power_up_baudrate(modem_status), uart_baudrate(modem_status),
		      "UART baudrate was not restored on power up");
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_shutdown_script_failure)
{
	boot_modem(modem);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	advance_to(modem, BOOT_DLCI2_SETUP);

	start_suspend(modem);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
	expect_no_edge();

	/* Software shutdown failed, fall back to the power key */
	respond_error(TEST_BACKEND_PIPE_UART);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	finish_suspend(modem);
	expect_no_edge();
}

ZTEST(modem_cellular_gpio, test_no_shutdown_script)
{
	boot_modem(modem_no_shutdown);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	advance_to(modem_no_shutdown, BOOT_DLCI2_SETUP);

	/* Powered down with the power key alone */
	start_suspend(modem_no_shutdown);
	expect_pulse(GPIO_ROLE_POWER, TEST_BACKEND_STATE_OFF);
	finish_suspend(modem_no_shutdown);
	expect_no_edge();
	expect_no_line(K_MSEC(100));
}

static void gpio_after(void *fixture)
{
	modem_test_after(fixture);
	gpio_monitor_stop();
}

ZTEST_SUITE(modem_cellular_gpio, NULL, modem_test_setup, modem_test_before, gpio_after, NULL);
