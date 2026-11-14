/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/ztest.h>

static const struct device *const modem_ring = DEVICE_DT_GET(DT_NODELABEL(modem_ring));
static const struct gpio_dt_spec ring_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(modem_ring), mdm_ring_gpios);

/* Pulse the ring indicator, as the modem does on an incoming call or message */
static void ring(void)
{
	zassert_ok(gpio_emul_input_set_dt(&ring_gpio, 1));
	zassert_ok(gpio_emul_input_set_dt(&ring_gpio, 0));
}

static bool wait_for_uart_open(bool open, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (test_backend_pipe_is_open(TEST_BACKEND_PIPE_UART) != open) {
		if (sys_timepoint_expired(end)) {
			return false;
		}
		k_msleep(1);
	}

	return true;
}

/* Let the CMUX close the UART to save power, then wake it with the ring indicator */
static void expect_ring_wakes_uart(void)
{
	test_backend_cmux_power_save();
	zassert_false(test_backend_pipe_is_open(TEST_BACKEND_PIPE_UART), "UART still open");

	ring();
	zassert_true(wait_for_uart_open(true, K_SECONDS(2)), "ring did not reopen the UART");
}

/* Once the CMUX is up and configured, every state reopens the UART on a ring */
ZTEST(modem_cellular_ring, test_wakes_uart)
{
	static const enum boot_step steps[] = {
		BOOT_APN,  BOOT_NETWORK, BOOT_AWAIT_REGISTRATION,
		BOOT_DIAL, BOOT_LINK_UP, BOOT_DEREGISTERED,
	};

	for (size_t i = 0; i < ARRAY_SIZE(steps); i++) {
		modem_restart(modem_ring);
		advance_to(modem_ring, steps[i]);
		expect_ring_wakes_uart();
	}
}

/* A ring while the UART is already open changes nothing */
ZTEST(modem_cellular_ring, test_uart_open)
{
	modem_restart(modem_ring);
	advance_to(modem_ring, BOOT_LINK_UP);

	ring();
	flush_driver_events();
	zassert_true(test_backend_pipe_is_open(TEST_BACKEND_PIPE_UART), "UART closed");
	expect_state(modem_ring, TEST_BACKEND_STATE_LINK_UP);
	expect_no_line(K_MSEC(100));
}

/* A suspended modem is not woken by a ring */
ZTEST(modem_cellular_ring, test_suspended)
{
	modem_restart(modem_ring);
	advance_to(modem_ring, BOOT_LINK_UP);
	suspend_and_shutdown(modem_ring);

	ring();
	flush_driver_events();
	zassert_false(wait_for_uart_open(true, K_MSEC(100)), "ring reopened the UART");
	zassert_equal(TEST_BACKEND_STATE_OFF, test_backend_state(modem_ring), "modem woke up");
	expect_no_line(K_NO_WAIT);
}

ZTEST_SUITE(modem_cellular_ring, NULL, modem_test_setup, modem_test_before, modem_test_after,
	    NULL);
