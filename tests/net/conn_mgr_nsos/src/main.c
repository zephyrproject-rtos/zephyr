/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>

K_SEM_DEFINE(l4_connected, 0, 1);
K_SEM_DEFINE(l4_disconnected, 0, 1);

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		k_sem_give(&l4_connected);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		k_sem_give(&l4_disconnected);
		break;
	default:
		break;
	}
}

ZTEST(conn_mgr_nsos, test_conn_mgr_nsos_opt)
{
	struct net_if *iface = net_if_get_default();
	k_timeout_t conn_delay;
	size_t optlen;

	/* Default delay is 1 second */
	optlen = sizeof(conn_delay);
	zassert_equal(0, conn_mgr_if_get_opt(iface, 0, &conn_delay, &optlen));
	zassert_equal(sizeof(conn_delay), optlen);
	zassert_true(K_TIMEOUT_EQ(K_SECONDS(1), conn_delay));

	/* Delay can be updated */
	conn_delay = K_SECONDS(2);
	zassert_equal(0, conn_mgr_if_set_opt(iface, 0, &conn_delay, sizeof(conn_delay)));
	optlen = sizeof(conn_delay);
	zassert_equal(0, conn_mgr_if_get_opt(iface, 0, &conn_delay, &optlen));
	zassert_equal(sizeof(conn_delay), optlen);
	zassert_true(K_TIMEOUT_EQ(K_SECONDS(2), conn_delay));

	/* Reset to 1 second */
	conn_delay = K_SECONDS(1);
	zassert_equal(0, conn_mgr_if_set_opt(iface, 0, &conn_delay, sizeof(conn_delay)));
}

ZTEST(conn_mgr_nsos, test_conn_mgr_nsos_opt_error)
{
	struct net_if *iface = net_if_get_default();
	k_timeout_t conn_delay = K_SECONDS(1);
	size_t optlen;

	/* Bad option */
	optlen = sizeof(conn_delay);
	zassert_equal(-EINVAL, conn_mgr_if_get_opt(iface, 1, &conn_delay, &optlen));
	zassert_equal(-EINVAL, conn_mgr_if_set_opt(iface, 1, &conn_delay, sizeof(conn_delay)));

	/* Bad output size */
	optlen = sizeof(conn_delay) - 1;
	zassert_equal(-EINVAL, conn_mgr_if_get_opt(iface, 0, &conn_delay, &optlen));
	zassert_equal(-EINVAL, conn_mgr_if_set_opt(iface, 0, &conn_delay, sizeof(conn_delay) - 1));
}

ZTEST(conn_mgr_nsos, test_conn_mgr_nsos)
{
	struct net_if *iface = net_if_get_default();
	k_timeout_t conn_delay, conn_delay_default;
	size_t optlen = sizeof(conn_delay_default);

	/* Store default delay */
	zassert_equal(0, conn_mgr_if_get_opt(iface, 0, &conn_delay_default, &optlen));

	/* Not connecting by default */
	zassert_equal(-EAGAIN, k_sem_take(&l4_connected, K_SECONDS(2)));

	/* Trigger the connection */
	zassert_equal(0, conn_mgr_if_connect(iface));

	/* Default time to connection is 1 second */
	zassert_equal(-EAGAIN, k_sem_take(&l4_connected, K_MSEC(950)));
	zassert_equal(0, k_sem_take(&l4_connected, K_MSEC(100)));
	zassert_true(net_if_is_up(iface));

	/* Small sleep to allow for network stack to return to idle */
	k_sleep(K_MSEC(500));

	/* Disconnect (actioned immediately) */
	zassert_equal(0, conn_mgr_if_disconnect(iface));
	zassert_equal(0, k_sem_take(&l4_disconnected, K_MSEC(100)));
	zassert_false(net_if_is_up(iface));

	/* Try again with custom timeout */
	conn_delay = K_MSEC(500);
	zassert_equal(0, conn_mgr_if_set_opt(iface, 0, &conn_delay, sizeof(conn_delay)));

	/* Trigger the connection */
	zassert_equal(0, conn_mgr_if_connect(iface));

	/* Should connect after 500ms this time */
	zassert_equal(-EAGAIN, k_sem_take(&l4_connected, K_MSEC(450)));
	zassert_equal(0, k_sem_take(&l4_connected, K_MSEC(100)));
	zassert_true(net_if_is_up(iface));

	/* Small sleep to allow for network stack to return to idle */
	k_sleep(K_MSEC(500));

	/* Disconnect (actioned immediately) */
	zassert_equal(0, conn_mgr_if_disconnect(iface));
	zassert_equal(0, k_sem_take(&l4_disconnected, K_MSEC(100)));
	zassert_false(net_if_is_up(iface));

	/* Reset to default value */
	zassert_equal(
		0, conn_mgr_if_set_opt(iface, 0, &conn_delay_default, sizeof(conn_delay_default)));
}

static void test_init(void *state)
{
	k_sem_take(&l4_connected, K_NO_WAIT);
	k_sem_take(&l4_disconnected, K_NO_WAIT);
}

static void test_after(void *fixture)
{
	conn_mgr_all_if_disconnect(false);
	conn_mgr_all_if_down(false);
}

static void *testsuite_init(void)
{
	static struct net_mgmt_event_callback l4_callback;
	struct net_if *iface = net_if_get_default();
	struct in_addr addr;

	net_mgmt_init_event_callback(&l4_callback, l4_event_handler,
				     NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&l4_callback);

	conn_mgr_all_if_down(false);

	/* Add an IP address so that NET_EVENT_L4_CONNECTED can trigger */
	net_addr_pton(AF_INET, "192.0.2.1", &addr);
	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	return NULL;
}

ZTEST_SUITE(conn_mgr_nsos, NULL, testsuite_init, test_init, test_after, NULL);
