/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmdline.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/virtual_mgmt.h>
#include "wg_private.h"
#include <zephyr/random/random.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define ECHO_PORT 9000
#define PEERS     CONFIG_WIREGUARD_MAX_PEER

static void overlay_ip(int family, unsigned int leaf, void *out)
{
	if (family == NET_AF_INET) {
		struct net_in_addr *ipv4 = out;

		/* Use the next documentation subnet after the /24 host routes fill. */
		ipv4->s_addr = net_htonl(leaf < 254 ? 0xc6336401U + leaf
						  : 0xcb007101U + (leaf - 254));
	} else {
		struct net_in6_addr *ipv6 = out;

		zassert_ok(net_addr_pton(family, "2001:db8:1::", ipv6));
		sys_put_be16(1 + leaf, &ipv6->s6_addr[14]);
	}
}

static void public_key(struct net_if *iface, uint8_t encoded[45])
{
	struct virtual_interface_req_params params = {0};
	size_t written;

	zassert_ok(net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_GET_PUBLIC_KEY, iface, &params,
			    sizeof(params)));
	zassert_ok(base64_encode(encoded, 45, &written, params.public_key.data,
				 params.public_key.len));
}

static int add_peer(struct net_if *iface, size_t i, const char *pubkey)
{
	struct wireguard_peer_config config = {.public_key = pubkey};
	struct net_sockaddr_in *remote = net_sin(net_sad(&config.endpoint_ip));

	remote->sin_family = NET_AF_INET;
	remote->sin_port = net_htons(51001 + i);
	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.1", &remote->sin_addr));
	config.allowed_ip[0].addr.family = NET_AF_INET;
	config.allowed_ip[0].is_valid = true;
	config.allowed_ip[0].mask_len = i == 0 ? 24 : 32;
	overlay_ip(NET_AF_INET, i + 1, &config.allowed_ip[0].addr.in_addr);
	config.allowed_ip[1].addr.family = NET_AF_INET6;
	config.allowed_ip[1].is_valid = true;
	config.allowed_ip[1].mask_len = i == 0 ? 64 : 128;
	overlay_ip(NET_AF_INET6, i + 1, &config.allowed_ip[1].addr.in6_addr);
#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	return wireguard_peer_add_to_interface(iface, &config, NULL);
#else
	struct net_if *assigned = NULL;
	int id = wireguard_peer_add(&config, &assigned);

	zassert_true(id > 0, "legacy peer add returned %d", id);
	zassert_equal(assigned, iface);
	return id;
#endif
}

static net_socklen_t address(int family, unsigned int leaf, int port,
			     struct net_sockaddr_storage *storage)
{
	struct net_sockaddr *addr = net_sad(storage);

	addr->sa_family = family;
	if (family == NET_AF_INET) {
		net_sin(addr)->sin_port = net_htons(port);
		overlay_ip(family, leaf, &net_sin(addr)->sin_addr);
		return sizeof(struct net_sockaddr_in);
	}

	net_sin6(addr)->sin6_port = net_htons(port);
	overlay_ip(family, leaf, &net_sin6(addr)->sin6_addr);
	return sizeof(struct net_sockaddr_in6);
}

static int connected_socket(struct net_if *iface, unsigned int leaf, int family)
{
	struct net_sockaddr_storage local = {0}, remote = {0};
	net_socklen_t len = address(family, 0, 0, &local);
	struct net_ifreq ifreq = {0};
	struct zsock_timeval timeout = {.tv_usec = 400000};
	int fd = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(fd >= 0);
	zassert_true(net_if_get_name(iface, ifreq.ifr_name, sizeof(ifreq.ifr_name)) >= 0);
	zassert_ok(zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &ifreq,
				    sizeof(ifreq)));
	zassert_ok(zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
				    sizeof(timeout)));
	zassert_ok(zsock_bind(fd, net_sad(&local), len));
	len = address(family, leaf, ECHO_PORT, &remote);
	zassert_ok(zsock_connect(fd, net_sad(&remote), len));
	return fd;
}

static bool exchange(struct net_if *iface, unsigned int leaf, int family)
{
	int fd = connected_socket(iface, leaf, family);
	uint8_t nonce[16], reply[sizeof(nonce) + 1];
	bool ok = false;

	zassert_ok(sys_csrand_get(nonce, sizeof(nonce)));
	for (size_t attempt = 0; attempt < 12 && !ok; attempt++) {
		if (zsock_send(fd, nonce, sizeof(nonce), 0) == sizeof(nonce) &&
		    zsock_recv(fd, reply, sizeof(reply), 0) == sizeof(nonce)) {
			ok = memcmp(nonce, reply, sizeof(nonce)) == 0;
		}
		if (!ok) {
			k_sleep(K_MSEC(80));
		}
	}
	zassert_ok(zsock_close(fd));
	return ok;
}

static void check_leaf(struct net_if *iface, int leaf)
{
	zassert_true(exchange(iface, leaf, NET_AF_INET), "leaf %d IPv4 nonce", leaf);
	zassert_true(exchange(iface, leaf, NET_AF_INET6), "leaf %d IPv6 nonce", leaf);
	printk("Linux peer %d echoed IPv4 and IPv6 nonces\n", leaf);
}

static void rekey_with_traffic(struct net_if *iface)
{
	struct net_stats_vpn stats = {0};
	uint8_t nonce[16], reply[sizeof(nonce) + 1];
	uint32_t before;
	int fd = connected_socket(iface, 3, NET_AF_INET6);
	int64_t deadline;

	zassert_ok(net_mgmt(NET_REQUEST_STATS_GET_VPN, iface, &stats, sizeof(stats)));
	before = stats.handshake_init_rx;
	zassert_ok(sys_csrand_get(nonce, sizeof(nonce)));
	zassert_equal(zsock_send(fd, nonce, sizeof(nonce), 0), sizeof(nonce));
	printk("Request fresh Linux peer 2 handshake\n");
	deadline = k_uptime_get() + 10000;
	do {
		zassert_true(exchange(iface, 1, NET_AF_INET));
		zassert_ok(net_mgmt(NET_REQUEST_STATS_GET_VPN, iface, &stats, sizeof(stats)));
	} while (stats.handshake_init_rx == before && k_uptime_get() < deadline);
	zassert_true(stats.handshake_init_rx > before, "missing fresh Linux handshake");
	check_leaf(iface, 2);
	zassert_equal(zsock_recv(fd, reply, sizeof(reply), 0), sizeof(nonce));
	zassert_mem_equal(nonce, reply, sizeof(nonce));
	zassert_ok(zsock_close(fd));
	printk("Peer 2 established a fresh session during peer 1 and 3 traffic\n");
}

static void remove_with_traffic(struct net_if *iface, const uint8_t original[45], int removed)
{
	uint8_t remaining[45];

	/* Keep one flow to each survivor open across removal of the middle peer. */
	int sockets[] = {connected_socket(iface, 1, NET_AF_INET),
			 connected_socket(iface, 3, NET_AF_INET6)};
	uint8_t pending[ARRAY_SIZE(sockets)][16];
	uint8_t reply[sizeof(pending[0]) + 1];

	for (size_t i = 0; i < ARRAY_SIZE(sockets); i++) {
		zassert_ok(sys_csrand_get(pending[i], sizeof(pending[i])));
		zassert_equal(zsock_send(sockets[i], pending[i], sizeof(pending[i]), 0),
			      sizeof(pending[i]));
	}
	zassert_ok(wireguard_peer_remove(removed));
	for (size_t i = 0; i < ARRAY_SIZE(sockets); i++) {
		zassert_equal(zsock_recv(sockets[i], reply, sizeof(reply), 0), sizeof(pending[i]));
		zassert_mem_equal(pending[i], reply, sizeof(pending[i]));
		zassert_ok(zsock_close(sockets[i]));
	}
	zassert_equal(wireguard_peer_keepalive(removed), -ENOENT);
	zassert_true(net_if_is_up(iface));
	public_key(iface, remaining);
	zassert_mem_equal(original, remaining, sizeof(remaining));
	check_leaf(iface, 1);
	check_leaf(iface, 3);
	zassert_false(exchange(iface, 2, NET_AF_INET), "removed peer replied");
}

ZTEST(wireguard_interop, test_shared_identity)
{
	struct net_if *iface = net_if_get_by_index(net_if_get_by_name("wg0"));
	struct virtual_interface_req_params key = {0};
	struct net_in_addr local4;
	struct net_in6_addr local6;
	uint8_t original[45], raw[32];
	int ids[PEERS], argc;
	char **argv;

	native_get_test_cmd_line_args(&argc, &argv);
	zassert_equal(argc, PEERS, "supply one Linux public key per configured peer slot");
	zassert_not_null(iface);
	zassert_ok(sys_csrand_get(raw, sizeof(raw)));
	key.private_key.data = raw;
	key.private_key.len = sizeof(raw);
	zassert_ok(
		net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, iface, &key, sizeof(key)));
	public_key(iface, original);
	for (size_t i = 0; i < PEERS; i++) {
		ids[i] = add_peer(iface, i, argv[i]);
		zassert_true(ids[i] > 0, "peer %zu add returned %d", i, ids[i]);
	}
	overlay_ip(NET_AF_INET, 0, &local4);
	overlay_ip(NET_AF_INET6, 0, &local6);
	zassert_not_null(net_if_ipv4_addr_add(iface, &local4, NET_ADDR_MANUAL, 0));
	zassert_not_null(net_if_ipv6_addr_add(iface, &local6, NET_ADDR_MANUAL, 0));
	printk("Hub public key %s\n", original);
	struct net_stats_vpn stats = {0};
	int64_t deadline = k_uptime_get() + 15000 + PEERS * 100;

	do {
		zassert_ok(net_mgmt(NET_REQUEST_STATS_GET_VPN, iface, &stats, sizeof(stats)));
		if (stats.handshake_init_rx >= PEERS) {
			break;
		}
		k_sleep(K_MSEC(50));
	} while (k_uptime_get() < deadline);
	zassert_true(stats.handshake_init_rx >= PEERS, "incoming Linux handshakes: %u",
		     stats.handshake_init_rx);
	for (int i = 0; i < PEERS; i++) {
		check_leaf(iface, i + 1);
	}
	printk("IPv4 measurement start\n");
	for (size_t i = 0; i < 64; i++) {
		zassert_true(exchange(iface, 1 + i % PEERS, NET_AF_INET), "timed echo %zu", i);
	}
	printk("Verified 64 IPv4 round trips\n");
	if (argc >= 3) {
		rekey_with_traffic(iface);
		remove_with_traffic(iface, original, ids[1]);
		ids[1] = 0;
	}
	for (size_t i = 0; i < PEERS; i++) {
		if (ids[i] > 0) {
			zassert_ok(wireguard_peer_remove(ids[i]));
		}
	}
}

ZTEST_SUITE(wireguard_interop, NULL, NULL, NULL, NULL, NULL);
