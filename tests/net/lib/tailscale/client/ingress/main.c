/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include "tailscale_internal.h"

#include <psa/crypto.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include "wg_private.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/ztest.h>

#include "ingress.h"
#include "ipv4.h"
#include "policy.h"
#include "net_stack.h"
#include "udp_internal.h"

#define ALICE_PRIVATE "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"
#define ALICE_PUBLIC  "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"
#define BOB_PUBLIC    "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f"
#define TEST_TCP_SERVICE_PORT 9001
#define TEST_UDP_SERVICE_PORT 9000
#define TEST_WILDCARD_PORT 9120
#define TEST_APP_SERVICE_PORT 9121
#define TEST_PEER_PORT 46210

#define MAP "{\"Seq\":1,\"Node\":{\"ID\":10,\"Key\":\"nodekey:" ALICE_PUBLIC "\"," \
	    "\"Addresses\":[\"198.51.100.1/32\",\"2001:db8::1/128\"]},\"Peers\":[{\"ID\":11," \
	    "\"Key\":\"nodekey:" BOB_PUBLIC "\",\"Addresses\":[\"198.51.100.2/32\"," \
	    "\"2001:db8::2/128\"]}],\"PacketFilter\":[{\"SrcIPs\":[\"198.51.100.2\"," \
	    "\"2001:db8::2\"],\"DstPorts\":[{\"IP\":\"*\",\"Ports\":{\"First\":1," \
	    "\"Last\":65535}}],\"IPProto\":[6,17]}]}"

static struct net_if *tunnel;
static wireguard_policy_input_t input;
static void *input_context;
static const struct net_in_addr self_ipv4 = {.s_addr = net_htonl(0xc6336401U)};
static const struct net_in_addr peer_ipv4 = {.s_addr = net_htonl(0xc6336402U)};
static const struct net_in_addr denied_ipv4 = {.s_addr = net_htonl(0xc6336403U)};
static const struct net_in_addr physical_ipv4 = {.s_addr = net_htonl(0x7f000001U)};

int __real_wireguard_replace_peers(struct net_if *iface, const struct wireguard_policy_peer *peers,
				   size_t count, wireguard_policy_input_t allow_input,
				   void *context);

int __wrap_wireguard_replace_peers(struct net_if *iface, const struct wireguard_policy_peer *peers,
				   size_t count, wireguard_policy_input_t allow_input,
				   void *context)
{
	int rc = __real_wireguard_replace_peers(iface, peers, count, allow_input, context);

	if (rc == 0) {
		input = allow_input;
		input_context = context;
	}
	return rc;
}

static int apply(uint32_t generation, const char *json)
{
	uint8_t frame[1024];
	size_t len = strlen(json);

	zassert_true(len <= sizeof(frame));
	memcpy(frame, json, len);
	return tailscale_client_policy_apply(generation, frame, len, NULL);
}

static void start_policy(void)
{
	uint8_t private_key[32];
	uint8_t public_key[32];
	int index = net_if_get_by_name("wg0");

	zassert_true(index > 0);
	tunnel = net_if_get_by_index(index);
	zassert_ok(psa_crypto_init());
	zassert_equal(hex2bin(ALICE_PRIVATE, strlen(ALICE_PRIVATE), private_key, 32), 32);
	zassert_equal(hex2bin(ALICE_PUBLIC, strlen(ALICE_PUBLIC), public_key, 32), 32);
	zassert_ok(tailscale_client_policy_start_hosted(tunnel, private_key, public_key));
}

static bool decide_as(uint64_t identity, uint8_t family, uint8_t protocol, const void *source,
		      const void *destination, uint16_t source_port, uint16_t port,
		      uint8_t tcp_flags)
{
	uint8_t packet[60] = {0};
	size_t header_len = family == NET_AF_INET ? 20 : 40;
	size_t transport_len = protocol == NET_IPPROTO_TCP ? 20 : 8;
	uint8_t *transport = &packet[header_len];

	if (family == NET_AF_INET) {
		packet[0] = 0x45;
		sys_put_be16(header_len + transport_len, packet + 2);
		packet[9] = protocol;
		memcpy(packet + 12, source, sizeof(struct net_in_addr));
		memcpy(packet + 16, destination, sizeof(struct net_in_addr));
	} else {
		packet[0] = 0x60;
		sys_put_be16(transport_len, packet + 4);
		packet[6] = protocol;
		memcpy(packet + 8, source, sizeof(struct net_in6_addr));
		memcpy(packet + 24, destination, sizeof(struct net_in6_addr));
	}
	sys_put_be16(source_port, transport);
	sys_put_be16(port, transport + 2);
	if (protocol == NET_IPPROTO_TCP) {
		transport[12] = 0x50;
		transport[13] = tcp_flags;
	} else {
		sys_put_be16(transport_len, transport + 4);
	}
	return input(identity, packet, header_len + transport_len, input_context);
}

static bool decide(uint8_t family, uint8_t protocol, const void *source, const void *destination,
		   uint16_t source_port, uint16_t port, uint8_t tcp_flags)
{
	return decide_as(11, family, protocol, source, destination, source_port, port, tcp_flags);
}

static int udp_wildcard(uint16_t port)
{
	struct net_sockaddr_in address = {.sin_family = NET_AF_INET, .sin_port = net_htons(port)};
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(fd >= 0);
	zassert_ok(zsock_bind(fd, (struct net_sockaddr *)&address, sizeof(address)));
	return fd;
}

static bool inject_overlay_udp(uint16_t port, uint8_t marker)
{
	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(tunnel, sizeof(marker), NET_AF_INET,
							   NET_IPPROTO_UDP, K_SECONDS(1));
	uint8_t bytes[29];

	zassert_not_null(pkt);
	zassert_ok(net_ipv4_create(pkt, &peer_ipv4, &self_ipv4));
	zassert_ok(net_udp_create(pkt, net_htons(TEST_PEER_PORT), net_htons(port)));
	zassert_ok(net_pkt_write_u8(pkt, marker));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, bytes, sizeof(bytes)));
	if (!input(11, bytes, sizeof(bytes), input_context)) {
		net_pkt_unref(pkt);
		return false;
	}
	net_pkt_set_l2_processed(pkt, true);
	zassert_ok(net_recv_data(tunnel, pkt));
	return true;
}

static void expect_udp(int fd, bool expected, uint8_t marker)
{
	struct zsock_pollfd pfd = {.fd = fd, .events = ZSOCK_POLLIN};
	uint8_t received;

	zassert_equal(zsock_poll(&pfd, 1, 150), expected ? 1 : 0);
	if (expected) {
		zassert_equal(zsock_recv(fd, &received, 1, ZSOCK_MSG_DONTWAIT), 1);
		zassert_equal(received, marker);
	}
}

static void expect_wildcards_closed(const struct tailscale_service_rule *rules, size_t count)
{
	const struct tailscale_service_rule wildcard_rules[] = {
		{.port = TEST_WILDCARD_PORT, .protocol = NET_IPPROTO_UDP},
		{.port = TEST_TCP_SERVICE_PORT, .protocol = NET_IPPROTO_UDP},
	};
	struct net_sockaddr_in physical = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(TEST_WILDCARD_PORT),
		.sin_addr = physical_ipv4,
	};
	int unlisted = udp_wildcard(TEST_WILDCARD_PORT);
	int wrong_protocol = udp_wildcard(TEST_TCP_SERVICE_PORT);
	uint8_t marker = 0xa5;

	zassert_ok(tailscale_ingress_replace(wildcard_rules, ARRAY_SIZE(wildcard_rules)));
	zassert_true(inject_overlay_udp(TEST_WILDCARD_PORT, marker));
	expect_udp(unlisted, true, marker);
	zassert_true(inject_overlay_udp(TEST_TCP_SERVICE_PORT, marker));
	expect_udp(wrong_protocol, true, marker);

	zassert_ok(tailscale_ingress_replace(rules, count));
	zassert_false(inject_overlay_udp(TEST_WILDCARD_PORT, marker));
	zassert_false(inject_overlay_udp(TEST_TCP_SERVICE_PORT, marker));
	expect_udp(unlisted, false, marker);
	expect_udp(wrong_protocol, false, marker);

	int sender = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(sender >= 0);
	zassert_equal(zsock_sendto(sender, &marker, 1, 0, (struct net_sockaddr *)&physical,
				   sizeof(physical)), 1);
	expect_udp(unlisted, true, marker);
	zassert_ok(zsock_close(sender));
	zassert_ok(zsock_close(wrong_protocol));
	zassert_ok(zsock_close(unlisted));
}

static uint16_t bound_ipv4_port(int fd)
{
	struct net_sockaddr_in local;
	net_socklen_t len = sizeof(local);

	zassert_ok(zsock_getsockname(fd, (struct net_sockaddr *)&local, &len));
	zassert_equal(local.sin_family, NET_AF_INET);
	zassert_true(net_ipv4_addr_cmp(&local.sin_addr, &self_ipv4));
	zassert_not_equal(local.sin_port, 0);
	return net_ntohs(local.sin_port);
}

static void expect_application_service(void)
{
	const struct tailscale_service_rule rule = {
		.port = TEST_APP_SERVICE_PORT, .protocol = NET_IPPROTO_UDP,
	};
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	uint8_t marker = 0xa5;

	zassert_true(fd >= 0);
	struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET,
		.sin_addr = self_ipv4,
		.sin_port = net_htons(TEST_APP_SERVICE_PORT),
	};

	zassert_ok(zsock_bind(fd, (const struct net_sockaddr *)&local, sizeof(local)));
	zassert_equal(bound_ipv4_port(fd), TEST_APP_SERVICE_PORT);
	zassert_false(inject_overlay_udp(TEST_APP_SERVICE_PORT, marker));
	expect_udp(fd, false, marker);

	zassert_ok(tailscale_ingress_replace(&rule, 1));
	zassert_true(inject_overlay_udp(TEST_APP_SERVICE_PORT, marker));
	expect_udp(fd, true, marker);
	zassert_ok(tailscale_ingress_replace(NULL, 0));
	zassert_false(inject_overlay_udp(TEST_APP_SERVICE_PORT, marker));
	zassert_ok(zsock_close(fd));
}

static void expect_service_unavailable(void)
{
	const struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_addr = self_ipv4,
		.sin_port = net_htons(TEST_APP_SERVICE_PORT),
	};
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(fd >= 0);
	zassert_equal(zsock_bind(fd, (const struct net_sockaddr *)&addr, sizeof(addr)), -1);
	zassert_ok(zsock_close(fd));
}

static void expect_wildcard_route(const struct net_sockaddr *peer, net_socklen_t len)
{
	struct net_sockaddr_storage local = {.ss_family = peer->sa_family};
	int fd = zsock_socket(peer->sa_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(fd >= 0);
	zassert_ok(zsock_bind(fd, net_sad(&local), len));
	zassert_ok(zsock_connect(fd, peer, len));
	zassert_equal_ptr(net_context_get_iface(zvfs_get_fd_obj(fd, NULL, 0)), tunnel);
	zassert_ok(zsock_close(fd));
}

static bool transmit(struct net_pkt *pkt, struct tailscale_filter_tuple *tuple)
{
	uint64_t revision = tailscale_client_policy_route_revision();

	tailscale_ingress_queue(pkt);
	return tailscale_ingress_transmit(pkt, tuple, false, revision, 11);
}

static void expect_pending_tx_kept(void)
{
	struct tailscale_filter_tuple tuple = {
		.family = NET_AF_INET,
		.protocol = NET_IPPROTO_UDP,
		.source_port = TEST_UDP_SERVICE_PORT,
		.destination_port = TEST_PEER_PORT,
		.source = (const uint8_t *)&self_ipv4,
		.destination = (const uint8_t *)&peer_ipv4,
	};
	struct net_pkt *pending = net_pkt_alloc(K_SECONDS(1));
	struct net_pkt *fast = net_pkt_alloc(K_SECONDS(1));
	uint64_t revision = tailscale_client_policy_route_revision();

	zassert_not_null(pending);
	zassert_not_null(fast);
	tailscale_ingress_queue(pending);
	/* Faster traffic classes can overtake the old packet repeatedly. */
	for (size_t i = 0; i < CONFIG_NET_PKT_TX_COUNT * 2U; i++) {
		tailscale_ingress_queue(fast);
		if (i % 2U == 0U) {
			zassert_true(tailscale_ingress_transmit(fast, &tuple, true, revision, 11));
		} else {
			tailscale_net_unqueue(fast);
			zassert_false(tailscale_ingress_transmit(fast, &tuple, true, revision, 11));
		}
	}
	zassert_true(tailscale_ingress_transmit(pending, &tuple, true, revision, 11));
	net_pkt_unref(fast);
	net_pkt_unref(pending);
}

static void expect_return_policy(void)
{
	const struct net_sockaddr_in peer = {
		.sin_family = NET_AF_INET,
		.sin_addr = peer_ipv4,
		.sin_port = net_htons(TEST_PEER_PORT),
	};
	const struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET,
		.sin_addr = self_ipv4,
	};
	struct tailscale_filter_tuple tuple = {
		.family = NET_AF_INET,
		.protocol = NET_IPPROTO_UDP,
		.destination_port = TEST_PEER_PORT,
	};
	struct net_pkt *pkt = net_pkt_alloc(K_SECONDS(1));
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	uint8_t marker = 0xa5;

	zassert_not_null(pkt);
	zassert_true(fd >= 0);
	zassert_ok(zsock_bind(fd, (const struct net_sockaddr *)&local, sizeof(local)));
	zassert_ok(zsock_connect(fd, (const struct net_sockaddr *)&peer, sizeof(peer)));
	tuple.source_port = bound_ipv4_port(fd);
	tuple.source = (const uint8_t *)&self_ipv4;
	tuple.destination = (const uint8_t *)&peer_ipv4;
	net_pkt_set_context(pkt, zvfs_get_fd_obj(fd, NULL, 0));

	zassert_false(inject_overlay_udp(tuple.source_port, marker));
	zassert_true(transmit(pkt, &tuple));
	zassert_false(decide_as(12, NET_AF_INET, NET_IPPROTO_UDP, &peer_ipv4, &self_ipv4,
			       TEST_PEER_PORT, tuple.source_port, 0));
	zassert_true(inject_overlay_udp(tuple.source_port, marker));
	expect_udp(fd, true, marker);

	/* Both the return grant and a queued send belong to the closed socket. */
	tailscale_ingress_queue(pkt);
	zassert_ok(zsock_close(fd));
	int wildcard = udp_wildcard(tuple.source_port);

	zassert_false(tailscale_ingress_transmit(pkt, &tuple, false,
						tailscale_client_policy_route_revision(), 11));
	zassert_false(inject_overlay_udp(tuple.source_port, marker));
	expect_udp(wildcard, false, marker);
	zassert_ok(zsock_close(wildcard));

	net_pkt_set_context(pkt, NULL);
	tuple.protocol = NET_IPPROTO_TCP;
	tuple.tcp_flags = 0x12;
	zassert_false(transmit(pkt, &tuple));
	tuple.tcp_flags = 0x02;
	zassert_true(transmit(pkt, &tuple));
	zassert_true(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4,
			    TEST_PEER_PORT, tuple.source_port, 0x12));
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4,
			     TEST_PEER_PORT, tuple.source_port, 0x10));
	tuple.tcp_flags = 0x10;
	zassert_true(transmit(pkt, &tuple));
	zassert_true(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4,
			    TEST_PEER_PORT, tuple.source_port, 0x10));

	uint64_t revision = tailscale_client_policy_route_revision();

	tailscale_ingress_queue(pkt);
	zassert_ok(apply(1, "{\"Seq\":2,\"DNSConfig\":{\"Resolvers\":[{\"Addr\":\"1.1.1.1\"}]}}"));
	zassert_ok(apply(1, "{\"Seq\":3,\"OnlineChange\":{\"11\":true}}"));
	zassert_ok(apply(1, "{\"KeepAlive\":true,\"ControlTime\":\"2030-01-01T00:00:00Z\"}"));
	zassert_equal(tailscale_client_policy_route_revision(), revision);
	zassert_true(tailscale_ingress_transmit(pkt, &tuple, false, revision, 11));
	zassert_true(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4,
			    TEST_PEER_PORT, tuple.source_port, 0x10));

	/* A queued packet and an established TCP flow cannot cross an ACL change. */
	tailscale_ingress_queue(pkt);
	zassert_ok(apply(1, "{\"Seq\":4,\"PacketFilter\":[]}"));
	zassert_false(tailscale_ingress_transmit(pkt, &tuple, false,
						tailscale_client_policy_route_revision(), 11));
	zassert_false(transmit(pkt, &tuple));
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4,
			     TEST_PEER_PORT, tuple.source_port, 0x10));
	net_pkt_unref(pkt);
}

static void expect_lan_exception(void)
{
	static const char map[] =
		"{\"Seq\":1,\"Node\":{\"ID\":10,\"Key\":\"nodekey:" ALICE_PUBLIC
		"\",\"Addresses\":[\"198.51.100.1/32\"]},\"Peers\":[{\"ID\":11,"
		"\"StableID\":\"exitA\",\"Key\":\"nodekey:" BOB_PUBLIC
		"\",\"Addresses\":[\"198.51.100.2/32\"],\"AllowedIPs\":[\"198.51.100.2/32\","
		"\"192.0.2.64/26\",\"0.0.0.0/0\",\"::/0\"]}],"
		"\"DNSConfig\":{\"Routes\":{\"corp.example\":[{\"Addr\":\"192.0.2.2\"}]}}}";
	struct tailscale_routes prefs = {.accept_routes = true};
	struct net_sockaddr_in dst = {.sin_family = NET_AF_INET, .sin_port = net_htons(53)};
	struct net_in_addr source, mask;
	struct net_if *lan = NULL;
	struct net_if *selected;
	struct net_sockaddr_storage local;
	struct tailscale_dns_selection dns;
	uint64_t revision;

	zassert_not_null(net_if_ipv4_addr_lookup(&physical_ipv4, &lan));
	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.25", &source));
	zassert_ok(net_addr_pton(NET_AF_INET, "255.255.255.0", &mask));
	zassert_not_null(net_if_ipv4_addr_add(lan, &source, NET_ADDR_MANUAL, 0));
	zassert_true(net_if_ipv4_set_netmask_by_addr(lan, &source, &mask));
	tailscale_client_policy_set_underlay(lan);
	start_policy();
	zassert_ok(tailscale_routes_set(&prefs));
	zassert_ok(apply(3, map));

	zassert_true(tailscale_net_dns_offload_allowed("unrelated.example"));
	zassert_false(tailscale_net_dns_offload_allowed("db.corp.example"));
	zassert_ok(tailscale_client_policy_select_dns("db.corp.example", &dns));
	zassert_equal(dns.source, TAILSCALE_DNS_SPLIT);
	zassert_equal(dns.server_count, 1U);
	zassert_equal(dns.servers[0].family, NET_AF_INET);
	memcpy(&dst.sin_addr, dns.servers[0].address, sizeof(dst.sin_addr));
	zassert_ok(tailscale_client_policy_route_dns((const struct net_sockaddr *)&dst,
						    &selected, &local, &revision));
	zassert_equal_ptr(selected, lan);
	zassert_true(net_ipv4_addr_cmp(&net_sin(net_sad(&local))->sin_addr, &source));

	zassert_ok(net_addr_pton(NET_AF_INET, "100.100.100.100", &dst.sin_addr));
	zassert_equal(tailscale_client_policy_route_dns((const struct net_sockaddr *)&dst,
						       &selected, &local, &revision),
		      -ENETUNREACH);
	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.65", &dst.sin_addr));
	zassert_ok(tailscale_client_policy_route_dns((const struct net_sockaddr *)&dst,
						    &selected, &local, &revision));
	zassert_equal_ptr(selected, tunnel);
	zassert_ok(apply(3, "{\"Seq\":2,\"OnlineChange\":{\"11\":false}}"));
	zassert_equal(tailscale_client_policy_route_dns((const struct net_sockaddr *)&dst,
						       &selected, &local, &revision),
		      -ENETUNREACH);
	zassert_ok(apply(3, "{\"Seq\":3,\"OnlineChange\":{\"11\":true}}"));

	memcpy(prefs.exit_node, "exitA", sizeof("exitA"));
	zassert_ok(tailscale_routes_set(&prefs));
	zassert_false(tailscale_net_dns_offload_allowed("unrelated.example"));
	zassert_equal(tailscale_client_policy_dns_authority("unrelated.example"), -ENETUNREACH);
	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.2", &dst.sin_addr));
	zassert_ok(tailscale_client_policy_route_dns((const struct net_sockaddr *)&dst,
						    &selected, &local, &revision));
	zassert_equal_ptr(selected, tunnel);
	zassert_equal_ptr(net_if_ipv4_select_src_iface(&dst.sin_addr), tunnel);
	prefs.allow_lan = true;
	zassert_ok(tailscale_routes_set(&prefs));
	zassert_equal_ptr(net_if_ipv4_select_src_iface(&dst.sin_addr), lan);
	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.65", &dst.sin_addr));
	zassert_equal_ptr(net_if_ipv4_select_src_iface(&dst.sin_addr), tunnel);
	zassert_ok(tailscale_client_policy_revoke());
	zassert_equal_ptr(net_if_ipv4_select_src_iface(&dst.sin_addr), tunnel);
	zassert_ok(net_addr_pton(NET_AF_INET, "203.0.113.10", &dst.sin_addr));
	zassert_equal_ptr(net_if_ipv4_select_src_iface(&dst.sin_addr), tunnel);

	zassert_ok(tailscale_client_policy_stop());
	tailscale_client_policy_set_underlay(NULL);
	zassert_ok(tailscale_routes_set(&(struct tailscale_routes){0}));
	zassert_true(net_if_ipv4_addr_rm(lan, &source));
}

ZTEST(tailscale_client_ingress, test_direct_service_policy)
{
	const struct tailscale_service_rule allowed[] = {
		{.port = TEST_TCP_SERVICE_PORT, .protocol = NET_IPPROTO_TCP},
		{.port = TEST_UDP_SERVICE_PORT, .protocol = NET_IPPROTO_UDP},
	};
	const struct tailscale_service_rule duplicate[] = {allowed[0], allowed[0]};
	struct tailscale_service_rule saved[TAILSCALE_SERVICE_MAX_RULES];
	struct net_in6_addr peer_ipv6;
	struct net_in6_addr self_ipv6;
	const struct net_in_addr *selected_ipv4;
	const struct net_in6_addr *selected_ipv6;

	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8::2", &peer_ipv6));
	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8::1", &self_ipv6));
	zassert_equal(tailscale_ingress_rules(saved), 0);
	expect_service_unavailable();
	start_policy();
	zassert_ok(apply(1, MAP));
	zassert_equal_ptr(net_if_ipv4_select_src_iface_addr(&peer_ipv4, &selected_ipv4), tunnel);
	zassert_equal_ptr(net_if_ipv6_select_src_iface_addr(&peer_ipv6, &selected_ipv6), tunnel);
	zassert_not_null(selected_ipv4);
	zassert_not_null(selected_ipv6);
	zassert_true(net_ipv4_addr_cmp(selected_ipv4, &self_ipv4));
	zassert_true(net_ipv6_addr_cmp(selected_ipv6, &self_ipv6));
	const struct net_sockaddr_in dst4 = {
		.sin_family = NET_AF_INET, .sin_addr = peer_ipv4,
		.sin_port = net_htons(TEST_PEER_PORT),
	};
	const struct net_sockaddr_in6 dst6 = {
		.sin6_family = NET_AF_INET6, .sin6_addr = peer_ipv6,
		.sin6_port = net_htons(TEST_PEER_PORT),
	};

	expect_wildcard_route((const struct net_sockaddr *)&dst4, sizeof(dst4));
	expect_wildcard_route((const struct net_sockaddr *)&dst6, sizeof(dst6));
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_UDP, &peer_ipv4, &self_ipv4, 1234, 9000, 0));
	expect_wildcards_closed(allowed, ARRAY_SIZE(allowed));
	zassert_equal(tailscale_ingress_replace(duplicate, ARRAY_SIZE(duplicate)), -EALREADY);

	zassert_true(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4, 1234, 9001, 2));
	zassert_true(decide(NET_AF_INET, NET_IPPROTO_UDP, &peer_ipv4, &self_ipv4, 1234, 9000, 0));
	zassert_true(decide(NET_AF_INET6, NET_IPPROTO_TCP, &peer_ipv6, &self_ipv6, 1234, 9001, 2));
	zassert_true(decide(NET_AF_INET6, NET_IPPROTO_UDP, &peer_ipv6, &self_ipv6, 1234, 9000, 0));
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_UDP, &denied_ipv4, &self_ipv4,
			     1234, 9000, 0));
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_UDP, &peer_ipv4, &physical_ipv4,
			     1234, 9000, 0));

	zassert_ok(tailscale_ingress_replace(allowed, 1));
	zassert_true(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4,
			    1234, 9001, 0x10));
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_UDP, &peer_ipv4, &self_ipv4, 1234, 9000, 0));
	zassert_ok(tailscale_ingress_replace(NULL, 0));
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4,
			     1234, 9001, 0x10));
	expect_application_service();
	zassert_ok(tailscale_ingress_replace(allowed, ARRAY_SIZE(allowed)));
	expect_pending_tx_kept();
	expect_return_policy();
	zassert_false(decide(NET_AF_INET, NET_IPPROTO_TCP, &peer_ipv4, &self_ipv4, 1234, 9001, 2));
	zassert_false(decide(NET_AF_INET6, NET_IPPROTO_UDP, &peer_ipv6, &self_ipv6, 1234, 9000, 0));
	zassert_ok(tailscale_client_policy_revoke());
	zassert_equal_ptr(net_if_ipv4_select_src_iface(&peer_ipv4), tunnel);
	zassert_equal_ptr(net_if_ipv6_select_src_iface(&peer_ipv6), tunnel);
	expect_service_unavailable();
	zassert_ok(tailscale_client_policy_stop());
	start_policy();
	int restart_rc = apply(2, MAP);

	zassert_ok(restart_rc, "Could not apply policy after restart (%d)", restart_rc);
	zassert_equal(tailscale_ingress_rules(saved), ARRAY_SIZE(allowed));
	zassert_equal(saved[0].protocol, allowed[0].protocol);
	zassert_equal(saved[0].port, allowed[0].port);
	zassert_equal(saved[1].protocol, allowed[1].protocol);
	zassert_equal(saved[1].port, allowed[1].port);
	zassert_ok(tailscale_client_policy_stop());
	expect_service_unavailable();
	zassert_ok(tailscale_ingress_replace(NULL, 0));
	expect_lan_exception();
}

ZTEST_SUITE(tailscale_client_ingress, NULL, NULL, NULL, NULL, NULL);
