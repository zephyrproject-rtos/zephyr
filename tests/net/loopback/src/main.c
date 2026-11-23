/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/socket.h>

#define TEST_PORT 4242

/* Larger than CONFIG_NET_LOOPBACK_MTU, so it needs two fragments */
#define TEST_PAYLOAD_LEN 2000

static uint8_t tx_buf[TEST_PAYLOAD_LEN];
static uint8_t rx_buf[TEST_PAYLOAD_LEN + 1];

static uint32_t ip_sent(net_sa_family_t family)
{
	struct net_stats_ip stats = { 0 };
	int ret;

	if (family == NET_AF_INET) {
		ret = net_mgmt(NET_REQUEST_STATS_GET_IPV4, NULL, &stats, sizeof(stats));
	} else {
		ret = net_mgmt(NET_REQUEST_STATS_GET_IPV6, NULL, &stats, sizeof(stats));
	}

	zassert_equal(ret, 0, "Cannot read the statistics (%d)", ret);

	return stats.sent;
}

static void send_oversized_datagram(net_sa_family_t family)
{
	struct net_sockaddr_storage addr = { 0 };
	net_socklen_t addrlen;
	struct zsock_timeval tv = {
		.tv_sec = 1,
	};
	uint32_t sent_before;
	uint32_t sent;
	ssize_t len;
	int rx_sock;
	int tx_sock;
	int ret;

	if (family == NET_AF_INET) {
		struct net_sockaddr_in *addr4 = (struct net_sockaddr_in *)&addr;

		addr4->sin_family = NET_AF_INET;
		addr4->sin_port = net_htons(TEST_PORT);
		addr4->sin_addr = (struct net_in_addr)NET_INADDR_LOOPBACK_INIT;
		addrlen = sizeof(*addr4);
	} else {
		struct net_sockaddr_in6 *addr6 = (struct net_sockaddr_in6 *)&addr;

		addr6->sin6_family = NET_AF_INET6;
		addr6->sin6_port = net_htons(TEST_PORT);
		addr6->sin6_addr = net_in6addr_loopback;
		addrlen = sizeof(*addr6);
	}

	for (size_t i = 0; i < sizeof(tx_buf); i++) {
		tx_buf[i] = (uint8_t)i;
	}

	/* A datagram whose source and destination are the same address and
	 * port is dropped on reception, so send from another socket.
	 */
	rx_sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(rx_sock >= 0, "Cannot create a socket (%d)", errno);

	tx_sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(tx_sock >= 0, "Cannot create a socket (%d)", errno);

	ret = zsock_setsockopt(rx_sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv, sizeof(tv));
	zassert_equal(ret, 0, "Cannot set the receive timeout (%d)", errno);

	ret = zsock_bind(rx_sock, (struct net_sockaddr *)&addr, addrlen);
	zassert_equal(ret, 0, "Cannot bind (%d)", errno);

	sent_before = ip_sent(family);

	len = zsock_sendto(tx_sock, tx_buf, sizeof(tx_buf), 0, (struct net_sockaddr *)&addr,
			   addrlen);
	zexpect_equal(len, sizeof(tx_buf), "Send failed (%zd, %d)", len, errno);

	len = zsock_recv(rx_sock, rx_buf, sizeof(rx_buf), 0);
	zexpect_equal(len, sizeof(tx_buf), "Receive failed (%zd, %d)", len, errno);
	zexpect_mem_equal(rx_buf, tx_buf, sizeof(tx_buf), "Received data differs");

	/* The datagram itself is counted when it is sent, and each fragment
	 * once more when the fragment is sent.
	 */
	sent = ip_sent(family) - sent_before;

	if (IS_ENABLED(CONFIG_NET_LOOPBACK_FRAGMENT)) {
		zexpect_equal(sent, 3, "Datagram was not sent as two fragments (%u)", sent);
	} else {
		zexpect_equal(sent, 1, "Datagram was fragmented (%u)", sent);
	}

	(void)zsock_close(tx_sock);
	(void)zsock_close(rx_sock);
}

/* Without fragmentation support a datagram larger than the MTU cannot be
 * sent at all.
 */
ZTEST(net_loopback, test_ipv4_oversized_datagram)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_FRAGMENT);

	send_oversized_datagram(NET_AF_INET);
}

ZTEST(net_loopback, test_ipv6_oversized_datagram)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_FRAGMENT);

	send_oversized_datagram(NET_AF_INET6);
}

ZTEST_SUITE(net_loopback, NULL, NULL, NULL, NULL, NULL);
