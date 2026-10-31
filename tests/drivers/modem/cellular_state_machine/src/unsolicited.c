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
static const struct device *const modem_slow = DEVICE_DT_GET(DT_NODELABEL(modem_slow));

struct test_event {
	enum cellular_event event;
	struct cellular_evt_registration_status reg;
};

K_MSGQ_DEFINE(unsol_events, sizeof(struct test_event), 16, 4);

static void event_handler(const struct device *dev, enum cellular_event event,
			  const void *payload, void *user_data)
{
	struct test_event evt = {.event = event};

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (event == CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED) {
		evt.reg = *(const struct cellular_evt_registration_status *)payload;
	}
	(void)k_msgq_put(&unsol_events, &evt, K_NO_WAIT);
}

static void listen_for_events(const struct device *dev, cellular_event_mask_t mask)
{
	k_msgq_purge(&unsol_events);
	zassert_ok(cellular_set_callback(dev, mask, event_handler, NULL));
}

static struct test_event expect_event(enum cellular_event event)
{
	struct test_event evt;

	zassert_ok(k_msgq_get(&unsol_events, &evt, K_SECONDS(2)),
		   "timed out waiting for event 0x%x", event);
	zassert_equal(event, evt.event, "got event 0x%x, expected 0x%x", evt.event, event);
	return evt;
}

static void expect_reg_status(enum cellular_registration_status status)
{
	struct test_event evt = expect_event(CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED);

	zassert_equal(status, evt.reg.status, "registration status %d, expected %d",
		      evt.reg.status, status);
}

/* Send @p notification on DLCI1 and wait for the driver to act on it */
static void notify(const char *notification)
{
	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, notification);
	expect_read(TEST_BACKEND_PIPE_DLCI1);
	flush_driver_events();
}

/* The registration of any radio access technology starts the dial */
ZTEST(modem_cellular_unsolicited, test_registration_per_rat)
{
	const struct {
		const char *notification;
		enum cellular_access_technology tech;
		enum cellular_registration_status status;
	} rats[] = {
		{"+CREG: 1\r", CELLULAR_ACCESS_TECHNOLOGY_GSM,
		 CELLULAR_REGISTRATION_REGISTERED_HOME},
		{"+CGREG: 5\r", CELLULAR_ACCESS_TECHNOLOGY_UTRAN,
		 CELLULAR_REGISTRATION_REGISTERED_ROAMING},
		{"+CEREG: 5\r", CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN,
		 CELLULAR_REGISTRATION_REGISTERED_ROAMING},
	};
	enum cellular_registration_status status;

	for (size_t i = 0; i < ARRAY_SIZE(rats); i++) {
		modem_restart(modem);
		advance_to(modem, BOOT_AWAIT_REGISTRATION);

		test_backend_respond(TEST_BACKEND_PIPE_DLCI1, rats[i].notification);
		expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
		zassert_ok(cellular_get_registration_status(modem, rats[i].tech, &status));
		zassert_equal(rats[i].status, status, "%s: status %d", rats[i].notification,
			      status);
	}
}

/* Searching and denied are reported but do not start the dial */
ZTEST(modem_cellular_unsolicited, test_registration_status_events)
{
	modem_restart(modem);
	advance_to(modem, BOOT_AWAIT_REGISTRATION);
	listen_for_events(modem, CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED);

	notify("+CEREG: 2\r");
	expect_reg_status(CELLULAR_REGISTRATION_SEARCHING);
	notify("+CEREG: 3\r");
	expect_reg_status(CELLULAR_REGISTRATION_DENIED);
	expect_no_line(K_MSEC(100));

	notify("+CEREG: 1\r");
	expect_reg_status(CELLULAR_REGISTRATION_REGISTERED_HOME);
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);

	zassert_ok(cellular_set_callback(modem, 0, NULL, NULL));
}

/* A read command answer carries <n> ahead of <stat>, a notification does not */
ZTEST(modem_cellular_unsolicited, test_read_answer_form)
{
	enum cellular_registration_status status;

	modem_restart(modem);
	advance_to(modem, BOOT_AWAIT_REGISTRATION);

	/* <n> of 2 would read as searching if taken for <stat> */
	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, "+CEREG: 2,5,\"ABCD\",\"0123ABCD\",7\r");
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
	zassert_ok(cellular_get_registration_status(modem, CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN,
						    &status));
	zassert_equal(CELLULAR_REGISTRATION_REGISTERED_ROAMING, status, "status %d", status);
}

/* The link stays up while any radio access technology is still registered */
ZTEST(modem_cellular_unsolicited, test_other_rat_keeps_link)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	notify("+CREG: 1\r");
	notify("+CEREG: 0\r");
	zassert_equal(TEST_BACKEND_STATE_LINK_UP, test_backend_state(modem),
		      "link dropped with GSM registered");

	notify("+CREG: 0\r");
	expect_state(modem, TEST_BACKEND_STATE_DIALED);
}

/* Losing the PDN connection while registered drops the link */
ZTEST(modem_cellular_unsolicited, test_cgev_detach)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	notify("+CGEV: NW DETACH\r");
	expect_state(modem, TEST_BACKEND_STATE_DIALED);
}

/* A PDN connection while not registered counts as registration */
ZTEST(modem_cellular_unsolicited, test_cgev_pdn_act)
{
	modem_restart(modem);
	advance_to(modem, BOOT_AWAIT_REGISTRATION);

	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, "+CGEV: ME PDN ACT 1\r");
	expect_line(TEST_BACKEND_PIPE_DLCI2, MODEM_CELLULAR_TEST_DIAL_CMD);
}

/* Events that agree with the registration status, or carry no PDN state, change nothing */
ZTEST(modem_cellular_unsolicited, test_cgev_ignored)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	notify("+CGEV: NW PDN ACT 1\r");
	notify("+CGEV: ME CLASS B\r");
	zassert_equal(TEST_BACKEND_STATE_LINK_UP, test_backend_state(modem), "link dropped");
	expect_no_line(K_NO_WAIT);
}

/* A ready indication after the CMUX disconnects ends the wait for AT mode early */
ZTEST(modem_cellular_unsolicited, test_modem_ready_after_cmux_disconnect)
{
	static const char *const indications[] = {"APP RDY\r", "Ready\r"};
	const uint32_t timeout_ms = DT_PROP(DT_NODELABEL(modem_slow), cmux_disconnect_timeout_ms);
	int64_t start;

	for (size_t i = 0; i < ARRAY_SIZE(indications); i++) {
		modem_restart(modem_slow);
		advance_to(modem_slow, BOOT_LINK_UP);

		start_suspend(modem_slow);
		expect_state(modem_slow, TEST_BACKEND_STATE_AT);

		start = k_uptime_get();
		test_backend_respond(TEST_BACKEND_PIPE_UART, indications[i]);
		expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SHUTDOWN_CMD);
		zassert_true(k_uptime_delta(&start) < timeout_ms, "%s did not end the wait",
			     indications[i]);
		ok(TEST_BACKEND_PIPE_UART);
		finish_suspend(modem_slow);
	}
}

ZTEST_SUITE(modem_cellular_unsolicited, NULL, modem_test_setup, modem_test_before,
	    modem_test_after, NULL);
