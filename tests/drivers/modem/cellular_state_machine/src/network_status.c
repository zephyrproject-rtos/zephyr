/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/ztest.h>

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));

/* Serving cell report fields, see MODEM_CELLULAR_TEST_SERVING_CELL */
#define SERVING_CELL(gci, rsrp, rsrq)                                                              \
	MODEM_CELLULAR_TEST_SERVING_CELL "505,1," #gci ",12345,1300,3,42," #rsrp "," #rsrq "\r"

struct test_event {
	enum cellular_event event;
	struct cellular_evt_network_status network;
};

K_MSGQ_DEFINE(network_events, sizeof(struct test_event), 16, 4);

/* Status carried by the last network status event periodic() saw */
static struct cellular_evt_network_status last_event;

static void event_handler(const struct device *dev, enum cellular_event event,
			  const void *payload, void *user_data)
{
	struct test_event evt = {.event = event};

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (event == CELLULAR_EVENT_NETWORK_STATUS_CHANGED) {
		evt.network = *(const struct cellular_evt_network_status *)payload;
	}
	(void)k_msgq_put(&network_events, &evt, K_NO_WAIT);
}

static struct test_event expect_event(enum cellular_event event)
{
	struct test_event evt;

	zassert_ok(k_msgq_get(&network_events, &evt, K_SECONDS(2)),
		   "timed out waiting for event 0x%x", event);
	zassert_equal(event, evt.event, "got event 0x%x, expected 0x%x", evt.event, event);
	return evt;
}

/* Boot to link up, registered on E-UTRAN, listening for network status events */
static void link_up(void)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	/* Same registration status, now with the access technology */
	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, "+CEREG: 1,\"ABCD\",\"0123ABCD\",7\r");
	expect_read(TEST_BACKEND_PIPE_DLCI1);

	k_msgq_purge(&network_events);
	zassert_ok(cellular_set_callback(modem,
					 CELLULAR_EVENT_NETWORK_STATUS_CHANGED |
						 CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED |
						 CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT,
					 event_handler, NULL));
}

/*
 * Answer the next periodic script, optionally with @p report, and return whether that raised a
 * network status event. The comms check result follows any event the report raised.
 */
static bool periodic(const char *report)
{
	struct test_event evt;
	bool changed = false;

	expect_line(TEST_BACKEND_PIPE_DLCI1, MODEM_CELLULAR_TEST_PERIODIC_CMD);
	if (report != NULL) {
		test_backend_respond(TEST_BACKEND_PIPE_DLCI1, report);
	}
	ok(TEST_BACKEND_PIPE_DLCI1);

	zassert_ok(k_msgq_get(&network_events, &evt, K_SECONDS(2)), "no event");
	if (evt.event == CELLULAR_EVENT_NETWORK_STATUS_CHANGED) {
		changed = true;
		last_event = evt.network;
		evt = expect_event(CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT);
	}
	zassert_equal(CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT, evt.event, "got event 0x%x",
		      evt.event);
	return changed;
}

static void expect_status(int16_t rsrp, int8_t rsrq, uint32_t gci)
{
	struct cellular_evt_network_status status;

	zassert_ok(cellular_get_network_status(modem, &status));
	zassert_equal(CELLULAR_REGISTRATION_REGISTERED_HOME, status.status);
	zassert_equal(CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN, status.access_tech);
	zassert_equal(505, status.cell.lte.mcc);
	zassert_equal(1, status.cell.lte.mnc);
	zassert_equal(gci, status.cell.lte.gci);
	zassert_equal(12345, status.cell.lte.tac);
	zassert_equal(1300, status.cell.lte.earfcn);
	zassert_equal(3, status.cell.lte.band);
	zassert_equal(42, status.cell.lte.phys_cell_id);
	zassert_equal(rsrp, status.cell.lte.rsrp);
	zassert_equal(rsrq, status.cell.lte.rsrq);
}

/* Registering drops any status from before, so there is none until the serving cell is reported */
ZTEST(modem_cellular_network_status, test_no_data_until_reported)
{
	struct cellular_evt_network_status status;

	link_up();
	zassert_equal(-ENODATA, cellular_get_network_status(modem, &status));
	zassert_false(periodic(NULL), "status reported without a serving cell");
	zassert_equal(-ENODATA, cellular_get_network_status(modem, &status));
}

ZTEST(modem_cellular_network_status, test_serving_cell)
{
	struct cellular_evt_network_status status;

	link_up();
	zassert_true(periodic(SERVING_CELL(1234567, -95, -11)), "no network status event");
	expect_status(-95, -11, 1234567);

	/* The event carries the same status */
	zassert_ok(cellular_get_network_status(modem, &status));
	zassert_equal(status.status, last_event.status);
	zassert_equal(status.access_tech, last_event.access_tech);
	zassert_equal(status.cell.lte.gci, last_event.cell.lte.gci);
	zassert_equal(status.cell.lte.rsrp, last_event.cell.lte.rsrp);
	zassert_equal(status.cell.lte.rsrq, last_event.cell.lte.rsrq);
}

/* Signal quality alone is not a change, but the status still reports the latest values */
ZTEST(modem_cellular_network_status, test_signal_change)
{
	link_up();
	zassert_true(periodic(SERVING_CELL(1234567, -95, -11)), "no network status event");
	zassert_false(periodic(SERVING_CELL(1234567, -101, -14)), "event on a signal change");
	expect_status(-101, -14, 1234567);

	/* A new serving cell is */
	zassert_true(periodic(SERVING_CELL(7654321, -101, -14)), "no event on a cell change");
	expect_status(-101, -14, 7654321);
}

/* A change in registration status drops the status, which is stale until the next report */
ZTEST(modem_cellular_network_status, test_registration_change)
{
	struct cellular_evt_network_status status;

	link_up();
	zassert_true(periodic(SERVING_CELL(1234567, -95, -11)), "no network status event");

	/* The same status again keeps it */
	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, "+CEREG: 1\r");
	expect_event(CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED);
	zassert_ok(cellular_get_network_status(modem, &status));

	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, "+CEREG: 5\r");
	expect_event(CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED);
	zassert_equal(-ENODATA, cellular_get_network_status(modem, &status));
}

/*
 * Deregistration reports the status without a serving cell, with the access technology last
 * notified, and drops the link
 */
ZTEST(modem_cellular_network_status, test_deregistration)
{
	struct cellular_evt_network_status status;
	struct test_event evt;

	link_up();
	zassert_true(periodic(SERVING_CELL(1234567, -95, -11)), "no network status event");

	test_backend_respond(TEST_BACKEND_PIPE_DLCI1, "+CEREG: 0\r");
	evt = expect_event(CELLULAR_EVENT_NETWORK_STATUS_CHANGED);
	zassert_equal(CELLULAR_REGISTRATION_NOT_REGISTERED, evt.network.status);
	zassert_equal(CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN, evt.network.access_tech);
	expect_event(CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED);
	expect_state(modem, TEST_BACKEND_STATE_DIALED);

	zassert_ok(cellular_get_network_status(modem, &status));
	zassert_equal(CELLULAR_REGISTRATION_NOT_REGISTERED, status.status);
	zassert_equal(CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN, status.access_tech);
	zassert_equal(0, status.cell.lte.gci, "serving cell kept");
}

static void network_status_after(void *fixture)
{
	(void)cellular_set_callback(modem, 0, NULL, NULL);
	modem_test_after(fixture);
}

ZTEST_SUITE(modem_cellular_network_status, NULL, modem_test_setup, modem_test_before,
	    network_status_after, NULL);
