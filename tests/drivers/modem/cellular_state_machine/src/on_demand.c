/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ppp.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT));

static const struct device *const modem = DEVICE_DT_GET(DT_NODELABEL(modem_on_demand));

static struct net_if *ppp_iface(void)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));

	zassert_not_null(iface, "no PPP interface");
	return iface;
}

/* Boot until the driver parks waiting for the PPP interface to be admitted */
static void park(void)
{
	modem_restart(modem);
	advance_to(modem, BOOT_APN);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_read(TEST_BACKEND_PIPE_DLCI1);
}

/* Parked, the periodic script keeps running while nothing is dialled */
static void expect_parked(void)
{
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_read(TEST_BACKEND_PIPE_DLCI1);
	expect_state(modem, TEST_BACKEND_STATE_CMUX);
}

/* Admit the interface and bring the link up. Registration outlives a hang-up, so a
 * re-dial goes straight to link up.
 */
static void dial(void)
{
	zassert_ok(net_if_up(ppp_iface()));
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
	ok(TEST_BACKEND_PIPE_DLCI2);
	registered(TEST_BACKEND_PIPE_DLCI1);
	expect_state(modem, TEST_BACKEND_STATE_LINK_UP);
}

ZTEST(modem_cellular_on_demand, test_dial_on_admin_up)
{
	park();
	expect_parked();
	expect_parked();
	dial();
}

/* An interface admitted before the modem is up is dialled as soon as it is */
ZTEST(modem_cellular_on_demand, test_admin_up_before_parking)
{
	zassert_ok(net_if_up(ppp_iface()));
	park();
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
}

/* Taking the interface down hangs up, returning to wait for the next admit */
ZTEST(modem_cellular_on_demand, test_hangup)
{
	park();
	dial();

	zassert_ok(net_if_down(ppp_iface()));
	expect_state(modem, TEST_BACKEND_STATE_CMUX);
	expect_parked();
	dial();
}

/* A hang-up while the dial script runs abandons it */
ZTEST(modem_cellular_on_demand, test_hangup_while_dialing)
{
	park();
	zassert_ok(net_if_up(ppp_iface()));
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);

	zassert_ok(net_if_down(ppp_iface()));
	expect_parked();
	dial();
}

/* While parked, a ring reopens the UART the CMUX closed to save power */
ZTEST(modem_cellular_on_demand, test_ring)
{
	const struct gpio_dt_spec ring_gpio =
		GPIO_DT_SPEC_GET(DT_NODELABEL(modem_on_demand), mdm_ring_gpios);
	k_timepoint_t end = sys_timepoint_calc(K_SECONDS(2));

	park();
	test_backend_cmux_power_save();
	zassert_false(test_backend_pipe_is_open(TEST_BACKEND_PIPE_UART), "UART still open");

	zassert_ok(gpio_emul_input_set_dt(&ring_gpio, 1));
	zassert_ok(gpio_emul_input_set_dt(&ring_gpio, 0));
	while (!test_backend_pipe_is_open(TEST_BACKEND_PIPE_UART)) {
		zassert_false(sys_timepoint_expired(end), "ring did not reopen the UART");
		k_msleep(1);
	}
}

ZTEST(modem_cellular_on_demand, test_suspend)
{
	park();
	suspend_and_shutdown(modem);
}

/* A paused periodic script skips its runs, and runs at once when resumed */
ZTEST(modem_cellular_on_demand, test_periodic_pause)
{
	park();
	zassert_ok(cellular_modem_pause_periodic_script(modem));
	expect_no_line(K_MSEC(2 * CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS));

	zassert_ok(cellular_modem_resume_periodic_script(modem));
	expect_parked();
}

/* Without a skipped run, resuming leaves the periodic script on its interval */
ZTEST(modem_cellular_on_demand, test_periodic_resume_before_due)
{
	park();
	zassert_ok(cellular_modem_pause_periodic_script(modem));
	zassert_ok(cellular_modem_resume_periodic_script(modem));
	expect_no_line(K_MSEC(CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS / 2));
	expect_parked();
}

/* The periodic script exhausting its failure budget restarts the modem */
ZTEST(modem_cellular_on_demand, test_periodic_failures)
{
	park();
	for (int i = 0; i < CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES; i++) {
		expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
		respond_error(TEST_BACKEND_PIPE_DLCI1);
	}
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SET_BAUDRATE_CMD);
}

static void on_demand_after(void *fixture)
{
	modem_test_after(fixture);

	/* The driver is halted, so this no longer reaches it */
	if (net_if_is_admin_up(ppp_iface())) {
		zassert_ok(net_if_down(ppp_iface()));
	}
}

ZTEST_SUITE(modem_cellular_on_demand, NULL, modem_test_setup, modem_test_before,
	    on_demand_after, NULL);
