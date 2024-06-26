/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include "test_conn_impl.h"
#include "test_ifaces.h"

static inline struct test_conn_data *conn_mgr_if_get_data(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	if (!binding) {
		return NULL;
	}
	return binding->ctx;
}

/**
 * @brief Reset the network state of the provided iface.
 *
 * @param iface - iface to reset.
 */
static void reset_test_iface_networking(struct net_if *iface)
{
	if (net_if_is_admin_up(iface)) {
		(void)net_if_down(iface);
	}

	/* Some tests can leave the iface in a bad state where it is admin-down but not dormant */
	net_if_dormant_on(iface);
}

/**
 * @brief Reset testing state for the provided iface.
 *
 * @param iface - iface to reset.
 */
static void reset_test_iface_state(struct net_if *iface)
{
	struct conn_mgr_conn_binding *iface_binding = conn_mgr_if_get_binding(iface);
	struct test_conn_data   *iface_data    = conn_mgr_if_get_data(iface);

	/* Some tests mark ifaces as ignored, this must be reset between each test. */
	conn_mgr_watch_iface(iface);

	if (iface_binding) {
		/* Reset all flags and settings for the binding */
		iface_binding->flags = 0;
		iface_binding->timeout = CONN_MGR_IF_NO_TIMEOUT;

		/* Disable auto-connect and auto-down */
		conn_mgr_if_set_flag(iface, CONN_MGR_IF_NO_AUTO_CONNECT, true);
		conn_mgr_if_set_flag(iface, CONN_MGR_IF_NO_AUTO_DOWN, true);
	}

	if (iface_data) {
		iface_data->call_cnt_a = 0;
		iface_data->call_cnt_b = 0;
		iface_data->conn_bal = 0;
		iface_data->api_err = 0;
		iface_data->fatal_error = 0;
		iface_data->timeout = false;
		memset(iface_data->data_x, 0, sizeof(iface_data->data_x));
		memset(iface_data->data_y, 0, sizeof(iface_data->data_y));
	}
}


/* NET_MGMT event tracking */

static K_MUTEX_DEFINE(event_mutex);
static struct event_stats {
	int timeout_count;
	int fatal_error_count;
	int event_count;
	int event_info;
	struct net_if *event_iface;
} test_event_stats;

struct net_mgmt_event_callback conn_mgr_conn_callback;

static void conn_mgr_conn_handler(struct net_mgmt_event_callback *cb,
				  uint32_t event, struct net_if *iface)
{
	k_mutex_lock(&event_mutex, K_FOREVER);

	if (event == NET_EVENT_CONN_IF_TIMEOUT) {
		test_event_stats.timeout_count += 1;
	} else if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		test_event_stats.fatal_error_count += 1;
	}

	test_event_stats.event_count += 1;
	test_event_stats.event_iface = iface;

	if (cb->info) {
		test_event_stats.event_info = *((int *)cb->info);
	} else {
		test_event_stats.event_info = 0;
	}

	k_mutex_unlock(&event_mutex);
}

static void conn_mgr_conn_before(void *data)
{
	ARG_UNUSED(data);
	reset_test_iface_networking(ifa1);
	reset_test_iface_networking(ifa2);
	reset_test_iface_networking(ifb);
	reset_test_iface_networking(ifni);
	reset_test_iface_networking(ifnone);
	reset_test_iface_networking(ifnull);

	/* Allow any triggered events to shake out */
	k_sleep(SIMULATED_EVENT_WAIT_TIME);

	reset_test_iface_state(ifa1);
	reset_test_iface_state(ifa2);
	reset_test_iface_state(ifb);
	reset_test_iface_state(ifni);
	reset_test_iface_state(ifnone);
	reset_test_iface_state(ifnull);

	k_mutex_lock(&event_mutex, K_FOREVER);

	test_event_stats.event_count = 0;
	test_event_stats.timeout_count = 0;
	test_event_stats.fatal_error_count = 0;
	test_event_stats.event_iface = NULL;
	test_event_stats.event_info = 0;

	k_mutex_unlock(&event_mutex);
}

static void *conn_mgr_conn_setup(void)
{
	net_mgmt_init_event_callback(&conn_mgr_conn_callback, conn_mgr_conn_handler,
				     NET_EVENT_CONN_IF_TIMEOUT | NET_EVENT_CONN_IF_FATAL_ERROR);
	net_mgmt_add_event_callback(&conn_mgr_conn_callback);
	return NULL;
}

/* This suite uses k_sleep(K_MSEC(1)) to allow Zephyr to perform event propagation.
 * This is not guaranteed to execute in the fastest possible time, nor is it technically guaranteed
 * that Zephyr will finish its operations in less than a millisecond, but for this test suite,
 * event propagation times longer than a millisecond would be a sign of a problem,
 * a few milliseconds of delay are miniscule compared to the time it takes to build the suite,
 * and using k_sleep has the advantage of being completely agnostic to the underlying operation
 * of the events.
 */

/* Verify that the correct init APIs were called. */
ZTEST(conn_mgr_conn, test_inspect_init)
{
	/* This isn't a proper test in that it only verifies the result of an exterior operation,
	 * but it increases coverage and costs next to nothing to add.
	 */
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);
	struct test_conn_data *ifa2_data = conn_mgr_if_get_data(ifa2);
	struct test_conn_data *ifb_data  = conn_mgr_if_get_data(ifb);
	struct test_conn_data *ifni_data = conn_mgr_if_get_data(ifni);


	zassert_equal(ifa1_data->init_calls_a, 1, "ifa1->init should be called exactly once.");
	zassert_equal(ifa1_data->init_calls_b, 0, "ifa1 should use implementation A");

	zassert_equal(ifa2_data->init_calls_a, 1, "ifa2->init should be called exactly once.");
	zassert_equal(ifa2_data->init_calls_b, 0, "ifa2 should use implementation A");

	zassert_equal(ifb_data->init_calls_b,  1, "ifb->init should be called exactly once.");
	zassert_equal(ifb_data->init_calls_a,  0, "ifb should use implementation B");

	zassert_equal(ifni_data->init_calls_a,  0, "ifni->init should not be called.");
	zassert_equal(ifni_data->init_calls_b,  0, "ifni->init should not be called.");
}

/* Verify that conn_mgr_if_connect and conn_mgr_if_disconnect perform the
 * correct API calls to the correct interfaces and connectivity implementations
 */
ZTEST(conn_mgr_conn, test_connect_disconnect)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);
	struct test_conn_data *ifa2_data = conn_mgr_if_get_data(ifa2);
	struct test_conn_data *ifb_data  = conn_mgr_if_get_data(ifb);

	/* Take all ifaces up */
	zassert_equal(net_if_up(ifa1), 0,	"net_if_up should not fail");
	zassert_equal(net_if_up(ifa2), 0,	"net_if_up should not fail");
	zassert_equal(net_if_up(ifb), 0,	"net_if_up should not fail");
	k_sleep(K_MSEC(1));

	/* Verify ifaces are still disconnected */
	zassert_false(net_if_is_up(ifa1),	"Ifaces must be disconnected before test");
	zassert_false(net_if_is_up(ifa2),	"Ifaces must be disconnected before test");
	zassert_false(net_if_is_up(ifb),	"Ifaces must be disconnected before test");

	/* Connect one of the A ifaces */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should be oper-up after conn_mgr_if_connect");
	zassert_false(net_if_is_up(ifa2),	"ifa2 should not be affected by ifa1");
	zassert_false(net_if_is_up(ifb),	"ifb should not be affected by ifa1");

	/* Verify that all ifaces have the expected call counts and types */
	zassert_equal(ifa1_data->conn_bal, 1,	"ifa1->connect should be called once");
	zassert_equal(ifa1_data->call_cnt_a, 1,	"Implementation A should be used for ifa1");
	zassert_equal(ifa1_data->call_cnt_b, 0,	"Implementation A should be used for ifa1");

	zassert_equal(ifa2_data->conn_bal, 0,	"ifa2 should not be affected by ifa1");
	zassert_equal(ifa2_data->call_cnt_a, 0,	"ifa2 should not be affected by ifa1");
	zassert_equal(ifa2_data->call_cnt_b, 0,	"ifa2 should not be affected by ifa1");

	zassert_equal(ifb_data->conn_bal, 0,	"ifb should not be affected by ifa1");
	zassert_equal(ifb_data->call_cnt_a, 0,	"ifb should not be affected by ifa1");
	zassert_equal(ifb_data->call_cnt_b, 0,	"ifb should not be affected by ifa1");

	/* Now connect the B iface */
	zassert_equal(conn_mgr_if_connect(ifb), 0,	"conn_mgr_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should still be connected");
	zassert_false(net_if_is_up(ifa2),	"ifa2 should not be affected by ifb");
	zassert_true(net_if_is_up(ifb),		"ifb should be oper-up after conn_mgr_if_connect");

	/* Verify that all ifaces have the expected call counts and types */
	zassert_equal(ifa1_data->conn_bal, 1,	"ifa1 should not be affected by ifb");
	zassert_equal(ifa1_data->call_cnt_a, 1,	"ifa1 should not be affected by ifb");
	zassert_equal(ifa1_data->call_cnt_b, 0,	"ifa1 should not be affected by ifb");

	zassert_equal(ifa2_data->conn_bal, 0,	"ifa2 should not be affected by ifb");
	zassert_equal(ifa2_data->call_cnt_a, 0,	"ifa2 should not be affected by ifb");
	zassert_equal(ifa2_data->call_cnt_b, 0,	"ifa2 should not be affected by ifb");

	zassert_equal(ifb_data->conn_bal, 1,	"ifb->connect should be called once");
	zassert_equal(ifb_data->call_cnt_a, 0,	"Implementation B should be used for ifb");
	zassert_equal(ifb_data->call_cnt_b, 1,	"Implementation B should be used for ifb");

	/* Now connect the other A iface */
	zassert_equal(conn_mgr_if_connect(ifa2), 0,	"conn_mgr_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should still be connected");
	zassert_true(net_if_is_up(ifa2),	"ifa2 should be oper-up after conn_mgr_if_connect");
	zassert_true(net_if_is_up(ifb),		"ifb should still be connected");

	/* Verify that all ifaces have the expected call counts and types */
	zassert_equal(ifa1_data->conn_bal, 1,	"ifa1 should not be affected by ifa2");
	zassert_equal(ifa1_data->call_cnt_a, 1,	"ifa1 should not be affected by ifa2");
	zassert_equal(ifa1_data->call_cnt_b, 0,	"ifa1 should not be affected by ifa2");

	zassert_equal(ifa2_data->conn_bal, 1,	"ifa2->connect should be called once");
	zassert_equal(ifa2_data->call_cnt_a, 1,	"Implementation A should be used for ifa2");
	zassert_equal(ifa2_data->call_cnt_b, 0,	"Implementation A should be used for ifa2");

	zassert_equal(ifb_data->conn_bal, 1,		"ifb should not be affected by ifa2");
	zassert_equal(ifb_data->call_cnt_a, 0,		"ifb should not be affected by ifa2");
	zassert_equal(ifb_data->call_cnt_b, 1,		"ifb should not be affected by ifa2");

	/* Now disconnect the original A iface */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0, "conn_mgr_if_disconnect should not fail");
	k_sleep(K_MSEC(1));


	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be oper-down after conn_mgr_if_disconnect");
	zassert_true(net_if_is_up(ifa2),  "ifa2 should not be affected by ifa1");
	zassert_true(net_if_is_up(ifb),   "ifb should not be affected by ifa1");

	/* Verify that all ifaces have the expected call counts and types */
	zassert_equal(ifa1_data->conn_bal, 0,		"ifa1->disconnect should be called once");
	zassert_equal(ifa1_data->call_cnt_a, 2,		"Implementation A should be used for ifa1");
	zassert_equal(ifa1_data->call_cnt_b, 0,		"Implementation A should be used for ifa1");

	zassert_equal(ifa2_data->conn_bal, 1,		"ifa2 should not be affected by ifa1");
	zassert_equal(ifa2_data->call_cnt_a, 1,		"ifa2 should not be affected by ifa1");
	zassert_equal(ifa2_data->call_cnt_b, 0,		"ifa2 should not be affected by ifa1");

	zassert_equal(ifb_data->conn_bal, 1,		"ifb should not be affected by ifa1");
	zassert_equal(ifb_data->call_cnt_a, 0,		"ifb should not be affected by ifa1");
	zassert_equal(ifb_data->call_cnt_b, 1,		"ifb should not be affected by ifa1");

	/* Now disconnect the B iface */
	zassert_equal(conn_mgr_if_disconnect(ifb), 0, "conn_mgr_if_disconnect should not fail");
	k_sleep(K_MSEC(1));


	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_false(net_if_is_up(ifa1), "ifa1 should still be disconnected");
	zassert_true(net_if_is_up(ifa2),  "ifa2 should not be affected by ifb");
	zassert_false(net_if_is_up(ifb),  "ifb should be oper-down after conn_mgr_if_disconnect");

	/* Verify that all ifaces have the expected call counts and types */
	zassert_equal(ifa1_data->conn_bal, 0,		"ifa1 should not be affected by ifb");
	zassert_equal(ifa1_data->call_cnt_a, 2,		"ifa1 should not be affected by ifb");
	zassert_equal(ifa1_data->call_cnt_b, 0,		"ifa1 should not be affected by ifb");

	zassert_equal(ifa2_data->conn_bal, 1,		"ifa1 should not be affected by ifb");
	zassert_equal(ifa2_data->call_cnt_a, 1,		"ifa1 should not be affected by ifb");
	zassert_equal(ifa2_data->call_cnt_b, 0,		"ifa1 should not be affected by ifb");

	zassert_equal(ifb_data->conn_bal, 0,		"ifa1->disconnect should be called once");
	zassert_equal(ifb_data->call_cnt_a, 0,		"Implementation B should be used for ifb");
	zassert_equal(ifb_data->call_cnt_b, 2,		"Implementation B should be used for ifb");

	/* Finally, disconnect the last A iface */
	zassert_equal(conn_mgr_if_disconnect(ifa2), 0, "conn_mgr_if_disconnect should not fail");
	k_sleep(K_MSEC(1));


	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_false(net_if_is_up(ifa1), "ifa1 should still be disconnected");
	zassert_false(net_if_is_up(ifa2), "ifa2 should be oper-down after conn_mgr_if_disconnect");
	zassert_false(net_if_is_up(ifb),  "ifb should still be disconnected");

	/* Verify that all ifaces have the expected call counts and types */
	zassert_equal(ifa1_data->conn_bal, 0,		"ifa1 should not be affected by ifa2");
	zassert_equal(ifa1_data->call_cnt_a, 2,		"ifa1 should not be affected by ifa2");
	zassert_equal(ifa1_data->call_cnt_b, 0,		"ifa1 should not be affected by ifa2");

	zassert_equal(ifa2_data->conn_bal, 0,		"ifa2->disconnect should be called once");
	zassert_equal(ifa2_data->call_cnt_a, 2,		"Implementation A should be used for ifa2");
	zassert_equal(ifa2_data->call_cnt_b, 0,		"Implementation A should be used for ifa2");

	zassert_equal(ifb_data->conn_bal, 0,		"ifb should not be affected by ifa2");
	zassert_equal(ifb_data->call_cnt_a, 0,		"ifb should not be affected by ifa2");
	zassert_equal(ifb_data->call_cnt_b, 2,		"ifb should not be affected by ifa2");
}

/* Verify that double calls to conn_mgr_if_connect and conn_mgr_if_disconnect cause no problems */
ZTEST(conn_mgr_conn, test_connect_disconnect_double_delayed)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail");

	/* Connect iface */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_true(net_if_is_up(ifa1), "ifa1 should be oper-up after conn_mgr_if_connect");
	zassert_equal(ifa1_data->conn_bal, 1, "ifa1->connect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 1,	"ifa1->connect should have been called once.");

	/* Connect iface again */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success
	 * To be clear: Yes, ifa1->connect should be called twice. It is up to the L2
	 * connectivity implementation to either handle idempotence
	 */
	zassert_true(net_if_is_up(ifa1), "ifa1 should still be connected");
	zassert_equal(ifa1_data->conn_bal, 2, "ifa1->connect should have been called again.");
	zassert_equal(ifa1_data->call_cnt_a, 2, "ifa1->connect should have been called again.");

	/* Now disconnect the iface */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0, "conn_mgr_if_disconnect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be oper-down after conn_mgr_if_disconnect");
	zassert_equal(ifa1_data->conn_bal, 1, "ifa1->disconnect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 3, "ifa1->disconnect should have been called once.");

	/* Disconnect again! */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0, "conn_mgr_if_disconnect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be oper-down after conn_mgr_if_disconnect");
	zassert_equal(ifa1_data->conn_bal, 0, "ifa1->disconnect should have been called again.");
	zassert_equal(ifa1_data->call_cnt_a, 4, "ifa1->disconnect should have been called again.");
}

/* Verify that fast double calls to conn_mgr_if_connect and conn_mgr_if_disconnect
 * do not fail
 */
ZTEST(conn_mgr_conn, test_connect_disconnect_double_instant)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail");

	/* Connect twice */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should not fail");
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should be oper-up after conn_mgr_if_connect");
	zassert_equal(ifa1_data->conn_bal, 2,	"ifa1->connect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 2,	"ifa1->connect should have been called once.");

	/* Now disconnect twice */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0, "conn_mgr_if_disconnect should not fail");
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0, "conn_mgr_if_disconnect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be oper-down after conn_mgr_if_disconnect");
	zassert_equal(ifa1_data->conn_bal, 0, "ifa1->disconnect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 4, "ifa1->disconnect should have been called once.");
}

/* Verify that calling connect on a down iface automatically takes the iface up. */
ZTEST(conn_mgr_conn, test_connect_autoup)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Connect iface */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify net_if_up was called */
	zassert_true(net_if_is_admin_up(ifa1), "ifa1 should be admin-up after conn_mgr_if_connect");

	/* Verify that connection succeeds */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should be oper-up after conn_mgr_if_connect");
	zassert_equal(ifa1_data->conn_bal, 1,	"ifa1->connect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 1,	"ifa1->connect should have been called once.");
}

/* Verify that calling disconnect on a down iface has no effect and raises no error. */
ZTEST(conn_mgr_conn, test_disconnect_down)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Disconnect iface */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0, "conn_mgr_if_disconnect should not fail.");
	k_sleep(K_MSEC(1));

	/* Verify iface is still down */
	zassert_false(net_if_is_admin_up(ifa1), "ifa1 should be still be admin-down.");

	/* Verify that no callbacks were fired */
	zassert_equal(ifa1_data->conn_bal, 0,	"No callbacks should have been fired.");
	zassert_equal(ifa1_data->call_cnt_a, 0,	"No callbacks should have been fired.");
}

/**
 * Verify that invalid bound ifaces are treated as though they are not bound at all.
 */
ZTEST(conn_mgr_conn, test_invalid_ignored)
{
	zassert_is_null(conn_mgr_if_get_binding(ifnull));
	zassert_is_null(conn_mgr_if_get_binding(ifnone));
	zassert_false(conn_mgr_if_is_bound(ifnull));
	zassert_false(conn_mgr_if_is_bound(ifnone));
}

/* Verify that connecting an iface that isn't up, missing an API,
 * or isn't connectivity-bound raises an error.
 */
ZTEST(conn_mgr_conn, test_connect_invalid)
{
	/* Bring ifnull and ifnone up */
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for ifnull");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for ifnone");

	/* Attempts to connect ifnull should fail, even if it is up */
	zassert_equal(conn_mgr_if_connect(ifnull), -ENOTSUP,
					"conn_mgr_if_connect should give -ENOTSUP for ifnull");

	/* Attempts to connect ifnone should fail, even if it is up */
	zassert_equal(conn_mgr_if_connect(ifnone), -ENOTSUP,
					"conn_mgr_if_connect should give -ENOTSUP for ifnone");
}

/* Verify that disconnecting an iface that isn't up, missing an API,
 * or isn't connectivity-bound raises an error.
 */
ZTEST(conn_mgr_conn, test_disconnect_invalid)
{
	/* Bring ifnull and ifnone up */
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for ifnull");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for ifnone");

	/* Attempts to disconnect ifnull should fail, even if it is up */
	zassert_equal(conn_mgr_if_disconnect(ifnull), -ENOTSUP,
				"conn_mgr_if_disconnect should give -ENOTSUP for ifnull");

	/* Attempts to disconnect ifnone should fail, even if it is up */
	zassert_equal(conn_mgr_if_disconnect(ifnone), -ENOTSUP,
				"conn_mgr_if_disconnect should give -ENOTSUP for ifnone");
}

/* Verify that conn_mgr_if_connect forwards error codes from API */
ZTEST(conn_mgr_conn, test_connect_fail)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Instruct ifa1 to fail on connect attempt */
	ifa1_data->api_err = -ECHILD;

	/* Take ifa1 up before attempting to connect */
	zassert_equal(net_if_up(ifa1), 0,
			"conn_mgr_if_connect should succeed");

	/* Attempts to connect ifa1 should return the expected error*/
	zassert_equal(conn_mgr_if_connect(ifa1), -ECHILD,
			"conn_mgr_if_connect should give -ECHILD");
}

/* Verify that conn_mgr_if_disconnect forwards error codes from API */
ZTEST(conn_mgr_conn, test_disconnect_fail)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Take up and connect iface first */
	zassert_equal(net_if_up(ifa1), 0,		"net_if_up should succeed");
	zassert_equal(conn_mgr_if_connect(ifa1), 0,	"conn_mgr_if_connect should succeed");

	/* Instruct ifa1 to fail on disconnect attempt */
	ifa1_data->api_err = -EDOM;

	/* Attempts to disconnect ifa1 should return the expected error*/
	zassert_equal(conn_mgr_if_disconnect(ifa1), -EDOM,
				"conn_mgr_if_disconnect should give -EDOM");
}

/* Verify that the NET_EVENT_CONN_IF_TIMEOUT event works as expected. */
ZTEST(conn_mgr_conn, test_connect_timeout)
{
	struct event_stats stats;
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* instruct ifa1 to timeout on connect */
	ifa1_data->timeout = true;

	/* Take up and attempt to connect iface */
	zassert_equal(net_if_up(ifa1), 0,		"net_if_up should succeed");

	zassert_equal(conn_mgr_if_connect(ifa1), 0,	"conn_mgr_if_connect should succeed");

	/* Confirm iface is not immediately connected */
	zassert_false(net_if_is_up(ifa1), "ifa1 should not be up if instructed to time out");

	/* Ensure timeout event is fired */
	k_sleep(SIMULATED_EVENT_WAIT_TIME);

	k_mutex_lock(&event_mutex, K_FOREVER);
	stats = test_event_stats;
	k_mutex_unlock(&event_mutex);

	zassert_equal(stats.timeout_count, 1,
		"NET_EVENT_CONN_IF_TIMEOUT should have been fired");
	zassert_equal(stats.event_count, 1,
		"only NET_EVENT_CONN_IF_TIMEOUT should have been fired");
	zassert_equal(stats.event_iface, ifa1,
		"Timeout event should be raised on ifa1");
}

/* Verify that the NET_EVENT_CONN_IF_FATAL_ERROR event works as expected. */
ZTEST(conn_mgr_conn, test_connect_fatal_error)
{
	struct event_stats stats;
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* instruct ifa1 to have fatal error on connect. */
	ifa1_data->fatal_error = -EADDRINUSE;

	/* Take up and attempt to connect iface */
	zassert_equal(net_if_up(ifa1), 0,		"net_if_up should succeed");
	zassert_equal(conn_mgr_if_connect(ifa1), 0,	"conn_mgr_if_connect should succeed");

	/* Confirm iface is not immediately connected */
	zassert_false(net_if_is_up(ifa1), "ifa1 should not be up if instructed to time out");

	/* Ensure fatal_error event is fired */
	k_sleep(SIMULATED_EVENT_WAIT_TIME);

	k_mutex_lock(&event_mutex, K_FOREVER);
	stats = test_event_stats;
	k_mutex_unlock(&event_mutex);

	zassert_equal(stats.fatal_error_count, 1,
		"NET_EVENT_CONN_IF_FATAL_ERROR should have been fired");
	zassert_equal(stats.event_count, 1,
		"only NET_EVENT_CONN_IF_FATAL_ERROR should have been fired");
	zassert_equal(stats.event_iface, ifa1,
		"Fatal error event should be raised on ifa1");
	zassert_equal(stats.event_info, -EADDRINUSE,
		"Fatal error info should be -EADDRINUSE");
}



/* Verify that conn_mgr_if_is_bound gives correct results */
ZTEST(conn_mgr_conn, test_supports_connectivity)
{
	zassert_true(conn_mgr_if_is_bound(ifa1));
	zassert_true(conn_mgr_if_is_bound(ifa2));
	zassert_true(conn_mgr_if_is_bound(ifb));
	zassert_false(conn_mgr_if_is_bound(ifnull));
	zassert_false(conn_mgr_if_is_bound(ifnone));
}

/* 60 characters long */
#define TEST_STR_LONG	"AAAAAaaaaaBBBBBbbbbbCCCCCcccccDDDDDdddddEEEEEeeeeeFFFFFfffff"

/* Verify that conn_opt get/set functions operate correctly and affect only the target iface */
ZTEST(conn_mgr_conn, test_conn_opt)
{
	char buf[100];
	size_t buf_len = 0;

	/* Set ifa1->X to "A" */
	strcpy(buf, "A");
	zassert_equal(conn_mgr_if_set_opt(ifa1, TEST_CONN_OPT_X, &buf, strlen(buf) + 1), 0,
		       "conn_mgr_if_set_opt should succeed for valid parameters");

	/* Verify success */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len),
		       0, "conn_mgr_if_get_opt should succeed for valid parameters");
	printk("%d, %d", buf_len, strlen(buf) + 1);
	zassert_equal(buf_len, strlen(buf) + 1, "conn_mgr_if_get_opt should return valid optlen");
	zassert_str_equal(buf, "A",
			  "conn_mgr_if_get_opt should retrieve \"A\"");

	/* Verify that ifa1->Y was not affected */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_Y, &buf, &buf_len),
		       0, "conn_mgr_if_get_opt should succeed for valid parameters");
	zassert_equal(buf_len, 1, "conn_mgr_if_get_opt should yield nothing for ifa1->Y");
	zassert_equal(buf[0], 0, "conn_mgr_if_get_opt should yield nothing for ifa1->Y");

	/* Verify that ifa2->X was not affected */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(conn_mgr_if_get_opt(ifa2, TEST_CONN_OPT_X, &buf, &buf_len),
		       0, "conn_mgr_if_get_opt should succeed for valid parameters");
	zassert_equal(buf_len, 1, "conn_mgr_if_get_opt should yield nothing for ifa2->X");
	zassert_equal(buf[0], 0, "conn_mgr_if_get_opt should yield nothing for ifa2->X");

	/* Now, set ifa->Y to "ABC" */
	strcpy(buf, "ABC");
	zassert_equal(conn_mgr_if_set_opt(ifa1, TEST_CONN_OPT_Y, &buf, strlen(buf) + 1), 0,
		       "conn_mgr_if_set_opt should succeed for valid parameters");

	/* Verify success */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_Y, &buf, &buf_len),
		       0, "conn_mgr_if_get_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1, "conn_mgr_if_get_opt should return valid optlen");
	zassert_str_equal(buf, "ABC",
			  "conn_mgr_if_get_opt should retrieve \"ABC\"");

	/* Verify that ifa1->X was not affected */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len),
		       0, "conn_mgr_if_get_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1, "conn_mgr_if_get_opt should return valid optlen");
	zassert_str_equal(buf, "A",
			  "conn_mgr_if_get_opt should retrieve \"A\"");

	/* Next, we pass some buffers that are too large or too small.
	 * This is an indirect way of verifying that buf_len is passed correctly.
	 */

	/* Try writing a string that is too large to ifa1->X */
	strcpy(buf, TEST_STR_LONG);
	zassert_equal(conn_mgr_if_set_opt(ifa1, TEST_CONN_OPT_X, &buf, strlen(buf) + 1), 0,
		       "conn_mgr_if_set_opt should succeed for valid parameters");

	/* Verify partial success */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len), 0,
				"conn_mgr_if_get_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1,
				"conn_mgr_if_get_opt should return valid optlen");

	/* This does, technically, test the test harness, but this test will fail if
	 * the unit under test (conn_mgr_if_set_opt) fails to pass along the optlen
	 */
	zassert_true(strlen(buf) < strlen(TEST_STR_LONG),
				"test_set_opt_a should truncate long values");

	/* For the same reason, verify that get_opt truncates given a small destination buffer */
	memset(buf, 0, sizeof(buf));
	buf_len = 10;
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len), 0,
				"conn_mgr_if_get_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1,
				"conn_mgr_if_get_opt should return valid optlen");
	zassert_equal(buf_len, 10,
				"test_get_opt_a should truncate if dest. buffer is too small.");
}

/* Verify that conn_mgr_if_get_opt and conn_mgr_if_set_opt behave as expected when given invalid
 * arguments.
 */
ZTEST(conn_mgr_conn, test_conn_opt_invalid)
{
	char buf[100];
	size_t buf_len;

	/* Verify that getting/setting non-existent option on ifa1 fails */
	zassert_equal(conn_mgr_if_set_opt(ifa1, -1, "A", strlen("A")), -ENOPROTOOPT,
				"conn_mgr_if_set_opt should fail with invalid optname");
	buf_len = sizeof(buf_len);
	zassert_equal(conn_mgr_if_get_opt(ifa1, -1, buf, &buf_len), -ENOPROTOOPT,
				"conn_mgr_if_get_opt should fail with invalid optname");
	zassert_equal(buf_len, 0, "failed conn_mgr_if_get_opt should always set buf_len to zero.");

	/* Verify that getting/setting with NULL buffer on ifa1 fails */
	zassert_equal(conn_mgr_if_set_opt(ifa1, TEST_CONN_OPT_X, NULL, 100), -EINVAL,
				"conn_mgr_if_set_opt should fail with invalid buffer");
	buf_len = sizeof(buf_len);
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_X, NULL, &buf_len), -EINVAL,
				"conn_mgr_if_get_opt should fail with invalid buffer");
	zassert_equal(buf_len, 0, "failed conn_mgr_if_get_opt should always set buf_len to zero.");

	/* Verify that getting with NULL buffer length on ifa1 fails */
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_X, buf, NULL), -EINVAL,
				"conn_mgr_if_get_opt should fail with invalid buffer length");

	/* Verify that getting/setting with ifnull fails */
	zassert_equal(conn_mgr_if_set_opt(ifnull, TEST_CONN_OPT_X, "A", strlen("A")), -ENOTSUP,
				"conn_mgr_if_set_opt should fail for ifnull");
	buf_len = sizeof(buf_len);
	zassert_equal(conn_mgr_if_get_opt(ifnull, TEST_CONN_OPT_X, buf, &buf_len), -ENOTSUP,
				"conn_mgr_if_get_opt should fail for ifnull");
	zassert_equal(buf_len, 0, "failed conn_mgr_if_get_opt should always set buf_len to zero.");

	/* Verify that getting/setting with ifnone fails */
	zassert_equal(conn_mgr_if_set_opt(ifnone, TEST_CONN_OPT_X, "A", strlen("A")), -ENOTSUP,
				"conn_mgr_if_set_opt should fail for ifnull");
	buf_len = sizeof(buf_len);
	zassert_equal(conn_mgr_if_get_opt(ifnone, TEST_CONN_OPT_X, buf, &buf_len), -ENOTSUP,
				"conn_mgr_if_get_opt should fail for ifnull");
	zassert_equal(buf_len, 0, "failed conn_mgr_if_get_opt should always set buf_len to zero.");

	/* Verify that getting/setting with ifb fails (since implementation B doesn't support it) */
	zassert_equal(conn_mgr_if_set_opt(ifb, TEST_CONN_OPT_X, "A", strlen("A")), -ENOTSUP,
				"conn_mgr_if_set_opt should fail for ifb");
	buf_len = sizeof(buf_len);
	zassert_equal(conn_mgr_if_get_opt(ifb, TEST_CONN_OPT_X, buf, &buf_len), -ENOTSUP,
				"conn_mgr_if_get_opt should fail for ifb");
	zassert_equal(buf_len, 0, "failed conn_mgr_if_get_opt should always set buf_len to zero.");
}

/* Verify that flag get/set functions operate correctly */
ZTEST(conn_mgr_conn, test_flags)
{
	struct conn_mgr_conn_binding *ifa1_binding = conn_mgr_if_get_binding(ifa1);

	/* Firstly, clear all flags (some are automatically enabled before each test) */
	ifa1_binding->flags = 0;

	/* Try setting persistence flag */
	zassert_equal(conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, true), 0,
				"Setting persistence flag should succeed for ifa1");

	/* Verify success */
	zassert_true(conn_mgr_if_get_flag(ifa1, CONN_MGR_IF_PERSISTENT),
				"Persistence should be set for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->flags, BIT(CONN_MGR_IF_PERSISTENT),
				"Persistence flag set should affect conn struct");

	/* Try setting no-autoconnect flag */
	zassert_equal(conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, true), 0,
				"Setting no-autoconnect flag should succeed for ifa1");

	/* Verify success */
	zassert_true(conn_mgr_if_get_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT),
				"No-autoconnect should be set for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->flags,
				BIT(CONN_MGR_IF_PERSISTENT) | BIT(CONN_MGR_IF_NO_AUTO_CONNECT),
				"Persistence flag set should affect conn struct");

	/* Try unsetting persistence flag */
	zassert_equal(conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, false), 0,
				"Unsetting persistence flag should succeed for ifa1");

	/* Verify success */
	zassert_false(conn_mgr_if_get_flag(ifa1, CONN_MGR_IF_PERSISTENT),
				"Persistence should be unset for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->flags, BIT(CONN_MGR_IF_NO_AUTO_CONNECT),
				"Persistence flag unset should affect conn struct");

	/* Try unsetting no-autoconnect flag */
	zassert_equal(conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, false), 0,
				"Clearing no-autoconnect flag should succeed for ifa1");

	/* Verify success */
	zassert_false(conn_mgr_if_get_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT),
				"No-autoconnect should be set for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->flags, 0,
				"No-autoconnect flag set should affect conn struct");
}

/* Verify that flag get/set fail and behave as expected respectively for invalid ifaces and
 * invalid flags.
 */
ZTEST(conn_mgr_conn, test_flags_invalid)
{
	int invalid_flag = CONN_MGR_NUM_IF_FLAGS;

	/* Verify set failure for invalid ifaces / flags */
	zassert_equal(conn_mgr_if_set_flag(ifnull, CONN_MGR_IF_PERSISTENT, true), -ENOTSUP,
		"Setting persistence flag should fail for ifnull");
	zassert_equal(conn_mgr_if_set_flag(ifnone, CONN_MGR_IF_PERSISTENT, true), -ENOTSUP,
		"Setting persistence flag should fail for ifnone");
	zassert_equal(conn_mgr_if_set_flag(ifa1, invalid_flag, true), -EINVAL,
		"Setting invalid flag should fail for ifa1");

	/* Verify get graceful behavior for invalid ifaces / flags */
	zassert_false(conn_mgr_if_get_flag(ifnull, CONN_MGR_IF_PERSISTENT),
		"Getting persistence flag should yield false for ifnull");
	zassert_false(conn_mgr_if_get_flag(ifnone, CONN_MGR_IF_PERSISTENT),
		"Getting persistence flag should yield false for ifnone");
	zassert_false(conn_mgr_if_get_flag(ifa1, invalid_flag),
		"Getting invalid flag should yield false for ifa1");
}


/* Verify that timeout get/set functions operate correctly (A/B) */
ZTEST(conn_mgr_conn, test_timeout)
{
	struct conn_mgr_conn_binding *ifa1_binding = conn_mgr_if_get_binding(ifa1);

	/* Try setting timeout */
	zassert_equal(conn_mgr_if_set_timeout(ifa1, 99), 0,
				"Setting timeout should succeed for ifa1");

	/* Verify success */
	zassert_equal(conn_mgr_if_get_timeout(ifa1), 99,
				"Timeout should be set to 99 for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->timeout, 99,
				"Timeout set should affect conn struct");

	/* Try unsetting timeout */
	zassert_equal(conn_mgr_if_set_timeout(ifa1, CONN_MGR_IF_NO_TIMEOUT), 0,
				"Unsetting timeout should succeed for ifa1");

	/* Verify success */
	zassert_equal(conn_mgr_if_get_timeout(ifa1), CONN_MGR_IF_NO_TIMEOUT,
				"Timeout should be unset for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->timeout, CONN_MGR_IF_NO_TIMEOUT,
				"Timeout unset should affect conn struct");
}

/* Verify that timeout get/set fail and behave as expected respectively for invalid ifaces */
ZTEST(conn_mgr_conn, test_timeout_invalid)
{
	/* Verify set failure */
	zassert_equal(conn_mgr_if_set_timeout(ifnull, 99), -ENOTSUP,
		"Setting timeout should fail for ifnull");
	zassert_equal(conn_mgr_if_set_timeout(ifnone, 99), -ENOTSUP,
		"Setting timeout should fail for ifnone");

	/* Verify get graceful behavior */
	zassert_equal(conn_mgr_if_get_timeout(ifnull), CONN_MGR_IF_NO_TIMEOUT,
		"Getting timeout should yield CONN_MGR_IF_NO_TIMEOUT for ifnull");
	zassert_equal(conn_mgr_if_get_timeout(ifnone), CONN_MGR_IF_NO_TIMEOUT,
		"Getting timeout should yield CONN_MGR_IF_NO_TIMEOUT for ifnone");
}

/* Verify that auto-connect works as expected. */
ZTEST(conn_mgr_conn, test_auto_connect)
{
	/* Disable auto-connect.
	 * Not strictly necessary, since this is the default for this suite, but do it anyways
	 * since this test case specifically focuses on auto-connect.
	 */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, true);

	/* Take the iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail.");

	/* Verify no connection */
	k_sleep(K_MSEC(1));
	zassert_false(net_if_is_up(ifa1), "Auto-connect should not trigger if disabled.");

	/* Take the iface down */
	zassert_equal(net_if_down(ifa1), 0, "net_if_down should not fail.");

	/* Enable auto-connect */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, false);

	/* Take the iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail.");

	/* Verify connection */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Auto-connect should succeed if enabled.");
}

/* Verify that if auto-down is enabled, disconnecting an iface also takes it down,
 * regardless of whether persistence is enabled, but only if auto-down is disabled.
 */
ZTEST(conn_mgr_conn, test_auto_down_disconnect)
{
	/* For convenience, use auto-connect for this test. */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, false);

	/* Enable auto-down, disable persistence */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, false);
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, false);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Disconnect iface */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0,
		"conn_mgr_if_disconnect should succeed.");

	/* Verify down */
	k_sleep(K_MSEC(1));
	zassert_false(net_if_is_admin_up(ifa1),
		"Auto-down should trigger on direct disconnect.");



	/* Enable persistence */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, true);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Disconnect iface */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0,
		"conn_mgr_if_disconnect should succeed.");

	/* Verify down */
	k_sleep(K_MSEC(1));
	zassert_false(net_if_is_admin_up(ifa1),
		"Auto-down should trigger on direct disconnect, even if persistence is enabled.");



	/* Disable auto-down */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, true);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Disconnect iface */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0,
		"conn_mgr_if_disconnect should succeed.");

	/* Verify up */
	zassert_true(net_if_is_admin_up(ifa1),
		"Auto-down should not trigger if it is disabled.");
}

/* Verify that auto-down takes an iface down if connection is lost, but only if persistence is not
 * enabled, and only if auto-down is enabled.
 */
ZTEST(conn_mgr_conn, test_auto_down_conn_loss)
{
	/* For convenience, use auto-connect for this test. */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, false);

	/* Enable auto-down, disable persistence */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, false);
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, false);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Simulate connection loss */
	simulate_connection_loss(ifa1);

	/* Verify down */
	k_sleep(K_MSEC(1));
	zassert_false(net_if_is_admin_up(ifa1),
		"Auto-down should trigger on connection loss if persistence is disabled.");

	/* Enable persistence */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, true);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Simulate connection loss */
	simulate_connection_loss(ifa1);

	/* Verify up */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_admin_up(ifa1),
		"Auto-down should not trigger on connection loss if persistence is enabled.");

	/* Disable persistence and disable auto-down*/
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, false);
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, true);

	/* Reconnect iface */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Simulate connection loss */
	simulate_connection_loss(ifa1);

	/* Verify up */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_admin_up(ifa1),
		"Auto-down should not trigger on connection loss if it is disabled.");
}

/* Verify that timeout takes the iface down, even if
 * persistence is enabled, but only if auto-down is enabled.
 */
ZTEST(conn_mgr_conn, test_auto_down_timeout)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* For convenience, use auto-connect for this test. */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, false);

	/* Enable auto-down and persistence*/
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, true);
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, false);

	/* Schedule timeout */
	ifa1_data->timeout = true;

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify iface down after timeout */
	k_sleep(SIMULATED_EVENT_WAIT_TIME);
	zassert_false(net_if_is_admin_up(ifa1),
		"Auto-down should trigger on connection timeout, even if persistence is enabled.");

	/* Disable auto-down */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, true);

	/* Take iface up (timing out again) */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify iface up after timeout */
	k_sleep(SIMULATED_EVENT_WAIT_TIME);
	zassert_true(net_if_is_admin_up(ifa1),
		"Auto-down should not trigger on connection timeout if it is disabled.");
}


/* Verify that fatal error takes the iface down, even if
 * persistence is enabled, but only if auto-down is enabled.
 */
ZTEST(conn_mgr_conn, test_auto_down_fatal)
{
	/* For convenience, use auto-connect for this test. */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_CONNECT, false);

	/* Enable auto-down and persistence */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, true);
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, false);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Raise fatal error */
	simulate_fatal_error(ifa1, -EAGAIN);

	/* Verify iface down after fatal error */
	k_sleep(SIMULATED_EVENT_WAIT_TIME);
	zassert_false(net_if_is_admin_up(ifa1),
		"Auto-down should trigger on fatal error, even if persistence is enabled.");

	/* Disable auto-down */
	conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_NO_AUTO_DOWN, true);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should succeed.");

	/* Verify connected */
	k_sleep(K_MSEC(1));
	zassert_true(net_if_is_up(ifa1), "Connection should succeed.");

	/* Raise fatal error */
	simulate_fatal_error(ifa1, -EAGAIN);

	/* Verify iface still up after fatal error */
	k_sleep(SIMULATED_EVENT_WAIT_TIME);
	zassert_true(net_if_is_admin_up(ifa1),
		"Auto-down should not trigger on fatal error if it is disabled.");
}

/* Verify that all_if_up brings all ifaces up, but only if they are not ignored or
 * skip_ignored is false
 */
ZTEST(conn_mgr_conn, test_all_if_up)
{
	/* Ignore an iface */
	conn_mgr_ignore_iface(ifa1);

	/* Take all ifaces up (do not skip ignored) */
	zassert_equal(conn_mgr_all_if_up(false), 0, "conn_mgr_all_if_up should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify all ifaces are up */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");


	/* Manually take all ifaces down */
	zassert_equal(net_if_down(ifa1),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifa2),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifb),		 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifni),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifnull),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifnone),	 0, "net_if_down should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Take all ifaces up (skip ignored) */
	zassert_equal(conn_mgr_all_if_up(true), 0, "conn_mgr_all_if_up should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify all except ignored are up */
	zassert_true(net_if_is_admin_up(ifa2),	 "All non-ignored ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All non-ignored ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All non-ignored ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All non-ignored ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All non-ignored ifaces should be admin-up.");

	zassert_false(net_if_is_admin_up(ifa1),		"Ignored iface should not be admin-up.");
}

/* Verify that all_if_connect brings all ifaces up, and connects all bound ifaces, but only those
 * that are not ignored, or all of them if skip_ignored is false
 */
ZTEST(conn_mgr_conn, test_all_if_connect)
{
	/* Ignore a bound and an unbound iface */
	conn_mgr_ignore_iface(ifa1);
	conn_mgr_ignore_iface(ifnone);

	/* Connect all ifaces (do not skip ignored) */
	zassert_equal(conn_mgr_all_if_connect(false), 0, "conn_mgr_all_if_connect should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify all ifaces are up */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");

	/* Verify bound ifaces are connected */
	zassert_true(net_if_is_up(ifa1),	 "All bound ifaces should be connected.");
	zassert_true(net_if_is_up(ifa2),	 "All bound ifaces should be connected.");
	zassert_true(net_if_is_up(ifb),		 "All bound ifaces should be connected.");
	zassert_true(net_if_is_up(ifni),	 "All bound ifaces should be connected.");

	/* Manually take all ifaces down */
	zassert_equal(conn_mgr_if_disconnect(ifa1), 0, "net_if_disconnect should succeed.");
	zassert_equal(conn_mgr_if_disconnect(ifa2), 0, "net_if_disconnect should succeed.");
	zassert_equal(conn_mgr_if_disconnect(ifb),  0, "net_if_disconnect should succeed.");
	zassert_equal(conn_mgr_if_disconnect(ifni), 0, "net_if_disconnect should succeed.");

	zassert_equal(net_if_down(ifa1),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifa2),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifb),		 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifni),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifnull),	 0, "net_if_down should succeed for all ifaces.");
	zassert_equal(net_if_down(ifnone),	 0, "net_if_down should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Connect all ifaces (skip ignored) */
	zassert_equal(conn_mgr_all_if_connect(true), 0, "conn_mgr_all_if_connect should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify all except ignored are up */
	zassert_true(net_if_is_admin_up(ifa2),	 "All non-ignored ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All non-ignored ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All non-ignored ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All non-ignored ifaces should be admin-up.");

	zassert_false(net_if_is_admin_up(ifa1),	  "All ignored ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifnone), "All ignored ifaces should be admin-down.");

	/* Verify bound ifaces are connected, except for ignored */
	zassert_true(net_if_is_up(ifa2), "All non-ignored bound ifaces should be connected.");
	zassert_true(net_if_is_up(ifb),  "All non-ignored bound ifaces should be connected.");
	zassert_true(net_if_is_up(ifni), "All non-ignored bound ifaces should be connected.");

	zassert_false(net_if_is_up(ifa1), "Ignored iface should not be connected.");
}

/* Verify that all_if_down takes all ifaces down, but only if they are not ignored,
 * or skip_ignored is false
 */
ZTEST(conn_mgr_conn, test_all_if_down)
{
	/* Ignore an iface */
	conn_mgr_ignore_iface(ifa1);

	/* Manually take all ifaces up */
	zassert_equal(net_if_up(ifa1),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifa2),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifb),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifni),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Take all ifaces down (do not skip ignored) */
	zassert_equal(conn_mgr_all_if_down(false), 0, "conn_mgr_all_if_down should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify all ifaces are down */
	zassert_false(net_if_is_admin_up(ifa1),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifa2),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifb),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifni),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifnull), "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifnone), "All ifaces should be admin-down.");

	/* Manually take all ifaces up */
	zassert_equal(net_if_up(ifa1),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifa2),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifb),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifni),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Take all ifaces down (skip ignored)  */
	zassert_equal(conn_mgr_all_if_down(true), 0, "conn_mgr_all_if_down should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify that all except the ignored iface is down */
	zassert_false(net_if_is_admin_up(ifa2),	  "All non-ignored ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifb),	  "All non-ignored ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifni),	  "All non-ignored ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifnull), "All non-ignored ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifnone), "All non-ignored ifaces should be admin-down.");

	zassert_true(net_if_is_admin_up(ifa1),	 "Ignored iface should be admin-up.");
}

/* Verify that all_if_disconnect disconnects all bound ifaces, but only if they are not ignored,
 * or skip_ignored is false
 */
ZTEST(conn_mgr_conn, test_all_if_disconnect)
{
	/* Ignore a bound iface */
	conn_mgr_ignore_iface(ifa1);

	/* Manually take all ifaces up */
	zassert_equal(net_if_up(ifa1),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifa2),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifb),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifni),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Manually connect all bound ifaces */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifa2), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifb),	 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifni), 0, "conn_mgr_if_connect should succeed.");
	k_sleep(K_MSEC(1));

	/* Disconnect all ifaces (do not skip ignored) */
	zassert_equal(conn_mgr_all_if_disconnect(false), 0,
			"conn_mgr_all_if_disconnect should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify that all bound ifaces are disconnected */
	zassert_false(net_if_is_up(ifa1),	"All bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifa2),	"All bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifb),	"All bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifni),	"All bound ifaces should be disconnected.");

	/* Verify that all ifaces are still up, even if disconnected */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");

	/* Manually reconnect bound ifaces */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifa2), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifb),	 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifni), 0, "conn_mgr_if_connect should succeed.");
	k_sleep(K_MSEC(1));

	/* Disconnect all ifaces (skip ignored) */
	zassert_equal(conn_mgr_all_if_disconnect(true), 0,
			"conn_mgr_all_if_disconnect should succeed.");
	k_sleep(K_MSEC(1));

	/* Verify that all bound ifaces are disconnected, except the ignored iface */
	zassert_false(net_if_is_up(ifa2), "All non-ignored bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifb),  "All non-ignored bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifni), "All non-ignored bound ifaces should be disconnected.");

	zassert_true(net_if_is_up(ifa1),  "Ignored iface should still be connected");
}


/* Verify that double calls to all_if_up do not raise errors */
ZTEST(conn_mgr_conn, test_all_if_up_double)
{
	/* Take all ifaces up twice in a row */
	zassert_equal(conn_mgr_all_if_up(false), 0,
			"conn_mgr_all_if_up should succeed.");
	zassert_equal(conn_mgr_all_if_up(false), 0,
			"conn_mgr_all_if_up should succeed twice in a row.");

	/* One more time, after a delay, to be sure */
	k_sleep(K_MSEC(1));
	zassert_equal(conn_mgr_all_if_up(false), 0,
			"conn_mgr_all_if_up should succeed twice in a row.");
	k_sleep(K_MSEC(1));

	/* Verify all ifaces are up */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");
}

/* Verify that double calls to all_if_down do not raise errors */
ZTEST(conn_mgr_conn, test_all_if_down_double)
{
	/* Manually take all ifaces up */
	zassert_equal(net_if_up(ifa1),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifa2),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifb),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifni),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Take all ifaces down twice in a row */
	zassert_equal(conn_mgr_all_if_down(false), 0,
			"conn_mgr_all_if_down should succeed.");
	zassert_equal(conn_mgr_all_if_down(false), 0,
			"conn_mgr_all_if_down should succeed twice in a row.");

	/* One more time, after a delay, to be sure */
	k_sleep(K_MSEC(1));
	zassert_equal(conn_mgr_all_if_down(false), 0,
			"conn_mgr_all_if_down should succeed twice in a row.");
	k_sleep(K_MSEC(1));

	/* Verify all ifaces are down */
	zassert_false(net_if_is_admin_up(ifa1),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifa2),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifb),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifni),	  "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifnull), "All ifaces should be admin-down.");
	zassert_false(net_if_is_admin_up(ifnone), "All ifaces should be admin-down.");
}

/* Verify that double calls to all_if_connect do not raise errors */
ZTEST(conn_mgr_conn, test_all_if_connect_double)
{
	/* Connect all ifaces twice in a row */
	zassert_equal(conn_mgr_all_if_connect(false), 0,
			"conn_mgr_all_if_connect should succeed.");
	zassert_equal(conn_mgr_all_if_connect(false), 0,
			"conn_mgr_all_if_connect should succeed twice in a row.");

	/* One more time, after a delay, to be sure */
	k_sleep(K_MSEC(1));
	zassert_equal(conn_mgr_all_if_connect(false), 0,
			"conn_mgr_all_if_connect should succeed twice in a row.");
	k_sleep(K_MSEC(1));

	/* Verify all ifaces are up */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");

	/* Verify all bound ifaces are connected */
}

/* Verify that double calls to all_if_disconnect do not raise errors */
ZTEST(conn_mgr_conn, test_all_if_disconnect_double)
{
	/* Manually take all ifaces up */
	zassert_equal(net_if_up(ifa1),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifa2),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifb),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifni),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Manually connect all bound ifaces */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifa2), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifb),	 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifni), 0, "conn_mgr_if_connect should succeed.");
	k_sleep(K_MSEC(1));

	/* Connect all ifaces twice in a row */
	zassert_equal(conn_mgr_all_if_disconnect(false), 0,
			"conn_mgr_all_if_disconnect should succeed.");
	zassert_equal(conn_mgr_all_if_disconnect(false), 0,
			"conn_mgr_all_if_disconnect should succeed twice in a row.");

	/* One more time, after a delay, to be sure */
	k_sleep(K_MSEC(1));
	zassert_equal(conn_mgr_all_if_disconnect(false), 0,
			"conn_mgr_all_if_disconnect should succeed twice in a row.");
	k_sleep(K_MSEC(1));

	/* Verify all bound ifaces are disconnected */
	zassert_false(net_if_is_up(ifa1),	"All bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifa2),	"All bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifb),	"All bound ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifni),	"All bound ifaces should be disconnected.");

	/* Verify all ifaces are up */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");
}



/* Testing error passing for all_if_up/all_if_down is not possible without using an L2 other than
 * Dummy, since the dummy L2 is not capable of erroring in response to either of these.
 *
 * However, since all bulk convenience functions share a single implementation, testing
 * connect and disconnect is sufficient to gain acceptable coverage of this behavior for all of
 * them.
 */

/* Verify that all_if_connect successfully forwards errors encountered on individual ifaces */
ZTEST(conn_mgr_conn, test_all_if_connect_err)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Schedule a connect error on one of the ifaces */
	ifa1_data->api_err = -ECHILD;

	/* Verify that this error is passed to all_if_connect */
	zassert_equal(conn_mgr_all_if_connect(false), -ECHILD,
			"conn_mgr_all_if_connect should fail with the requested error.");
	k_sleep(K_MSEC(1));

	/* Verify that all ifaces went admin-up */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");

	/* Verify that all the non-error ifaces are connected */
	zassert_true(net_if_is_up(ifa2),	 "All non-failing ifaces should be connected.");
	zassert_true(net_if_is_up(ifb),		 "All non-failing ifaces should be connected.");
	zassert_true(net_if_is_up(ifni),	 "All non-failing ifaces should be connected.");

	/* Verify that the error iface is not connected */
	zassert_false(net_if_is_up(ifa1),	 "The failing iface should not be connected.");
}

/* Verify that all_if_disconnect successfully forwards errors encountered on individual ifaces */
ZTEST(conn_mgr_conn, test_all_if_disconnect_err)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Manually take all ifaces up */
	zassert_equal(net_if_up(ifa1),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifa2),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifb),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifni),	 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for all ifaces.");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for all ifaces.");
	k_sleep(K_MSEC(1));

	/* Manually connect all bound ifaces */
	zassert_equal(conn_mgr_if_connect(ifa1), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifa2), 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifb),	 0, "conn_mgr_if_connect should succeed.");
	zassert_equal(conn_mgr_if_connect(ifni), 0, "conn_mgr_if_connect should succeed.");
	k_sleep(K_MSEC(1));

	/* Schedule a disconnect error on one of the ifaces */
	ifa1_data->api_err = -ECHILD;

	/* Verify that this error is passed to all_if_disconnect */
	zassert_equal(conn_mgr_all_if_disconnect(false), -ECHILD,
			"conn_mgr_all_if_disconnect should fail with the requested error.");

	/* Verify that all ifaces are still admin-up */
	zassert_true(net_if_is_admin_up(ifa1),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifa2),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifb),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifni),	 "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnull), "All ifaces should be admin-up.");
	zassert_true(net_if_is_admin_up(ifnone), "All ifaces should be admin-up.");

	/* Verify that all the non-error ifaces are disconnected */
	zassert_false(net_if_is_up(ifa2),	 "All non-failing ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifb),	 "All non-failing ifaces should be disconnected.");
	zassert_false(net_if_is_up(ifni),	 "All non-failing ifaces should be disconnected.");

	/* Verify that the error iface is not connected */
	zassert_true(net_if_is_up(ifa1),	 "The failing iface should not be disconnected.");
}

ZTEST_SUITE(conn_mgr_conn, NULL, conn_mgr_conn_setup, conn_mgr_conn_before, NULL, NULL);
