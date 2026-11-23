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

#include "route_ipv4.h"

#define PEER_PORT 4242

static const struct net_in_addr my_addr = { { { 192, 0, 2, 2 } } };
/* On the link under a /24 netmask, but not under a /32 one */
static const struct net_in_addr neighbor_addr = { { { 192, 0, 2, 1 } } };
static const struct net_in_addr remote_addr = { { { 198, 51, 100, 1 } } };
static const struct net_in_addr mcast_addr = { { { 224, 0, 0, 1 } } };
static const struct net_in_addr loopback_addr = { { { 127, 0, 0, 1 } } };
static const struct net_in_addr netmask_24 = { { { 255, 255, 255, 0 } } };
static const struct net_in_addr netmask_32 = { { { 255, 255, 255, 255 } } };

static struct net_if *eth_iface;
static atomic_t frames_sent;

static void eth_fake_iface_init(struct net_if *iface)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	static uint8_t mac_addr[6] = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 };

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	atomic_inc(&frames_sent);

	return 0;
}

static enum ethernet_hw_caps eth_fake_get_capabilities(const struct device *dev,
							struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return IS_ENABLED(CONFIG_NET_VLAN) ? ETHERNET_HW_VLAN : 0;
}

static DEVICE_API(ethernet, eth_fake_api_funcs) = {
	.iface_api.init = eth_fake_iface_init,
	.get_capabilities = eth_fake_get_capabilities,
	.send = eth_fake_send,
};

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", NULL, NULL, NULL, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs, NET_ETH_MTU);

static void set_netmask(const struct net_in_addr *netmask)
{
	zassert_true(net_if_ipv4_set_netmask_by_addr(eth_iface, &my_addr, netmask),
		     "Cannot set netmask");
}

static void set_gw(const struct net_in_addr *gw)
{
	net_if_ipv4_set_gw(eth_iface, gw != NULL ? gw : net_ipv4_unspecified_address());
}

static void make_peer(struct net_sockaddr_in *peer, const struct net_in_addr *addr)
{
	memset(peer, 0, sizeof(*peer));
	peer->sin_family = NET_AF_INET;
	peer->sin_port = net_htons(PEER_PORT);
	peer->sin_addr = *addr;
}

static int udp_socket(void)
{
	int sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(sock >= 0, "Cannot create UDP socket (%d)", errno);

	return sock;
}

static int udp_sendto(int sock, const struct net_in_addr *addr)
{
	struct net_sockaddr_in peer;
	int ret;

	make_peer(&peer, addr);

	ret = zsock_sendto(sock, "x", 1, 0, (struct net_sockaddr *)&peer, sizeof(peer));

	return ret < 0 ? -errno : ret;
}

static void *unreachable_setup(void)
{
	/* The link-local suite runs on its own in the autoconfiguration
	 * scenario, where the address added here would take part in conflict
	 * detection and its frames would be counted.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4_AUTO)) {
		ztest_test_skip();
		return NULL;
	}

	eth_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	zassert_not_null(eth_iface, "No Ethernet interface");

	zassert_not_null(net_if_ipv4_addr_add(eth_iface, &my_addr, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv4 address");
	net_if_set_default(eth_iface);

	return NULL;
}

static void unreachable_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* An address leased with a host netmask and no router */
	set_netmask(&netmask_32);
	set_gw(NULL);

	/* Let what an earlier test queued go out before counting */
	k_sleep(K_MSEC(10));
	atomic_set(&frames_sent, 0);
}

/* With no address on the link and no gateway, sendto() reports that there is
 * no way to the destination, and nothing is put on the wire, not even an ARP
 * request.
 */
ZTEST(net_socket_unreachable, test_udp_sendto_without_next_hop)
{
	int sock = udp_socket();

	zexpect_equal(udp_sendto(sock, &remote_addr), -ENETUNREACH,
		      "Off-link destination without a gateway");
	zexpect_equal(udp_sendto(sock, &neighbor_addr), -ENETUNREACH,
		      "Destination off-link under a /32 netmask");

	k_sleep(K_MSEC(10));
	zexpect_equal(atomic_get(&frames_sent), 0, "%ld frames were sent",
		      (long)atomic_get(&frames_sent));

	zsock_close(sock);
}

/* What does have a next hop is still sent. */
ZTEST(net_socket_unreachable, test_udp_sendto_with_next_hop)
{
	int sock = udp_socket();

	/* Multicast and the host itself need no next hop */
	zexpect_equal(udp_sendto(sock, &mcast_addr), 1, "Multicast destination");
	zexpect_equal(udp_sendto(sock, &my_addr), 1, "Own address");

	/* Turned around in the stack when the addresses are checked */
	if (IS_ENABLED(CONFIG_NET_IP_ADDR_CHECK)) {
		zexpect_equal(udp_sendto(sock, &loopback_addr), 1, "Loopback destination");
	}

	/* On-link destination */
	set_netmask(&netmask_24);
	zexpect_equal(udp_sendto(sock, &neighbor_addr), 1, "On-link destination");

	/* Off-link destination through the gateway */
	set_gw(&neighbor_addr);
	zexpect_equal(udp_sendto(sock, &remote_addr), 1, "Off-link destination via gateway");

	k_sleep(K_MSEC(10));
	zexpect_true(atomic_get(&frames_sent) > 0, "Nothing was sent");

	zsock_close(sock);
}

/* connect() fails at once instead of waiting out the SYN retransmissions,
 * so that an application can move on to its next address.
 */
ZTEST(net_socket_unreachable, test_tcp_connect_without_next_hop)
{
	struct net_sockaddr_in peer;
	int64_t start;
	int64_t elapsed;
	int sock;
	int ret;

	sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(sock >= 0, "Cannot create TCP socket (%d)", errno);

	make_peer(&peer, &remote_addr);

	start = k_uptime_get();
	ret = zsock_connect(sock, (struct net_sockaddr *)&peer, sizeof(peer));
	elapsed = k_uptime_get() - start;

	zexpect_equal(ret, -1, "connect() succeeded");
	zexpect_equal(errno, ENETUNREACH, "connect() failed with %d", errno);
	zexpect_true(elapsed < CONFIG_NET_SOCKETS_CONNECT_TIMEOUT,
		     "connect() took %lld ms", elapsed);

	k_sleep(K_MSEC(10));
	zexpect_equal(atomic_get(&frames_sent), 0, "%ld frames were sent",
		      (long)atomic_get(&frames_sent));

	zsock_close(sock);
}

/* A UDP connect() to a destination without a next hop fails and leaves the
 * peer the socket was connected to before.
 */
ZTEST(net_socket_unreachable, test_udp_connect_without_next_hop_keeps_peer)
{
	struct net_sockaddr_in peer;
	net_socklen_t len = sizeof(peer);
	int sock = udp_socket();
	int ret;

	set_netmask(&netmask_24);

	make_peer(&peer, &neighbor_addr);
	ret = zsock_connect(sock, (struct net_sockaddr *)&peer, sizeof(peer));
	zassert_equal(ret, 0, "connect() to the on-link peer failed (%d)", errno);

	make_peer(&peer, &remote_addr);
	ret = zsock_connect(sock, (struct net_sockaddr *)&peer, sizeof(peer));
	zexpect_equal(ret, -1, "connect() without a next hop succeeded");
	zexpect_equal(errno, ENETUNREACH, "connect() failed with %d", errno);

	memset(&peer, 0, sizeof(peer));
	ret = zsock_getpeername(sock, (struct net_sockaddr *)&peer, &len);
	zexpect_equal(ret, 0, "getpeername() failed (%d)", errno);
	zexpect_true(net_ipv4_addr_cmp(&peer.sin_addr, &neighbor_addr),
		     "The peer changed in the failed connect()");

	zexpect_equal(zsock_send(sock, "x", 1, 0), 1, "send() to the earlier peer failed");

	zsock_close(sock);
}

/* What the stack turns around to the host itself needs no next hop, and
 * connect() lets it through as sendto() does.
 */
ZTEST(net_socket_unreachable, test_udp_connect_to_loopback)
{
	struct net_sockaddr_in peer;
	int sock;
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IP_ADDR_CHECK);

	sock = udp_socket();
	make_peer(&peer, &loopback_addr);

	ret = zsock_connect(sock, (struct net_sockaddr *)&peer, sizeof(peer));
	zexpect_equal(ret, 0, "connect() to the loopback address failed (%d)", errno);
	zexpect_equal(zsock_send(sock, "x", 1, 0), 1, "send() to the loopback address failed");

	zsock_close(sock);
}

/* A route is a next hop too, with neither the gateway nor an address on
 * the link.
 */
ZTEST(net_socket_unreachable, test_udp_sendto_through_route)
{
	struct net_in_addr prefix = { { { 198, 51, 100, 0 } } };
	struct net_route_entry *route;
	int sock;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_ROUTE);

	sock = udp_socket();

	set_netmask(&netmask_24);
	route = net_route_ipv4_add(eth_iface, &prefix, 24, &neighbor_addr,
				   NET_ROUTE_INFINITE_LIFETIME, NET_ROUTE_PREFERENCE_LOW);
	zassert_not_null(route, "Cannot add route");

	zexpect_equal(udp_sendto(sock, &remote_addr), 1, "Off-link destination via route");

	zassert_equal(net_route_ipv4_del(route), 0, "Cannot delete route");

	zexpect_equal(udp_sendto(sock, &remote_addr), -ENETUNREACH,
		      "Off-link destination after the route went");

	zsock_close(sock);
}

#if defined(CONFIG_NET_VLAN)
#define VLAN_TAG 100

static const struct net_in_addr vlan_my_addr = { { { 203, 0, 113, 2 } } };
static const struct net_in_addr vlan_neighbor_addr = { { { 203, 0, 113, 1 } } };
static const struct net_in_addr vlan_gw_addr = { { { 203, 0, 113, 3 } } };
/* Off-link under the /25 netmask, and closer to the VLAN address than to the
 * Ethernet one, so that sends to it go out on the VLAN interface.
 */
static const struct net_in_addr vlan_remote_addr = { { { 203, 0, 113, 200 } } };
static const struct net_in_addr netmask_25 = { { { 255, 255, 255, 128 } } };

static struct net_if *vlan_iface_get(void)
{
	static struct net_if *vlan_iface;
	int ret;

	if (vlan_iface != NULL) {
		return vlan_iface;
	}

	zassert_equal(net_eth_vlan_enable(eth_iface, VLAN_TAG), 0, "Cannot enable VLAN");
	vlan_iface = net_eth_get_vlan_iface(eth_iface, VLAN_TAG);
	zassert_not_null(vlan_iface, "No VLAN interface");

	zassert_not_null(net_if_ipv4_addr_add(vlan_iface, &vlan_my_addr, NET_ADDR_MANUAL, 0),
			 "Cannot add IPv4 address to VLAN interface");
	zassert_true(net_if_ipv4_set_netmask_by_addr(vlan_iface, &vlan_my_addr, &netmask_25),
		     "Cannot set VLAN netmask");

	ret = net_if_up(vlan_iface);
	zassert_true(ret == 0 || ret == -EALREADY, "Cannot bring VLAN interface up (%d)", ret);

	return vlan_iface;
}

/* A VLAN interface hands its packets to the Ethernet interface it is attached
 * to, which here has a /32 address and no gateway. They are sent by what the
 * VLAN interface has for a next hop, not by what the Ethernet interface has.
 */
ZTEST(net_socket_unreachable, test_udp_sendto_on_vlan)
{
	struct net_if *vlan_iface = vlan_iface_get();
	int sock;

	net_if_ipv4_set_gw(vlan_iface, net_ipv4_unspecified_address());

	sock = udp_socket();

	zexpect_equal(udp_sendto(sock, &vlan_remote_addr), -ENETUNREACH,
		      "Off-link destination without a VLAN gateway");

	zexpect_equal(udp_sendto(sock, &vlan_neighbor_addr), 1, "On-link VLAN destination");
	k_sleep(K_MSEC(10));
	zexpect_true(atomic_get(&frames_sent) > 0, "Nothing was sent to the VLAN neighbor");

	net_if_ipv4_set_gw(vlan_iface, &vlan_gw_addr);
	atomic_set(&frames_sent, 0);
	zexpect_equal(udp_sendto(sock, &vlan_remote_addr), 1,
		      "Off-link destination via VLAN gateway");
	k_sleep(K_MSEC(10));
	zexpect_true(atomic_get(&frames_sent) > 0, "Nothing was sent to the VLAN gateway");

	zsock_close(sock);
}
#endif /* CONFIG_NET_VLAN */

ZTEST_SUITE(net_socket_unreachable, NULL, unreachable_setup, unreachable_before, NULL, NULL);
