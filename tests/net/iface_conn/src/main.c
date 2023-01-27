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

#include <zephyr/net/dummy.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2_connectivity.h>
#include "test_conn_impl.h"

/* This is a duplicate of net_if_get_conn in net_if.c,
 * which is currently not exposed.
 */
static inline struct net_if_conn *net_if_get_conn(struct net_if *iface)
{
	STRUCT_SECTION_FOREACH(net_if_conn, conn) {
		if (iface == conn->iface) {
			return conn;
		}
	}
	return NULL;
}

static inline struct test_conn_data *net_if_get_conn_data(struct net_if *iface)
{
	struct net_if_conn *conn = net_if_get_conn(iface);

	if (!conn) {
		return NULL;
	}
	return conn->ctx;
}

/* Create test ifaces */

/* Generic iface initializer, shared by all three ifaces */
static void test_iface_init(struct net_if *iface)
{
	/* Fake link layer address is needed to silence assertions inside the net core */
	static uint8_t fake_lladdr[] = { 0x01 };
	net_if_set_link_addr(iface, fake_lladdr, sizeof(fake_lladdr), NET_LINK_DUMMY);

	/* Do not automatically start the iface */
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
}

/* Mandatory stub for NET_DEVICE_INIT */
static int test_iface_netdev_init(const struct device *dev)
{
	return 0;
}

static struct dummy_api test_iface_api = {
	.iface_api.init = test_iface_init,
};

/* Create three ifaces, a1, a2, b such that:
 * iface a1 and a2 share L2 connectivity implementation a
 * iface b uses connectivity implementation b
 */
NET_DEVICE_INIT(test_iface_a1,
		"test_iface_a1",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_a2,
		"test_iface_a2",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_b,
		"test_iface_b",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

/* Create an ifaces with NULL implementation, NULL init, and no connectivity at all */
NET_DEVICE_INIT(test_iface_null,
		"test_iface_null",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_ni,
		"test_iface_ni",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_none,
		"test_iface_none",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

/* Bind L2 connectivity implementations to ifaces */
NET_DEVICE_BIND_CONNECTIVITY(test_iface_a1,	TEST_L2_CONN_IMPL_A);
NET_DEVICE_BIND_CONNECTIVITY(test_iface_a2,	TEST_L2_CONN_IMPL_A);
NET_DEVICE_BIND_CONNECTIVITY(test_iface_b,	TEST_L2_CONN_IMPL_B);

/* Bind edge-case L2 connectivity implementations to ifaces */
NET_DEVICE_BIND_CONNECTIVITY(test_iface_null,	TEST_L2_CONN_IMPL_N);
NET_DEVICE_BIND_CONNECTIVITY(test_iface_ni,	TEST_L2_CONN_IMPL_NI);

static void reset_test_iface(struct net_if *iface)
{
	net_if_flag_set(iface, NET_IF_NO_AUTO_CONNECT);

	if (net_if_is_admin_up(iface)) {
		(void)net_if_down(iface);
	}

	/* Some tests can leave the iface in a bad state where it is admin-down but not dormant */
	net_if_dormant_on(iface);

	struct net_if_conn *iface_conn = net_if_get_conn(iface);
	struct test_conn_data *iface_data = net_if_get_conn_data(iface);

	if (iface_conn) {
		iface_conn->persistence = false;
		iface_conn->timeout = 0;
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


static void net_if_conn_before(void *data)
{
	ARG_UNUSED(data);
	reset_test_iface(NET_IF_GET(test_iface_a1,    0));
	reset_test_iface(NET_IF_GET(test_iface_a2,    0));
	reset_test_iface(NET_IF_GET(test_iface_b,     0));
	reset_test_iface(NET_IF_GET(test_iface_ni,    0));
	reset_test_iface(NET_IF_GET(test_iface_none,  0));
	reset_test_iface(NET_IF_GET(test_iface_null,  0));
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
ZTEST(net_if_conn, test_inspect_init)
{
	/* This isn't a proper test in that it only verifies the result of an exterior operation,
	 * but it increases coverage and costs next to nothing to add.
	 */
	struct net_if *ifa1  = NET_IF_GET(test_iface_a1, 0);
	struct net_if *ifa2  = NET_IF_GET(test_iface_a2, 0);
	struct net_if *ifb   = NET_IF_GET(test_iface_b,  0);
	struct net_if *ifni  = NET_IF_GET(test_iface_ni,  0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);
	struct test_conn_data *ifa2_data = net_if_get_conn_data(ifa2);
	struct test_conn_data *ifb_data  = net_if_get_conn_data(ifb);
	struct test_conn_data *ifni_data  = net_if_get_conn_data(ifni);


	zassert_equal(ifa1_data->init_calls_a, 1, "ifa1->init should be called exactly once.");
	zassert_equal(ifa1_data->init_calls_b, 0, "ifa1 should use implementation A");

	zassert_equal(ifa2_data->init_calls_a, 1, "ifa2->init should be called exactly once.");
	zassert_equal(ifa2_data->init_calls_b, 0, "ifa2 should use implementation A");

	zassert_equal(ifb_data->init_calls_b,  1, "ifb->init should be called exactly once.");
	zassert_equal(ifb_data->init_calls_a,  0, "ifb should use implementation B");

	zassert_equal(ifni_data->init_calls_a,  0, "ifni->init should not be called.");
	zassert_equal(ifni_data->init_calls_b,  0, "ifni->init should not be called.");
}

/* Verify that net_if_connect and net_if_disconnect perform the
 * correct API calls to the correct interfaces and connectivity implementations
 */
ZTEST(net_if_conn, test_connect_disconnect)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct net_if *ifa2 = NET_IF_GET(test_iface_a2, 0);
	struct net_if *ifb  = NET_IF_GET(test_iface_b,  0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);
	struct test_conn_data *ifa2_data = net_if_get_conn_data(ifa2);
	struct test_conn_data *ifb_data  = net_if_get_conn_data(ifb);

	/* Take all ifaces up */
	zassert_equal(net_if_up(ifa1), 0,	"net_if_up should not fail");
	zassert_equal(net_if_up(ifa2), 0,	"net_if_up should not fail");
	zassert_equal(net_if_up(ifb), 0,	"net_if_up should not fail");

	/* Verify ifaces are still disconnected */
	zassert_false(net_if_is_up(ifa1),	"Ifaces must be disconnected before test");
	zassert_false(net_if_is_up(ifa2),	"Ifaces must be disconnected before test");
	zassert_false(net_if_is_up(ifb),	"Ifaces must be disconnected before test");

	/* Connect one of the A ifaces */
	zassert_equal(net_if_connect(ifa1), 0,	"net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should be connected after net_if_connect");
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
	zassert_equal(net_if_connect(ifb), 0,	"net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should still be connected");
	zassert_false(net_if_is_up(ifa2),	"ifa2 should not be affected by ifb");
	zassert_true(net_if_is_up(ifb),		"ifb should be connected after net_if_connect");

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
	zassert_equal(net_if_connect(ifa2), 0,	"net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_true(net_if_is_up(ifa1),	"ifa1 should still be connected");
	zassert_true(net_if_is_up(ifa2),	"ifa2 should be connected after net_if_connect");
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
	zassert_equal(net_if_disconnect(ifa1), 0, "net_if_disconnect should not fail");
	k_sleep(K_MSEC(1));


	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be disconnected after net_if_disconnect");
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
	zassert_equal(net_if_disconnect(ifb), 0, "net_if_disconnect should not fail");
	k_sleep(K_MSEC(1));


	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_false(net_if_is_up(ifa1), "ifa1 should still be disconnected");
	zassert_true(net_if_is_up(ifa2),  "ifa2 should not be affected by ifb");
	zassert_false(net_if_is_up(ifb),  "ifb should be disconnected after net_if_disconnect");

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
	zassert_equal(net_if_disconnect(ifa2), 0, "net_if_disconnect should not fail");
	k_sleep(K_MSEC(1));


	/* Verify success, and that only the target iface/conn impl were affected/invoked */
	zassert_false(net_if_is_up(ifa1), "ifa1 should still be disconnected");
	zassert_false(net_if_is_up(ifa2), "ifa2 should be disconnected after net_if_disconnect");
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

/* Verify that double calls to net_if_connect and net_if_disconnect do not cause problems */
ZTEST(net_if_conn, test_connect_disconnect_double_delayed)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail");

	/* Connect iface */
	zassert_equal(net_if_connect(ifa1), 0, "net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_true(net_if_is_up(ifa1), "ifa1 should be connected after net_if_connect");
	zassert_equal(ifa1_data->conn_bal, 1, "ifa1->connect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 1,    "ifa1->connect should have been called once.");

	/* Connect iface again */
	zassert_equal(net_if_connect(ifa1), 0, "net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success
	 * To be clear: Yes, ifa1->connect should be called twice. It is up to the L2
	 * connectivity implementation to either handle idempotence
	 */
	zassert_true(net_if_is_up(ifa1), "ifa1 should still be connected");
	zassert_equal(ifa1_data->conn_bal, 2, "ifa1->connect should have been called again.");
	zassert_equal(ifa1_data->call_cnt_a, 2, "ifa1->connect should have been called again.");

	/* Now disconnect the iface */
	zassert_equal(net_if_disconnect(ifa1), 0, "net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be disconnected after net_if_disconnect");
	zassert_equal(ifa1_data->conn_bal, 1, "ifa1->disconnect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 3, "ifa1->disconnect should have been called once.");

	/* Disconnect again! */
	zassert_equal(net_if_disconnect(ifa1), 0, "net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be disconnected after net_if_disconnect");
	zassert_equal(ifa1_data->conn_bal, 0, "ifa1->disconnect should have been called again.");
	zassert_equal(ifa1_data->call_cnt_a, 4, "ifa1->disconnect should have been called again.");
}

/* Verify that fast double calls to net_if_connect and net_if_disconnect do not cause problems */
ZTEST(net_if_conn, test_connect_disconnect_double_instant)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail");

	/* Connect twice */
	zassert_equal(net_if_connect(ifa1), 0, "net_if_connect should not fail");
	zassert_equal(net_if_connect(ifa1), 0, "net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_true(net_if_is_up(ifa1), "ifa1 should be connected after net_if_connect");
	zassert_equal(ifa1_data->conn_bal, 2, "ifa1->connect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 2,    "ifa1->connect should have been called once.");

	/* Now disconnect twice */
	zassert_equal(net_if_disconnect(ifa1), 0, "net_if_connect should not fail");
	zassert_equal(net_if_disconnect(ifa1), 0, "net_if_connect should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_false(net_if_is_up(ifa1), "ifa1 should be disconnected after net_if_disconnect");
	zassert_equal(ifa1_data->conn_bal, 0, "ifa1->disconnect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 4, "ifa1->disconnect should have been called once.");
}

/* Verify that connecting an iface that isn't up, missing an API,
 * or isn't connectivity-bound raises an error.
 */
ZTEST(net_if_conn, test_connect_invalid)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	struct net_if *ifnull = NET_IF_GET(test_iface_null, 0);
	struct test_conn_data *ifnull_data = net_if_get_conn_data(ifnull);

	struct net_if *ifnone = NET_IF_GET(test_iface_none, 0);

	/* Bring ifnull and ifnone up */
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for ifnull");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for ifnone");

	/* Attempts to connect ifa1 without bringing it up should fail */
	zassert_equal(net_if_connect(ifa1), -ESHUTDOWN,
					"net_if_connect should give -ENOTSUP for down iface");
	zassert_equal(ifa1_data->conn_bal, 0,
					"net_if_connect should not affect down iface");
	zassert_equal(ifa1_data->call_cnt_a, 0,
					"net_if_connect should not affect down iface");

	/* Attempts to connect ifnull should fail, even if it is up */
	zassert_equal(net_if_connect(ifnull), -ENOTSUP,
					"net_if_connect should give -ENOTSUP for ifnull");
	zassert_equal(ifnull_data->conn_bal, 0,
					"net_if_connect should not affect ifnull");
	zassert_equal(ifnull_data->call_cnt_a, 0,
					"net_if_connect should not affect ifnull");

	/* Attempts to connect ifnone should fail, even if it is up */
	zassert_equal(net_if_connect(ifnone), -ENOTSUP,
					"net_if_connect should give -ENOTSUP for ifnone");
}



/* Verify that disconnecting an iface that isn't up, missing an API,
 * or isn't connectivity-bound raises an error.
 */
ZTEST(net_if_conn, test_disconnect_invalid)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	struct net_if *ifnull = NET_IF_GET(test_iface_null, 0);
	struct test_conn_data *ifnull_data = net_if_get_conn_data(ifnull);

	struct net_if *ifnone = NET_IF_GET(test_iface_none, 0);

	/* Bring ifnull and ifnone up */
	zassert_equal(net_if_up(ifnull), 0, "net_if_up should succeed for ifnull");
	zassert_equal(net_if_up(ifnone), 0, "net_if_up should succeed for ifnone");

	/* Attempts to disconnect ifa1 without bringing it up should fail */
	zassert_equal(net_if_disconnect(ifa1), -EALREADY,
					"net_if_disconnect should give -ENOTSUP for down iface");
	zassert_equal(ifa1_data->conn_bal, 0,
					"net_if_disconnect should not affect down iface");
	zassert_equal(ifa1_data->call_cnt_a, 0,
					"net_if_disconnect should not affect down iface");

	/* Attempts to disconnect ifnull should fail, even if it is up */
	zassert_equal(net_if_disconnect(ifnull), -ENOTSUP,
					"net_if_disconnect should give -ENOTSUP for ifnull");
	zassert_equal(ifnull_data->conn_bal, 0,
					"net_if_disconnect should not affect ifnull");
	zassert_equal(ifnull_data->call_cnt_a, 0,
					"net_if_disconnect should not affect ifnull");

	/* Attempts to disconnect ifnone should fail, even if it is up */
	zassert_equal(net_if_disconnect(ifnone), -ENOTSUP,
					"net_if_disconnect should give -ENOTSUP for ifnone");
}

/* Verify that net_if_connect forwards error codes from API */
ZTEST(net_if_conn, test_connect_fail)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	/* Instruct ifa1 to fail on connect attempt */
	ifa1_data->api_err = -ECHILD;

	/* Take ifa1 up before attempting to connect */
	zassert_equal(net_if_up(ifa1), 0, "net_if_connect should succeed");

	/* Attempts to connect ifa1 should return the expected error*/
	zassert_equal(net_if_connect(ifa1), -ECHILD, "net_if_connect should give -ECHILD");
}

/* Verify that net_if_disconnect forwards error codes from API */
ZTEST(net_if_conn, test_disconnect_fail)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	/* Take up and connect iface first */
	zassert_equal(net_if_up(ifa1), 0,	"net_if_up should succeed");
	zassert_equal(net_if_connect(ifa1), 0,	"net_if_connect should succeed");

	/* Instruct ifa1 to fail on disconnect attempt */
	ifa1_data->api_err = -EDOM;

	/* Attempts to disconnect ifa1 should return the expected error*/
	zassert_equal(net_if_disconnect(ifa1), -EDOM, "net_if_disconnect should give -EDOM");
}

/* Verify that net_if_up automatically triggers net_if_connect
 * (if and only if auto-connect is enabled)
 */
ZTEST(net_if_conn, test_up_auto_connect)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	/* Take iface up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail");
	k_sleep(K_MSEC(1));

	/* Verify that this had no effect on connectivity. */
	zassert_false(net_if_is_up(ifa1),
		"net_if_up should not affect connectivity if NET_IF_NO_AUTO_CONNECT is set");
	zassert_equal(ifa1_data->conn_bal, 0,
		"net_if_up should not affect connectivity if NET_IF_NO_AUTO_CONNECT is set");
	zassert_equal(ifa1_data->call_cnt_a, 0,
		"net_if_up should not affect connectivity if NET_IF_NO_AUTO_CONNECT is set");

	/* Take iface down */
	zassert_equal(net_if_down(ifa1), 0, "net_if_down should not fail");

	/* Re-enable autoconnect */
	net_if_flag_clear(ifa1, NET_IF_NO_AUTO_CONNECT);

	/* Take iface back up */
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail");
	k_sleep(K_MSEC(1));

	/* Verify success */
	zassert_true(net_if_is_up(ifa1),
		"net_if_up should call net_if_connect if NET_IF_NO_AUTO_CONNECT is unset");
	zassert_equal(ifa1_data->conn_bal, 1,	"ifa1->connect should have been called once.");
	zassert_equal(ifa1_data->call_cnt_a, 1, "ifa1->connect should have been called once.");
}

/* Verify that auto-connect failure behaves as expected */
ZTEST(net_if_conn, test_up_auto_connect_fail)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct test_conn_data *ifa1_data = net_if_get_conn_data(ifa1);

	/* Enable autoconnect */
	net_if_flag_clear(ifa1, NET_IF_NO_AUTO_CONNECT);

	/* Schedule an error */
	ifa1_data->api_err = -EAFNOSUPPORT;

	/* Verify that error is not forwarded to net_if_up*/
	zassert_equal(net_if_up(ifa1), 0, "net_if_up should not fail");
	k_sleep(K_MSEC(1));

	/* Verify that iface is admin_up despite error */
	zassert_true(net_if_is_admin_up(ifa1));
}

/* Verify that net_if_supports_connectivity gives correct results */
ZTEST(net_if_conn, test_supports_connectivity)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct net_if *ifa2 = NET_IF_GET(test_iface_a2, 0);
	struct net_if *ifb  = NET_IF_GET(test_iface_b,  0);
	struct net_if *ifnull = NET_IF_GET(test_iface_null, 0);
	struct net_if *ifnone = NET_IF_GET(test_iface_none, 0);

	zassert_true(net_if_supports_connectivity(ifa1));
	zassert_true(net_if_supports_connectivity(ifa2));
	zassert_true(net_if_supports_connectivity(ifb));
	zassert_false(net_if_supports_connectivity(ifnull));
	zassert_false(net_if_supports_connectivity(ifnone));
}

/* 60 characters long */
#define TEST_STR_LONG	"AAAAAaaaaaBBBBBbbbbbCCCCCcccccDDDDDdddddEEEEEeeeeeFFFFFfffff"

/* Verify that conn_opt get/set functions operate correctly and affect only the target iface */
ZTEST(net_if_conn, test_conn_opt)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct net_if *ifa2 = NET_IF_GET(test_iface_a2, 0);

	char buf[100];
	size_t buf_len = 0;

	/* Set ifa1->X to "A" */
	strcpy(buf, "A");
	zassert_equal(net_if_set_conn_opt(ifa1, TEST_CONN_OPT_X, &buf, strlen(buf) + 1), 0,
		       "net_if_set_conn_opt should succeed for valid parameters");

	/* Verify success */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len),
		       0, "net_if_get_conn_opt should succeed for valid parameters");
	printk("%d, %d", buf_len, strlen(buf) + 1);
	zassert_equal(buf_len, strlen(buf) + 1, "net_if_get_conn_opt should return valid optlen");
	zassert_equal(strcmp(buf, "A"), 0, "net_if_get_conn_opt should retrieve \"A\"");

	/* Verify that ifa1->Y was not affected */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_Y, &buf, &buf_len),
		       0, "net_if_get_conn_opt should succeed for valid parameters");
	zassert_equal(buf_len, 1, "net_if_get_conn_opt should yield nothing for ifa1->Y");
	zassert_equal(buf[0], 0, "net_if_get_conn_opt should yield nothing for ifa1->Y");

	/* Verify that ifa2->X was not affected */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(net_if_get_conn_opt(ifa2, TEST_CONN_OPT_X, &buf, &buf_len),
		       0, "net_if_get_conn_opt should succeed for valid parameters");
	zassert_equal(buf_len, 1, "net_if_get_conn_opt should yield nothing for ifa2->X");
	zassert_equal(buf[0], 0, "net_if_get_conn_opt should yield nothing for ifa2->X");

	/* Now, set ifa->Y to "ABC" */
	strcpy(buf, "ABC");
	zassert_equal(net_if_set_conn_opt(ifa1, TEST_CONN_OPT_Y, &buf, strlen(buf) + 1), 0,
		       "net_if_set_conn_opt should succeed for valid parameters");

	/* Verify success */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_Y, &buf, &buf_len),
		       0, "net_if_get_conn_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1, "net_if_get_conn_opt should return valid optlen");
	zassert_equal(strcmp(buf, "ABC"), 0, "net_if_get_conn_opt should retrieve \"ABC\"");

	/* Verify that ifa1->X was not affected */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len),
		       0, "net_if_get_conn_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1, "net_if_get_conn_opt should return valid optlen");
	zassert_equal(strcmp(buf, "A"), 0, "net_if_get_conn_opt should retrieve \"A\"");

	/* Next, we pass some buffers that are too large or too small.
	 * This is an indirect way of verifying that buf_len is passed correctly.
	 */

	/* Try writing a string that is too large to ifa1->X */
	strcpy(buf, TEST_STR_LONG);
	zassert_equal(net_if_set_conn_opt(ifa1, TEST_CONN_OPT_X, &buf, strlen(buf) + 1), 0,
		       "net_if_set_conn_opt should succeed for valid parameters");

	/* Verify partial success */
	memset(buf, 0, sizeof(buf));
	buf_len = sizeof(buf);
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len), 0,
				"net_if_get_conn_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1,
				"net_if_get_conn_opt should return valid optlen");

	/* This does, technically, test the test harness, but this test will fail if
	 * the unit under test (net_if_set_conn_opt) fails to pass along the optlen
	 */
	zassert_true(strlen(buf) < strlen(TEST_STR_LONG),
				"test_set_opt_a should truncate long values");

	/* For the same reason, verify that get_opt truncates given a small destination buffer */
	memset(buf, 0, sizeof(buf));
	buf_len = 10;
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_X, &buf, &buf_len), 0,
				"net_if_get_conn_opt should succeed for valid parameters");
	zassert_equal(buf_len, strlen(buf) + 1,
				"net_if_get_conn_opt should return valid optlen");
	zassert_equal(buf_len, 10,
				"test_get_opt_a should truncate if dest. buffer is too small.");
}

/* Verify that net_if_get_conn_opt and net_if_set_conn_opt behave as expected when given invalid
 * arguments.
 */
ZTEST(net_if_conn, test_conn_opt_invalid)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct net_if *ifb  = NET_IF_GET(test_iface_b,  0);
	struct net_if *ifnull = NET_IF_GET(test_iface_null, 0);
	struct net_if *ifnone = NET_IF_GET(test_iface_none, 0);
	char buf[100];
	size_t buf_len;

	/* Verify that getting/setting non-existent option on ifa1 fails */
	zassert_equal(net_if_set_conn_opt(ifa1, -1, "A", strlen("A")), -ENOPROTOOPT,
				"net_if_set_conn_opt should fail with invalid optname");
	buf_len = sizeof(buf_len);
	zassert_equal(net_if_get_conn_opt(ifa1, -1, buf, &buf_len), -ENOPROTOOPT,
				"net_if_get_conn_opt should fail with invalid optname");
	zassert_equal(buf_len, 0, "failed net_if_get_conn_opt should always set buf_len to zero.");

	/* Verify that getting/setting with NULL buffer on ifa1 fails */
	zassert_equal(net_if_set_conn_opt(ifa1, TEST_CONN_OPT_X, NULL, 100), -EINVAL,
				"net_if_set_conn_opt should fail with invalid buffer");
	buf_len = sizeof(buf_len);
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_X, NULL, &buf_len), -EINVAL,
				"net_if_get_conn_opt should fail with invalid buffer");
	zassert_equal(buf_len, 0, "failed net_if_get_conn_opt should always set buf_len to zero.");

	/* Verify that getting with NULL buffer length on ifa1 fails */
	zassert_equal(net_if_get_conn_opt(ifa1, TEST_CONN_OPT_X, buf, NULL), -EINVAL,
				"net_if_get_conn_opt should fail with invalid buffer length");

	/* Verify that getting/setting with ifnull fails */
	zassert_equal(net_if_set_conn_opt(ifnull, TEST_CONN_OPT_X, "A", strlen("A")), -ENOTSUP,
				"net_if_set_conn_opt should fail for ifnull");
	buf_len = sizeof(buf_len);
	zassert_equal(net_if_get_conn_opt(ifnull, TEST_CONN_OPT_X, buf, &buf_len), -ENOTSUP,
				"net_if_get_conn_opt should fail for ifnull");
	zassert_equal(buf_len, 0, "failed net_if_get_conn_opt should always set buf_len to zero.");

	/* Verify that getting/setting with ifnone fails */
	zassert_equal(net_if_set_conn_opt(ifnone, TEST_CONN_OPT_X, "A", strlen("A")), -ENOTSUP,
				"net_if_set_conn_opt should fail for ifnull");
	buf_len = sizeof(buf_len);
	zassert_equal(net_if_get_conn_opt(ifnone, TEST_CONN_OPT_X, buf, &buf_len), -ENOTSUP,
				"net_if_get_conn_opt should fail for ifnull");
	zassert_equal(buf_len, 0, "failed net_if_get_conn_opt should always set buf_len to zero.");

	/* Verify that getting/setting with ifb fails (since implementation B doesn't support it) */
	zassert_equal(net_if_set_conn_opt(ifb, TEST_CONN_OPT_X, "A", strlen("A")), -ENOTSUP,
				"net_if_set_conn_opt should fail for ifb");
	buf_len = sizeof(buf_len);
	zassert_equal(net_if_get_conn_opt(ifb, TEST_CONN_OPT_X, buf, &buf_len), -ENOTSUP,
				"net_if_get_conn_opt should fail for ifb");
	zassert_equal(buf_len, 0, "failed net_if_get_conn_opt should always set buf_len to zero.");
}

/* Verify that persistence get/set functions operate correctly */
ZTEST(net_if_conn, test_persistence)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct net_if_conn *ifa1_conn = net_if_get_conn(ifa1);

	/* Try setting persistence */
	zassert_equal(net_if_set_conn_persistence(ifa1, true), 0,
				"Setting persistence should succeed for ifa1");

	/* Verify success */
	zassert_true(net_if_get_conn_persistence(ifa1),
				"Persistence should be set for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_true(ifa1_conn->persistence,
				"Persistence set should affect conn struct");

	/* Try unsetting persistence */
	zassert_equal(net_if_set_conn_persistence(ifa1, false), 0,
				"Unsetting persistence should succeed for ifa1");

	/* Verify success */
	zassert_false(net_if_get_conn_persistence(ifa1),
				"Persistence should be unset for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_false(ifa1_conn->persistence,
				"Persistence unset should affect conn struct");
}

/* Verify that persistence get/set fail and behave as expected respectively for invalid ifaces */
ZTEST(net_if_conn, test_persistence_invalid)
{
	struct net_if *ifnull = NET_IF_GET(test_iface_null, 0);
	struct net_if *ifnone = NET_IF_GET(test_iface_none, 0);

	/* Verify set failure */
	zassert_equal(net_if_set_conn_persistence(ifnull, true), -ENOTSUP,
		"Setting persistence should fail for ifnull");
	zassert_equal(net_if_set_conn_persistence(ifnone, true), -ENOTSUP,
		"Setting persistence should fail for ifnone");

	/* Verify get graceful behavior */
	zassert_false(net_if_get_conn_persistence(ifnull),
		"Getting persistence should yield false for ifnull");
	zassert_false(net_if_get_conn_persistence(ifnone),
		"Getting persistence should yield false for ifnone");

}


/* Verify that timeout get/set functions operate correctly (A/B) */
ZTEST(net_if_conn, test_timeout)
{
	struct net_if *ifa1 = NET_IF_GET(test_iface_a1, 0);
	struct net_if_conn *ifa1_conn = net_if_get_conn(ifa1);

	/* Try setting timeout */
	zassert_equal(net_if_set_conn_timeout(ifa1, 99), 0,
				"Setting timeout should succeed for ifa1");

	/* Verify success */
	zassert_equal(net_if_get_conn_timeout(ifa1), 99,
				"Timeout should be set to 99 for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_conn->timeout, 99,
				"Timeout set should affect conn struct");

	/* Try unsetting timeout */
	zassert_equal(net_if_set_conn_timeout(ifa1, false), 0,
				"Unsetting timeout should succeed for ifa1");

	/* Verify success */
	zassert_equal(net_if_get_conn_timeout(ifa1), 0,
				"Timeout should be unset for ifa1");

	/* Verify that the conn struct agrees, since this is what implementations may use */
	zassert_equal(ifa1_conn->timeout, 0,
				"Timeout unset should affect conn struct");
}

/* Verify that timeout get/set fail and behave as expected respectively for invalid ifaces */
ZTEST(net_if_conn, test_timeout_invalid)
{
	struct net_if *ifnull = NET_IF_GET(test_iface_null, 0);
	struct net_if *ifnone = NET_IF_GET(test_iface_none, 0);

	/* Verify set failure */
	zassert_equal(net_if_set_conn_timeout(ifnull, true), -ENOTSUP,
		"Setting timeout should fail for ifnull");
	zassert_equal(net_if_set_conn_timeout(ifnone, true), -ENOTSUP,
		"Setting timeout should fail for ifnone");

	/* Verify get graceful behavior */
	zassert_equal(net_if_get_conn_timeout(ifnull), 0,
		"Getting timeout should yield zero for ifnull");
	zassert_equal(net_if_get_conn_timeout(ifnone), 0,
		"Getting timeout should yield zero for ifnone");
}

ZTEST_SUITE(net_if_conn, NULL, NULL, net_if_conn_before, NULL, NULL);
