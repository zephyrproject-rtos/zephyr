/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <psa/crypto.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/virtual_mgmt.h>
#include "wg_private.h"
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "policy.h"

#define ALICE_PRIVATE "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"
#define ALICE_PUBLIC  "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"
#define BOB_PUBLIC    "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f"
#define UNKNOWN_PUBLIC "f5827bbafa1359dc883d73d445a62f7702a1933a285eeb174056c49c5c8d6855"
#define THIRD_PUBLIC "0a00000000000000000000000000000000000000000000000000000000000000"

#define MAP(self_public) \
	"{\"Seq\":1,\"Node\":{\"ID\":10,\"Key\":\"nodekey:" self_public "\"," \
	"\"Name\":\"node.tail.example.\",\"Addresses\":[\"198.51.100.1/32\"]}," \
	"\"Peers\":[{\"ID\":11,\"Key\":\"nodekey:" BOB_PUBLIC "\"," \
	"\"Name\":\"peer.tail.example.\",\"Addresses\":[\"198.51.100.2/32\"]}], " \
	"\"PacketFilter\":[]," \
	"\"DNSConfig\":{\"Proxied\":true,\"Routes\":{\"corp.example\":[{\"Addr\":\"198.51.100.2\"}]}}}"

#define LOCAL(expiry) \
	"\"Node\":{\"ID\":10,\"Key\":\"nodekey:" ALICE_PUBLIC "\",\"KeyExpiry\":\"" expiry "\"," \
	"\"Name\":\"node.tail.example.\",\"Addresses\":[\"198.51.100.1/32\"]}"
#define FINITE_PEERS \
	"\"Peers\":[{\"ID\":11,\"Key\":\"nodekey:" BOB_PUBLIC "\"," \
	"\"Name\":\"peer.tail.example.\"," \
	"\"KeyExpiry\":\"2030-01-01T00:00:00.300Z\",\"Addresses\":[\"198.51.100.2/32\"]}," \
	"{\"ID\":12,\"Key\":\"nodekey:" UNKNOWN_PUBLIC "\"," \
	"\"KeyExpiry\":\"2030-01-01T00:00:00.850Z\",\"Addresses\":[\"198.51.100.3/32\"]}," \
	"{\"ID\":13,\"Key\":\"nodekey:" THIRD_PUBLIC "\",\"Addresses\":[\"198.51.100.4/32\"]}]"

static struct net_if *tunnel;
static struct net_in_addr self_address;

static int apply(uint32_t generation, const char *json)
{
	static uint8_t frame[1024];
	size_t length = strlen(json);

	zassert_true(length < sizeof(frame));
	memcpy(frame, json, length);
	return tailscale_client_policy_apply(generation, frame, length, NULL);
}

static struct net_if *named_iface(const char *name)
{
	int index = net_if_get_by_name(name);

	zassert_true(index > 0);
	return net_if_get_by_index(index);
}

static void load_keys(uint8_t private_key[32], uint8_t public_key[32])
{
	zassert_ok(psa_crypto_init());
	zassert_equal(hex2bin(ALICE_PRIVATE, strlen(ALICE_PRIVATE), private_key, 32), 32);
	zassert_equal(hex2bin(ALICE_PUBLIC, strlen(ALICE_PUBLIC), public_key, 32), 32);
}

static void start_policy(void)
{
	uint8_t private_key[32];
	uint8_t public_key[32];

	tunnel = named_iface("wg0");
	zassert_ok(net_addr_pton(NET_AF_INET, "198.51.100.1", &self_address));
	load_keys(private_key, public_key);
	zassert_ok(tailscale_client_policy_start_hosted(tunnel, private_key, public_key));
}

static void expect_policy(uint32_t generation, uint64_t revision, bool valid)
{
	struct tailscale_peer_inventory_summary summary;

	zassert_ok(tailscale_client_policy_summary(&summary));
	zassert_equal(summary.generation, generation);
	zassert_equal(summary.revision, revision);
	zassert_equal(summary.valid, valid);
	zassert_equal(summary.peer_count, valid ? 1 : 0);
	zassert_equal(net_if_ipv4_addr_lookup_by_iface(tunnel, &self_address) != NULL, valid);
}

static bool deny_input(uint64_t identity, const uint8_t *ip, size_t len, void *context)
{
	return false;
}

static bool has_route(uint8_t octet)
{
	struct net_in_addr address = {.s_addr = net_htonl(0xc6336400U + octet)};

	return wireguard_policy_route_owner(tunnel, NET_AF_INET, &address) != 0U;
}

static void check_finite_expiry(void)
{
	const char *map = "{\"Seq\":1," LOCAL("0001-01-01T00:00:00Z") "," FINITE_PEERS "}";
	const char *timed_map = "{\"Seq\":1,\"ControlTime\":\"2030-01-01T00:00:00Z\","
		LOCAL("0001-01-01T00:00:00Z") "," FINITE_PEERS ",\"DNSConfig\":{\"Proxied\":true}}";
	const char *renewal = "{\"Seq\":2,\"PeersChangedPatch\":[{\"NodeID\":11,"
		"\"KeyExpiry\":\"2030-01-01T00:00:00.849Z\"}]}";
	const char *jump_map = "{\"Seq\":1," LOCAL(
		"2030-01-01T00:00:03.500Z") ","
					    "\"Peers\":[{\"ID\":11,\"Key\":\"nodekey:" BOB_PUBLIC
					    "\","
					    "\"KeyExpiry\":\"2030-01-01T00:00:02.800Z\","
					    "\"Addresses\":[\"198.51.100.2/32\"]},"
					    "{\"ID\":13,\"Key\":\"nodekey:" THIRD_PUBLIC
					    "\",\"Addresses\":[\"198.51.100.4/32\"]}]}";
	struct tailscale_dns_answer answer;
	int64_t started;

	start_policy();
	zassert_equal(apply(8, map), -ENODATA);
	expect_policy(7, 4, false);
	started = k_uptime_get();
	zassert_ok(apply(9, timed_map));
	zassert_true(has_route(2));
	zassert_true(has_route(3));
	zassert_true(has_route(4));
	zassert_ok(tailscale_client_policy_resolve_magic_dns("peer.tail.example", NET_AF_INET,
							     &answer));
	k_sleep(K_TIMEOUT_ABS_MS(started + 100));
	zassert_ok(apply(9, renewal));
	k_sleep(K_TIMEOUT_ABS_MS(started + 500));
	zassert_true(has_route(2));
	zassert_true(has_route(3));
	zassert_ok(apply(9, "{\"KeepAlive\":true}"));
	zassert_equal(apply(9,
			    "{\"Seq\":3,\"ControlTime\":\"2050-01-01T00:00:00Z\","
			    "\"PeersChangedPatch\":[{\"NodeID\":11,\"KeyExpiry\":\"invalid\"}]}"),
		      -EBADMSG);
	zassert_true(has_route(2));
	zassert_true(has_route(3));

	k_sleep(K_TIMEOUT_ABS_MS(started + 1200));
	zassert_false(has_route(2));
	zassert_false(has_route(3));
	zassert_true(has_route(4));
	expect_policy(9, 6, true);
	zassert_equal(tailscale_client_policy_resolve_magic_dns("peer.tail.example", NET_AF_INET,
								&answer),
		      -ENOENT);

	zassert_ok(apply(10, "{\"Seq\":1," LOCAL("2030-01-01T00:00:01.400Z") ","
			     FINITE_PEERS "}"));
	zassert_true(has_route(4));
	k_sleep(K_TIMEOUT_ABS_MS(started + 1475));
	zassert_false(has_route(4));
	expect_policy(10, 7, false);
	zassert_equal(apply(10, "{\"KeepAlive\":true}"), -ESTALE);

	zassert_ok(apply(11, "{\"Seq\":1," LOCAL("2030-01-01T00:00:01.900Z") ","
			     FINITE_PEERS "}"));
	zassert_ok(tailscale_client_policy_stop());
	start_policy();
	zassert_ok(apply(12, MAP(ALICE_PUBLIC)));
	k_sleep(K_TIMEOUT_ABS_MS(started + 2000));
	zassert_true(has_route(2));
	expect_policy(12, 9, true);

	/* A valid clock-only keepalive advances expiry without replacing the map. */
	zassert_ok(apply(13, jump_map));
	zassert_true(has_route(2));
	zassert_true(has_route(4));
	zassert_ok(apply(13, "{\"KeepAlive\":true,\"ControlTime\":\"2030-01-01T00:00:02.801Z\"}"));
	zassert_false(has_route(2));
	zassert_true(has_route(4));
	expect_policy(13, 10, true);
	zassert_ok(apply(13, "{\"KeepAlive\":true,\"ControlTime\":\"2030-01-01T00:00:02.700Z\"}"));
	zassert_false(has_route(2));
	zassert_equal(
		apply(13, "{\"KeepAlive\":true,\"ControlTime\":\"2030-01-01T00:00:03.501Z\"}"),
		-EKEYREJECTED);
	zassert_false(has_route(4));
	expect_policy(13, 10, false);
	zassert_equal(apply(13, "{\"KeepAlive\":true}"), -ESTALE);
	zassert_equal(apply(14, jump_map), -EKEYREJECTED);
	zassert_false(has_route(4));
	zassert_ok(apply(15, MAP(ALICE_PUBLIC)));
	expect_policy(15, 11, true);
	zassert_ok(tailscale_client_policy_stop());
	expect_policy(15, 11, false);
}

ZTEST(tailscale_client_policy, test_failed_claim_releases_key)
{
	struct net_if *other = named_iface("wg1");
	struct net_if *target = named_iface("wg0");
	uint8_t private_key[32];
	uint8_t public_key[32];
	struct virtual_interface_req_params key = {
		.private_key = {.data = private_key, .len = sizeof(private_key)},
	};

	load_keys(private_key, public_key);
	zassert_ok(
		net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, other, &key, sizeof(key)));
	zassert_ok(wireguard_replace_peers(other, NULL, 0, deny_input, NULL));
	zassert_equal(tailscale_client_policy_start_hosted(target, private_key, public_key),
		      -EADDRINUSE);
	zassert_equal(wireguard_release_peers(target), -ENOENT);
	zassert_ok(wireguard_release_peers(other));
}

ZTEST(tailscale_client_policy, test_lifecycle)
{
	const char *keepalive = "{\"KeepAlive\":true}";
	struct net_if *underlay = net_if_get_default();
	struct tailscale_dns_selection dns;
	struct tailscale_dns_answer answer;

	start_policy();
	zassert_not_equal(underlay, tunnel);
	zassert_not_null(net_if_ipv4_addr_add(underlay, &self_address, NET_ADDR_MANUAL, 0));
	zassert_equal(apply(1, MAP(ALICE_PUBLIC)), -EADDRINUSE);
	expect_policy(0, 0, false);
	zassert_equal(tailscale_client_policy_select_dns("corp.example", &dns), -ENETDOWN);
	zassert_not_null(net_if_ipv4_addr_lookup_by_iface(underlay, &self_address));
	zassert_true(net_if_ipv4_addr_rm(underlay, &self_address));

	zassert_ok(apply(2, MAP(ALICE_PUBLIC)));
	expect_policy(2, 1, true);
	zassert_ok(tailscale_client_policy_select_dns("host.Corp.Example.", &dns));
	zassert_equal(dns.generation, 2U);
	zassert_equal(dns.revision, 1U);
	zassert_equal(dns.source, TAILSCALE_DNS_SPLIT);
	zassert_equal(dns.server_count, 1U);
	zassert_equal(dns.servers[0].family, NET_AF_INET);
	zassert_equal(dns.servers[0].port, 53U);
	zassert_mem_equal(dns.servers[0].address, ((uint8_t[]){198U, 51U, 100U, 2U}), 4U);
	zassert_ok(tailscale_client_policy_resolve_magic_dns("peer.TAIL.example.", NET_AF_INET,
							     &answer));
	zassert_equal(answer.generation, dns.generation);
	zassert_equal(answer.revision, dns.revision);
	zassert_equal(answer.address_count, 1U);
	zassert_mem_equal(answer.addresses[0].address, dns.servers[0].address, 4U);
	k_sleep(K_MSEC(600));
	zassert_ok(apply(2, keepalive));
	k_sleep(K_MSEC(500));
	expect_policy(2, 1, true);
	zassert_true(apply(2, "{") < 0);
	expect_policy(2, 1, true);
	zassert_ok(tailscale_client_policy_select_dns("corp.example", &dns));
	zassert_equal(dns.revision, 1U);
	zassert_equal(dns.source, TAILSCALE_DNS_SPLIT);

	/* A newer connection without an installed map cannot renew the old lease. */
	k_sleep(K_MSEC(300));
	zassert_ok(apply(3, keepalive));
	k_sleep(K_MSEC(300));
	expect_policy(2, 1, false);
	zassert_equal(tailscale_client_policy_select_dns("corp.example", &dns), -ENETDOWN);
	zassert_equal(tailscale_client_policy_resolve_magic_dns("peer.tail.example", NET_AF_INET,
								&answer),
		      -ENETDOWN);
	zassert_equal(apply(3, MAP(ALICE_PUBLIC)), -ESTALE);

	zassert_ok(apply(4, MAP(ALICE_PUBLIC)));
	expect_policy(4, 2, true);
	zassert_equal(apply(5, MAP(UNKNOWN_PUBLIC)), -EKEYREJECTED);
	expect_policy(4, 2, false);
	zassert_equal(tailscale_client_policy_select_dns("corp.example", &dns), -ENETDOWN);
	zassert_equal(tailscale_client_policy_resolve_magic_dns("peer.tail.example", NET_AF_INET,
								&answer),
		      -ENETDOWN);
	zassert_equal(apply(5, MAP(ALICE_PUBLIC)), -ESTALE);
	zassert_ok(apply(6, MAP(ALICE_PUBLIC)));
	expect_policy(6, 3, true);

	zassert_ok(tailscale_client_policy_stop());
	expect_policy(6, 3, false);
	zassert_equal(apply(6, MAP(ALICE_PUBLIC)), -ENETDOWN);
	start_policy();
	zassert_equal(apply(6, MAP(ALICE_PUBLIC)), -ESTALE);
	zassert_ok(apply(7, MAP(ALICE_PUBLIC)));
	expect_policy(7, 4, true);
	zassert_ok(tailscale_client_policy_stop());
	expect_policy(7, 4, false);
	zassert_equal(tailscale_client_policy_select_dns("corp.example", &dns), -ENETDOWN);
	zassert_equal(tailscale_client_policy_resolve_magic_dns("peer.tail.example", NET_AF_INET,
								&answer),
		      -ENETDOWN);

	check_finite_expiry();
}

ZTEST_SUITE(tailscale_client_policy, NULL, NULL, NULL, NULL, NULL);

#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
#include "tailscale_internal.h"

static uint64_t route_owner(const char *address)
{
	uint8_t raw[16];
	int family = strchr(address, ':') ? NET_AF_INET6 : NET_AF_INET;

	zassert_ok(net_addr_pton(family, address, raw));
	return wireguard_policy_route_owner(tunnel, family, raw);
}

ZTEST(tailscale_client_policy, test_subnet_and_exit_selection)
{
	const char *map = "{\"Seq\":1," LOCAL(
		"0001-01-01T00:00:00Z") ",\"Peers\":["
					"{\"ID\":11,\"StableID\":\"nodeA\",\"Name\":\"home.example."
					"\",\"Key\":\"nodekey:" BOB_PUBLIC
					"\",\"Addresses\":[\"198.51.100.2/"
					"32\"],\"AllowedIPs\":[\"198.51.100.2/32\","
					"\"10.0.0.0/8\",\"10.1.0.0/16\",\"0.0.0.0/0\",\"::/"
					"0\"],\"CapMap\":{\"suggest-exit-node\":[]}},"
					"{\"ID\":12,\"StableID\":\"nodeB\",\"Key\":"
					"\"nodekey:" UNKNOWN_PUBLIC
					"\",\"Addresses\":[\"198.51.100.3/"
					"32\"],\"AllowedIPs\":[\"198.51.100.3/32\","
					"\"10.0.0.0/8\",\"0.0.0.0/0\",\"::/0\"],"
					"\"PrimaryRoutes\":[\"10.0.0.0/8\"]}]}";
	struct tailscale_routes prefs = {.accept_routes = true, .exit_node = "auto:any"};
	char selected[TAILSCALE_PEER_STABLE_ID_SIZE];

	zassert_ok(tailscale_routes_set(&prefs));
	start_policy();
	zassert_equal(route_owner("203.0.113.9"), 0);
	zassert_ok(apply(50, map));
	zassert_equal(route_owner("10.2.0.1"), 12);
	zassert_equal(route_owner("10.1.0.1"), 11);
	zassert_equal(route_owner("203.0.113.9"), 11);
	zassert_equal(route_owner("2001:db8::123"), 11);
	zassert_equal(route_owner("198.51.100.3"), 12);
	zassert_ok(tailscale_routes_get(&prefs, selected));
	zassert_str_equal(selected, "nodeA");
	/* An inconsistent pair of primary subnet owners cannot replace working routes. */
	const char *conflict = "{\"Seq\":2,\"PeersChangedPatch\":[{\"NodeID\":11,"
		"\"PrimaryRoutes\":[\"10.0.0.0/8\"]}]}";
	zassert_equal(apply(50, conflict), -EADDRINUSE);
	zassert_equal(route_owner("10.2.0.1"), 12);
	zassert_equal(route_owner("2001:db8::123"), 11);
	strcpy(prefs.exit_node, "198.51.100.3");
	prefs.accept_routes = false;
	zassert_ok(tailscale_routes_set(&prefs));
	zassert_equal(route_owner("2001:db8::123"), 12);
	/* The rejected frame requires a full map on a fresh stream. */
	zassert_ok(apply(51, map));
	zassert_ok(apply(51, "{\"Seq\":2,\"PeersRemoved\":[12]}"));
	zassert_equal(route_owner("203.0.113.9"), 0);
	zassert_equal(route_owner("198.51.100.2"), 11);
	zassert_ok(tailscale_routes_get(&prefs, selected));
	zassert_equal(selected[0], 0);
	zassert_ok(tailscale_client_policy_stop());
}
#endif
