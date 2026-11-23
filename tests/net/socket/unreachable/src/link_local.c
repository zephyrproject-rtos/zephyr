/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define PEER_PORT 4242

/* Long enough for the address conflict detection of the autoconfigured
 * address to finish.
 */
#define LL_ADDR_WAIT_MS 15000

static const struct net_in_addr remote_addr = { { { 198, 51, 100, 1 } } };

static void make_peer(struct net_sockaddr_in *peer, const struct net_in_addr *addr)
{
	memset(peer, 0, sizeof(*peer));
	peer->sin_family = NET_AF_INET;
	peer->sin_port = net_htons(PEER_PORT);
	peer->sin_addr = *addr;
}

static void *link_local_setup(void)
{
	struct net_if *iface;
	int64_t end;

	if (!IS_ENABLED(CONFIG_NET_IPV4_AUTO)) {
		ztest_test_skip();
		return NULL;
	}

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	zassert_not_null(iface, "No Ethernet interface");

	end = k_uptime_get() + LL_ADDR_WAIT_MS;
	while (net_if_ipv4_get_ll(iface, NET_ADDR_PREFERRED) == NULL) {
		zassert_true(k_uptime_get() < end, "No link-local address was configured");
		k_sleep(K_MSEC(100));
	}

	net_if_set_default(iface);

	return NULL;
}

/* A host with only a link-local address sends to any destination straight on
 * the link (RFC 3927, section 2.6.2), so it has a next hop without a gateway.
 * A socket bound to no specific address must not be refused for having none.
 */
ZTEST(net_socket_unreachable_ll, test_udp_off_link_from_link_local)
{
	struct net_sockaddr_in peer;
	int sock;
	int ret;

	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create UDP socket (%d)", errno);

	make_peer(&peer, &remote_addr);

	ret = zsock_sendto(sock, "x", 1, 0, (struct net_sockaddr *)&peer, sizeof(peer));
	zexpect_equal(ret, 1, "sendto() failed (%d)", errno);

	ret = zsock_connect(sock, (struct net_sockaddr *)&peer, sizeof(peer));
	zexpect_equal(ret, 0, "connect() failed (%d)", errno);

	zsock_close(sock);
}

ZTEST(net_socket_unreachable_ll, test_tcp_off_link_from_link_local)
{
	struct net_sockaddr_in peer;
	int sock;
	int ret;

	sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(sock >= 0, "Cannot create TCP socket (%d)", errno);

	ret = zsock_fcntl(sock, ZVFS_F_SETFL, ZVFS_O_NONBLOCK);
	zassert_equal(ret, 0, "Cannot make the socket non-blocking (%d)", errno);

	make_peer(&peer, &remote_addr);

	ret = zsock_connect(sock, (struct net_sockaddr *)&peer, sizeof(peer));
	zexpect_equal(ret, -1, "connect() to a peer that never answers succeeded");
	zexpect_equal(errno, EINPROGRESS, "connect() failed with %d", errno);

	zsock_close(sock);
}

ZTEST_SUITE(net_socket_unreachable_ll, NULL, link_local_setup, NULL, NULL, NULL);
