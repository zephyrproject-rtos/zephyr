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
#include <zephyr/net/conn_mgr_connectivity.h>
#include "test_conn_impl.h"
#include "test_ifaces.h"


/* This is a duplicate of conn_mgr_if_get_binding in net_if.c,
 * which is currently not exposed.
 */
static inline struct conn_mgr_conn_binding *conn_mgr_if_get_binding(struct net_if *iface)
{
	STRUCT_SECTION_FOREACH(conn_mgr_conn_binding, binding) {
		if (iface == binding->iface) {
			if (binding->impl->api) {
				return binding;
			}
			return NULL;
		}
	}
	return NULL;
}

static inline struct test_conn_data *conn_mgr_if_get_data(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	if (!binding) {
		return NULL;
	}
	return binding->ctx;
}

static void reset_test_iface(struct net_if *iface)
{
	if (net_if_is_admin_up(iface)) {
		(void)net_if_down(iface);
	}

	/* Some tests can leave the iface in a bad state where it is admin-down but not dormant */
	net_if_dormant_on(iface);

	struct conn_mgr_conn_binding *iface_binding = conn_mgr_if_get_binding(iface);
	struct test_conn_data   *iface_data    = conn_mgr_if_get_data(iface);

	if (iface_binding) {
		iface_binding->flags = 0;
		iface_binding->timeout = CONN_MGR_IF_NO_TIMEOUT;
	}

	if (iface_data) {
		iface_data->call_cnt_a = 0;
		iface_data->call_cnt_b = 0;
		iface_data->conn_bal = 0;
		iface_data->api_err = 0;
		memset(iface_data->data_x, 0, sizeof(iface_data->data_x));
		memset(iface_data->data_y, 0, sizeof(iface_data->data_y));
	}
}

static void conn_mgr_conn_before(void *data)
{
	ARG_UNUSED(data);
	reset_test_iface(ifa1);
	reset_test_iface(ifa2);
	reset_test_iface(ifb);
	reset_test_iface(ifni);
	reset_test_iface(ifnone);
	reset_test_iface(ifnull);
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

/* Verify that connecting an iface that isn't up, missing an API,
 * or isn't connectivity-bound raises an error.
 */
ZTEST(conn_mgr_conn, test_connect_invalid)
{
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Bring ifnull and ifnone up */
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for ifnull");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for ifnone");

	/* Attempts to connect ifa1 without bringing it up should fail */
	zassert_equal(conn_mgr_if_connect(ifa1), -ESHUTDOWN,
					"conn_mgr_if_connect should give -ENOTSUP for down iface");
	zassert_equal(ifa1_data->conn_bal, 0,
					"conn_mgr_if_connect should not affect down iface");
	zassert_equal(ifa1_data->call_cnt_a, 0,
					"conn_mgr_if_connect should not affect down iface");

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
	struct test_conn_data *ifa1_data = conn_mgr_if_get_data(ifa1);

	/* Bring ifnull and ifnone up */
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for ifnull");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for ifnone");

	/* Attempts to disconnect ifa1 without bringing it up should fail */
	zassert_equal(conn_mgr_if_disconnect(ifa1), -EALREADY,
				"conn_mgr_if_disconnect should give -ENOTSUP for down iface");
	zassert_equal(ifa1_data->conn_bal, 0,
				"conn_mgr_if_disconnect should not affect down iface");
	zassert_equal(ifa1_data->call_cnt_a, 0,
				"conn_mgr_if_disconnect should not affect down iface");

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
	zassert_equal(strcmp(buf, "A"), 0, "conn_mgr_if_get_opt should retrieve \"A\"");

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
	zassert_equal(strcmp(buf, "ABC"), 0, "conn_mgr_if_get_opt should retrieve \"ABC\"");

	/* Verify that ifa1->X was not affected */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(conn_mgr_if_get_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len),
		       0, "conn_mgr_if_get_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1, "conn_mgr_if_get_opt should return valid optlen");
	zassert_equal(strcmp(buf, "A"), 0, "conn_mgr_if_get_opt should retrieve \"A\"");

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

	/* Try setting persistence flag */
	zassert_equal(conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, true), 0,
				"Setting persistence flag should succeed for ifa1");

	/* Verify success */
	zassert_true(conn_mgr_if_get_flag(ifa1, CONN_MGR_IF_PERSISTENT),
				"Persistence should be set for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->flags, BIT(CONN_MGR_IF_PERSISTENT),
				"Persistence flag set should affect conn struct");

	/* Try unsetting persistence flag */
	zassert_equal(conn_mgr_if_set_flag(ifa1, CONN_MGR_IF_PERSISTENT, false), 0,
				"Unsetting persistence flag should succeed for ifa1");

	/* Verify success */
	zassert_false(conn_mgr_if_get_flag(ifa1, CONN_MGR_IF_PERSISTENT),
				"Persistence should be unset for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_binding->flags, 0,
				"Persistence flag unset should affect conn struct");
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

ZTEST_SUITE(conn_mgr_conn, NULL, NULL, conn_mgr_conn_before, NULL, NULL);
