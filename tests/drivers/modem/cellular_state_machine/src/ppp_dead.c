/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/modem/ppp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ppp.h>
#include <zephyr/ztest.h>

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));
static const struct device *const modem_no_network = DEVICE_DT_GET(DT_NODELABEL(modem_no_network));

static struct net_if *ppp_iface(const struct device *dev)
{
	const struct modem_cellular_config *config = dev->config;

	return modem_ppp_get_iface(config->ppp);
}

/* Report the PPP link dead, as the PPP L2 does when its phase drops to dead */
static void ppp_dead(const struct device *dev)
{
	net_mgmt_event_notify(NET_EVENT_PPP_PHASE_DEAD, ppp_iface(dev));
}

/*
 * The driver carries on as before: its next action is the periodic script, where dropping the
 * link would release PPP and dial again once the wait for PPP times out.
 */
static void expect_ignored(const struct device *dev, enum test_backend_state state)
{
	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	ok(TEST_BACKEND_PIPE_DLCI1);
	expect_read(TEST_BACKEND_PIPE_DLCI1);
	zassert_equal(state, test_backend_state(dev), "link changed to state %d",
		      test_backend_state(dev));
}

/* With the interface admitted, losing PPP while registered drops the link and dials again */
ZTEST(modem_cellular_ppp_dead, test_registered)
{
	zassert_ok(net_if_up(ppp_iface(modem)));
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	ppp_dead(modem);
	expect_state(modem, TEST_BACKEND_STATE_DIALED);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
	ok(TEST_BACKEND_PIPE_DLCI2);
	expect_state(modem, TEST_BACKEND_STATE_LINK_UP);
}

/* Without the interface admitted, PPP stopping is expected and the link is kept */
ZTEST(modem_cellular_ppp_dead, test_registered_admin_down)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	ppp_dead(modem);
	expect_ignored(modem, TEST_BACKEND_STATE_LINK_UP);
}

/* Losing PPP while dialled and waiting for registration dials again */
ZTEST(modem_cellular_ppp_dead, test_await_registered)
{
	zassert_ok(net_if_up(ppp_iface(modem_no_network)));
	modem_restart(modem_no_network);
	advance_to(modem_no_network, BOOT_AWAIT_REGISTRATION);

	ppp_dead(modem_no_network);
	expect_state(modem_no_network, TEST_BACKEND_STATE_CMUX);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
	ok(TEST_BACKEND_PIPE_DLCI2);
	expect_state(modem_no_network, TEST_BACKEND_STATE_DIALED);
	registered(TEST_BACKEND_PIPE_DLCI1);
	expect_state(modem_no_network, TEST_BACKEND_STATE_LINK_UP);
}

ZTEST(modem_cellular_ppp_dead, test_await_registered_admin_down)
{
	modem_restart(modem_no_network);
	advance_to(modem_no_network, BOOT_AWAIT_REGISTRATION);

	ppp_dead(modem_no_network);
	expect_ignored(modem_no_network, TEST_BACKEND_STATE_DIALED);
}

/*
 * Once the link has dropped, PPP reporting itself dead cuts the wait for it short, leaving
 * only the time for the channel to return to AT mode.
 */
ZTEST(modem_cellular_ppp_dead, test_ends_wait)
{
	const uint32_t at_mode_ms = DT_PROP(DT_ALIAS(modem), reset_pulse_duration_ms);
	int64_t start;
	int64_t elapsed;

	BUILD_ASSERT(DT_PROP(DT_ALIAS(modem), reset_pulse_duration_ms) * 2 <
		     CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS);

	modem_restart(modem);
	advance_to(modem, BOOT_DEREGISTERED);

	start = k_uptime_get();
	ppp_dead(modem);

	/* PPP is released on leaving the wait, then registration is awaited again */
	expect_state(modem, TEST_BACKEND_STATE_CMUX);
	elapsed = k_uptime_delta(&start);
	zassert_true(elapsed >= at_mode_ms, "left after %lld ms, before AT mode", elapsed);
	zassert_true(elapsed < CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS,
		     "left after %lld ms, as if PPP was never reported dead", elapsed);
	expect_no_line(K_MSEC(100));
}

static void ppp_dead_after(void *fixture)
{
	const struct device *const devs[] = {modem, modem_no_network};

	modem_test_after(fixture);

	/* The drivers are halted, so this no longer reaches them */
	for (size_t i = 0; i < ARRAY_SIZE(devs); i++) {
		if (net_if_is_admin_up(ppp_iface(devs[i]))) {
			zassert_ok(net_if_down(ppp_iface(devs[i])));
		}
	}
}

ZTEST_SUITE(modem_cellular_ppp_dead, NULL, modem_test_setup, modem_test_before, ppp_dead_after,
	    NULL);
