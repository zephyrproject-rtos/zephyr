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

#include <zephyr/net/socket.h>

K_SEM_DEFINE(l4_connected, 0, 1);
K_SEM_DEFINE(l4_disconnected, 0, 1);

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
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

ZTEST(conn_mgr_nsos, test_conn_mgr_nsos_idle)
{
	struct net_if *iface = net_if_get_default();
	struct sockaddr_in v4addr;
	int sock, rc;

	/* 2 second idle timeout */
	conn_mgr_if_set_idle_timeout(iface, 2);

	/* Trigger the connection */
	zassert_equal(0, conn_mgr_if_connect(iface));
	zassert_equal(0, k_sem_take(&l4_connected, K_SECONDS(2)));

	/* Connection should terminate after 2 seconds due to inactivity */
	zassert_equal(-EAGAIN, k_sem_take(&l4_disconnected, K_MSEC(1900)));
	zassert_equal(0, k_sem_take(&l4_disconnected, K_MSEC(500)));

	/* Connect again */
	zassert_equal(0, conn_mgr_if_connect(iface));
	zassert_equal(0, k_sem_take(&l4_connected, K_SECONDS(2)));

	/* Send data after a second (to localhost) */
	rc = zsock_inet_pton(AF_INET, "127.0.0.1", (void *)&v4addr);
	zassert_equal(1, rc);
	v4addr.sin_family = AF_INET;
	v4addr.sin_port = htons(1234);

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	rc = zsock_sendto(sock, "TEST", 4, 0, (const struct sockaddr *)&v4addr, sizeof(v4addr));
	zassert_equal(4, rc);

	/* Should have reset the idle timeout */
	zassert_equal(-EAGAIN, k_sem_take(&l4_disconnected, K_MSEC(1900)));
	zassert_equal(0, k_sem_take(&l4_disconnected, K_MSEC(500)));

	/* Set the interface to persistent */
	conn_mgr_if_set_flag(iface, CONN_MGR_IF_PERSISTENT, true);

	/* Trigger the connection */
	zassert_equal(0, conn_mgr_if_connect(iface));
	zassert_equal(0, k_sem_take(&l4_connected, K_SECONDS(2)));

	/* Interface should disconnect due to idle */
	zassert_equal(0, k_sem_take(&l4_disconnected, K_MSEC(2100)));
	/* But it should also come back up automatically */
	zassert_equal(0, k_sem_take(&l4_connected, K_SECONDS(2)));

	/* Clear the persistent flag, times out and doesn't reconnect */
	conn_mgr_if_set_flag(iface, CONN_MGR_IF_PERSISTENT, false);
	zassert_equal(0, k_sem_take(&l4_disconnected, K_MSEC(2100)));
	zassert_equal(-EAGAIN, k_sem_take(&l4_connected, K_MSEC(2100)));

	/* Cleanup socket */
	zsock_close(sock);
}

static void test_init(void *state)
{
	struct net_if *iface = net_if_get_default();

	k_sem_take(&l4_connected, K_NO_WAIT);
	k_sem_take(&l4_disconnected, K_NO_WAIT);
	conn_mgr_if_set_idle_timeout(iface, CONN_MGR_IF_NO_TIMEOUT);
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
