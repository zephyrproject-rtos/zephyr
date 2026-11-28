/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include "tailscale_internal.h"
#include "net_stack.h"
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "policy.h"
#include "socket.h"
#include "underlay_dns.h"

#define SELF_KEY "nodekey:0101010101010101010101010101010101010101010101010101010101010101"
#define PEER_KEY "nodekey:0202020202020202020202020202020202020202020202020202020202020202"
#define SELF                                                                                       \
	"{\"ID\":1,\"Key\":\"" SELF_KEY "\",\"Name\":\"unit.tail.example.\","                      \
	"\"Addresses\":[\"100.64.0.1/32\"]}"
#define PEER                                                                                       \
	"{\"ID\":2,\"Key\":\"" PEER_KEY "\",\"Name\":\"peer.tail.example.\","                      \
	"\"Addresses\":[\"100.64.0.2/32\",\"fd7a:115c:a1e0::2/128\"]}"
#define SERVER(ip, port) "{\"Addr\":\"" ip ":" STRINGIFY(port) "\"}"
#define DEFAULT_PORT     15353
#define SPLIT_PORT       15354
#define DEEP_PORT        15355
#define SILENT_PORT      15356
#define UNDERLAY_PORT    15357
#define UDP_HEADER_SIZE  12U
#define TIMEOUT_MS       1500

#define DEFAULT_SERVER SERVER("127.0.0.1", DEFAULT_PORT)
#define SPLIT_SERVER   SERVER("127.0.0.1", SPLIT_PORT)
#define DEEP_SERVER    SERVER("127.0.0.1", DEEP_PORT)
#define SILENT_SERVER  SERVER("127.0.0.1", SILENT_PORT)

static const char initial_map[] =
	"{\"Seq\":1,\"Node\":" SELF ",\"Peers\":[" PEER "],\"DNSConfig\":{"
	"\"Proxied\":true,\"Domains\":[\"tail.example\",\"corp.example\"],"
	"\"Resolvers\":[" DEFAULT_SERVER "],"
	"\"Routes\":{\"corp.example\":[" SPLIT_SERVER "],"
	"\"deep.corp.example\":[" DEEP_SERVER "],\"private.example\":[]}}}";
static const char split_only_map[] =
	"{\"Seq\":1,\"Node\":" SELF ",\"Peers\":[" PEER "],\"DNSConfig\":{"
	"\"Proxied\":true,\"Domains\":[\"tail.example\"],"
	"\"Routes\":{\"corp.example\":[" SPLIT_SERVER "]}}}";
static const char replacement_map[] =
	"{\"Seq\":2,\"DNSConfig\":{\"Resolvers\":[" SPLIT_SERVER "]}}";
static const char failover_map[] =
	"{\"Seq\":2,\"DNSConfig\":{\"Resolvers\":[" SILENT_SERVER "," DEFAULT_SERVER "]}}";

static struct tailscale_policy policy;
static atomic_t route_revision;
static atomic_t tunnel_available;
static bool dns_policy_ready;
static atomic_t dial_attempts;
static K_THREAD_STACK_DEFINE(responder_stack, 4096);
static struct k_thread responder;

static int install(const struct tailscale_peer_snapshot *peers,
		   const struct tailscale_packet_filter *filter, void *context)
{
	ARG_UNUSED(peers);
	ARG_UNUSED(filter);
	ARG_UNUSED(context);
	atomic_inc(&route_revision);
	return 0;
}

static int apply(const char *map)
{
	return tailscale_policy_apply(&policy, 1U, (uint8_t *)map, strlen(map), install, NULL,
				      NULL);
}

int tailscale_client_policy_summary(struct tailscale_peer_inventory_summary *summary)
{
	return tailscale_policy_get_summary(&policy, summary);
}

uint64_t tailscale_client_policy_route_revision(void)
{
	return atomic_get(&route_revision);
}

int tailscale_client_policy_dns_authority(const char *name)
{
	return tailscale_policy_dns_authority(&policy, name, dns_policy_ready);
}

bool tailscale_net_dns_offload_allowed(const char *host)
{
	return host == NULL || tailscale_client_policy_dns_authority(host) == -ENOTSUP;
}

int tailscale_client_policy_select_dns(const char *name, struct tailscale_dns_selection *selection)
{
	return tailscale_policy_select_dns(&policy, name, selection);
}

int tailscale_client_policy_dns_search_domain(uint64_t revision, size_t index,
				     char domain[TAILSCALE_DNS_NAME_SIZE])
{
	return tailscale_policy_dns_search_domain(&policy, revision, index, domain);
}

int tailscale_client_policy_resolve_magic_dns(const char *name, uint8_t family,
				     struct tailscale_dns_answer *answer)
{
	return tailscale_policy_resolve_magic_dns(&policy, name, family, answer);
}

/* Substitute a native loopback connection for the managed tunnel transport. */
int tailscale_socket_connect_dns(int type, const struct net_sockaddr *remote, net_socklen_t len,
			      uint64_t revision, int32_t timeout_ms)
{
	ARG_UNUSED(timeout_ms);
	atomic_inc(&dial_attempts);
	if (revision != tailscale_client_policy_route_revision()) {
		return -ESTALE;
	}
	if (!atomic_get(&tunnel_available)) {
		return -EHOSTUNREACH;
	}
	int fd = zsock_socket(remote->sa_family, type, 0);

	if (fd < 0) {
		return -errno;
	}
	if (zsock_connect(fd, remote, len) < 0) {
		int rc = -errno;

		zsock_close(fd);
		return rc;
	}
	return fd;
}

int tailscale_socket_close(int fd)
{
	return zsock_close(fd);
}

static int bind_responder(int type, uint16_t port)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(port),
		.sin_addr = {.s_addr = net_htonl(0x7f000001)},
	};
	int fd = zsock_socket(NET_AF_INET, type, 0);

	zassert_true(fd >= 0);
	zassert_ok(zsock_bind(fd, (struct net_sockaddr *)&addr, sizeof(addr)));
	return fd;
}

static void wait_input(int fd)
{
	struct zsock_pollfd pfd = {.fd = fd, .events = ZSOCK_POLLIN};

	zassert_equal(zsock_poll(&pfd, 1, TIMEOUT_MS + 500), 1);
	zassert_true(pfd.revents & ZSOCK_POLLIN);
}

static size_t read_tcp(int fd, uint8_t *data, size_t length)
{
	size_t done = 0U;

	while (done != length) {
		wait_input(fd);
		ssize_t received = zsock_recv(fd, data + done, length - done, ZSOCK_MSG_DONTWAIT);

		zassert_true(received > 0);
		done += received;
	}
	return done;
}

static void check_question(const uint8_t *query, size_t size, const char *name, uint16_t type)
{
	size_t offset = UDP_HEADER_SIZE;

	while (*name != '\0') {
		size_t length = strcspn(name, ".");

		zassert_true(offset + 1U + length < size);
		zassert_equal(query[offset++], length);
		zassert_mem_equal(query + offset, name, length);
		offset += length;
		name += length + (name[length] == '.');
	}
	zassert_equal(size, offset + 5U);
	zassert_equal(query[offset++], 0);
	zassert_equal(sys_get_be16(query + offset), type);
	zassert_equal(sys_get_be16(query + offset + 2U), 1);
}

static size_t rdata(uint8_t *response, size_t offset, uint16_t owner, uint16_t type,
		    const uint8_t *data, uint16_t length)
{
	sys_put_be16(0xc000U | owner, response + offset);
	sys_put_be16(type, response + offset + 2U);
	sys_put_be16(1, response + offset + 4U);
	sys_put_be32(60, response + offset + 6U);
	sys_put_be16(length, response + offset + 10U);
	memcpy(response + offset + 12U, data, length);
	return offset + 12U + length;
}

enum reply_behavior {
	DIRECT,
	TRUNCATE,
	ALIAS_LOCAL,
	REPLACE,
};

struct dns_test_server {
	int udp;
	int tcp;
	const char *name;
	uint16_t type;
	enum reply_behavior behavior;
};

static size_t answer(struct dns_test_server *server, uint8_t *packet, size_t length)
{
	static const uint8_t ip4[] = {192, 0, 2, 9};
	static const uint8_t ip6[] = {0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9};
	const uint8_t *address = server->type == 1U ? ip4 : ip6;
	uint16_t width = server->type == 1U ? sizeof(ip4) : sizeof(ip6);

	sys_put_be16(0x8180U, packet + 2U);
	if (server->behavior == ALIAS_LOCAL || server->behavior == TRUNCATE) {
		const uint8_t local[] = "\004peer\004tail\007example";
		const uint8_t canonical[] = "\006target\007example";
		const uint8_t *alias = server->behavior == ALIAS_LOCAL ? local : canonical;
		uint16_t alias_size =
			server->behavior == ALIAS_LOCAL ? sizeof(local) : sizeof(canonical);
		uint16_t target_start = length + 12U;

		sys_put_be16(2U, packet + 6U);
		length = rdata(packet, length, UDP_HEADER_SIZE, 5U, alias, alias_size);
		return rdata(packet, length, target_start, server->type, address, width);
	}
	sys_put_be16(1U, packet + 6U);
	return rdata(packet, length, UDP_HEADER_SIZE, server->type, address, width);
}

static void respond(void *arg, void *unused1, void *unused2)
{
	struct dns_test_server *server = arg;
	struct net_sockaddr_in client;
	net_socklen_t client_len = sizeof(client);
	uint8_t frame[514];
	uint8_t *packet = frame + 2U;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	wait_input(server->udp);
	ssize_t received =
		zsock_recvfrom(server->udp, packet, sizeof(frame) - 2U, ZSOCK_MSG_DONTWAIT,
			       (struct net_sockaddr *)&client, &client_len);

	zassert_true(received > UDP_HEADER_SIZE);
	check_question(packet, received, server->name, server->type);
	if (server->behavior == TRUNCATE) {
		sys_put_be16(0x8380U, packet + 2U);
		zassert_equal(zsock_sendto(server->udp, packet, received, 0,
					   (struct net_sockaddr *)&client, client_len),
			      received);
		wait_input(server->tcp);
		int fd = zsock_accept(server->tcp, NULL, NULL);

		zassert_true(fd >= 0);
		read_tcp(fd, frame, 2U);
		size_t length = sys_get_be16(frame);

		zassert_true(length <= sizeof(frame) - 2U);
		read_tcp(fd, packet, length);
		check_question(packet, length, server->name, server->type);
		length = answer(server, packet, length);
		sys_put_be16(length, frame);
		zassert_equal(zsock_send(fd, frame, length + 2U, 0), length + 2U);
		zassert_ok(zsock_close(fd));
		return;
	}
	if (server->behavior == REPLACE) {
		zassert_ok(apply(replacement_map));
	}
	size_t length = answer(server, packet, received);

	if (server->behavior == DIRECT) {
		packet[0] ^= 1U;
		zassert_equal(zsock_sendto(server->udp, packet, length, 0,
					   (struct net_sockaddr *)&client, client_len),
			      length);
		packet[0] ^= 1U;
	}
	zassert_equal(zsock_sendto(server->udp, packet, length, 0, (struct net_sockaddr *)&client,
				   client_len),
		      length);
}

static int lookup(const char *name, net_sa_family_t family, uint16_t port, const char *expected,
		  enum reply_behavior behavior, struct tailscale_dns_result *result)
{
	struct dns_test_server server = {
		.udp = bind_responder(NET_SOCK_DGRAM, port),
		.tcp = -1,
		.name = expected,
		.type = family == NET_AF_INET ? 1U : 28U,
		.behavior = behavior,
	};

	if (behavior == TRUNCATE) {
		server.tcp = bind_responder(NET_SOCK_STREAM, port);
		zassert_ok(zsock_listen(server.tcp, 1));
	}
	k_thread_create(&responder, responder_stack, K_THREAD_STACK_SIZEOF(responder_stack),
			respond, &server, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	int rc = tailscale_dns_resolve(name, family, TIMEOUT_MS, result);

	zassert_ok(k_thread_join(&responder, K_SECONDS(2)));
	if (server.tcp >= 0) {
		zassert_ok(zsock_close(server.tcp));
	}
	zassert_ok(zsock_close(server.udp));
	return rc;
}

static void expect_address(const struct tailscale_dns_result *result, net_sa_family_t family,
			   const char *ip)
{
	uint8_t binary[16];

	zassert_equal(result->address_count, 1U);
	zassert_equal(result->addresses[0].ss_family, family);
	zassert_ok(net_addr_pton(family, ip, binary));
	if (family == NET_AF_INET) {
		const struct net_sockaddr_in *address = net_sin(net_sad(&result->addresses[0]));

		zassert_equal(address->sin_port, 0);
		zassert_mem_equal(&address->sin_addr, binary, sizeof(address->sin_addr));
	} else {
		const struct net_sockaddr_in6 *address = net_sin6(net_sad(&result->addresses[0]));

		zassert_equal(address->sin6_port, 0);
		zassert_mem_equal(&address->sin6_addr, binary, sizeof(address->sin6_addr));
	}
}

static void reset(void *fixture)
{
	ARG_UNUSED(fixture);
	tailscale_policy_init(&policy);
	atomic_set(&route_revision, 0);
	atomic_set(&tunnel_available, 1);
	atomic_set(&dial_attempts, 0);
	dns_policy_ready = true;
}

ZTEST(tailscale_dns, test_local_authority_and_search)
{
	struct tailscale_dns_result result;

	zassert_equal(tailscale_dns_resolve("peer", NET_AF_INET, TIMEOUT_MS, &result), -ENETDOWN);
	zassert_ok(apply(initial_map));
	zassert_ok(tailscale_dns_resolve("PEER", NET_AF_INET6, TIMEOUT_MS, &result));
	expect_address(&result, NET_AF_INET6, "fd7a:115c:a1e0::2");
	zassert_equal(
		tailscale_dns_resolve("missing.private.example", NET_AF_INET, TIMEOUT_MS, &result),
		-ENOENT);
	zassert_equal(atomic_get(&dial_attempts), 0);
}

ZTEST(tailscale_dns, test_split_default_and_qualified_search)
{
	struct tailscale_dns_result result;

	zassert_ok(apply(initial_map));
	zassert_ok(lookup("db.Deep.Corp.EXAMPLE.", NET_AF_INET6, DEEP_PORT, "db.deep.corp.example",
			  DIRECT, &result));
	expect_address(&result, NET_AF_INET6, "2001:db8::9");
	zassert_ok(lookup("db", NET_AF_INET, SPLIT_PORT, "db.corp.example", DIRECT, &result));
	expect_address(&result, NET_AF_INET, "192.0.2.9");
	zassert_ok(lookup("db.", NET_AF_INET, DEFAULT_PORT, "db", DIRECT, &result));
	expect_address(&result, NET_AF_INET, "192.0.2.9");
}

ZTEST(tailscale_dns, test_tcp_fallback_and_cname)
{
	struct tailscale_dns_result result;

	zassert_ok(apply(initial_map));
	zassert_ok(lookup("alias.example", NET_AF_INET, DEFAULT_PORT, "alias.example", TRUNCATE,
			  &result));
	expect_address(&result, NET_AF_INET, "192.0.2.9");
	zassert_ok(lookup("alias.example", NET_AF_INET, DEFAULT_PORT, "alias.example", ALIAS_LOCAL,
			  &result));
	expect_address(&result, NET_AF_INET, "100.64.0.2");
}

ZTEST(tailscale_dns, test_reject_stale_reply_and_unavailable_tunnel)
{
	struct tailscale_dns_result result = {.revision = 42U};

	zassert_ok(apply(initial_map));
	zassert_equal(
		lookup("host.example", NET_AF_INET, DEFAULT_PORT, "host.example", REPLACE, &result),
		-ESTALE);
	zassert_equal(result.revision, 42U);
	zassert_ok(
		lookup("host.example", NET_AF_INET, SPLIT_PORT, "host.example", DIRECT, &result));
	expect_address(&result, NET_AF_INET, "192.0.2.9");
	zassert_equal(result.revision, 2U);
	atomic_set(&tunnel_available, 0);
	zassert_equal(tailscale_dns_resolve("host.example", NET_AF_INET, TIMEOUT_MS, &result),
		      -EHOSTUNREACH);
	zassert_equal(result.revision, 2U);
}

ZTEST(tailscale_dns, test_resolver_failover)
{
	struct tailscale_dns_result result;
	uint8_t query[512];
	int silent = bind_responder(NET_SOCK_DGRAM, SILENT_PORT);

	zassert_ok(apply(initial_map));
	zassert_ok(apply(failover_map));
	zassert_ok(
		lookup("host.example", NET_AF_INET, DEFAULT_PORT, "host.example", DIRECT, &result));
	ssize_t length = zsock_recv(silent, query, sizeof(query), ZSOCK_MSG_DONTWAIT);

	zassert_true(length > UDP_HEADER_SIZE);
	check_question(query, length, "host.example", 1U);
	expect_address(&result, NET_AF_INET, "192.0.2.9");
	zassert_ok(zsock_close(silent));
}

ZTEST(tailscale_dns, test_native_dns_fallback_during_policy_outage)
{
	dns_policy_ready = false;
	zassert_equal(tailscale_net_query("public.example.org", NET_AF_INET, NULL, NULL), -ENOTSUP);
	zassert_equal(tailscale_net_query("node.tailnet.ts.net.", NET_AF_INET, NULL, NULL),
		      DNS_EAI_AGAIN);
	zassert_equal(tailscale_net_query("peer", NET_AF_INET, NULL, NULL), DNS_EAI_AGAIN);

	zassert_ok(apply(split_only_map));
	zassert_equal(tailscale_net_query("public.example.org", NET_AF_INET, NULL, NULL), -ENOTSUP);
	zassert_equal(tailscale_net_query("peer.tail.example", NET_AF_INET, NULL, NULL),
		      DNS_EAI_AGAIN);
	zassert_equal(tailscale_net_query("db.corp.example", NET_AF_INET, NULL, NULL),
		      DNS_EAI_AGAIN);
	zassert_equal(tailscale_net_query("peer", NET_AF_INET, NULL, NULL), DNS_EAI_AGAIN);

	zassert_ok(apply(replacement_map));
	zassert_equal(tailscale_net_query("public.example.org", NET_AF_INET, NULL, NULL),
		      DNS_EAI_AGAIN);
	zassert_equal(atomic_get(&dial_attempts), 0);
}

ZTEST(tailscale_dns, test_underlay_resolves_during_overlay_outage)
{
	const char *servers[2];
	const char *host = "control.corp.example";
	const struct net_in_addr loopback = {.s_addr = net_htonl(0x7f000001U)};
	struct net_if *underlay = NULL;
	struct net_sockaddr_storage address;
	struct dns_test_server server = {
		.udp = bind_responder(NET_SOCK_DGRAM, UNDERLAY_PORT),
		.name = host,
		.behavior = DIRECT,
	};
	const uint16_t port = 443;
	struct net_in_addr v4;
	struct net_in6_addr v6;

	zassert_not_null(net_if_ipv4_addr_lookup(&loopback, &underlay));
	zassert_not_null(underlay);
	dns_policy_ready = false;
	zassert_ok(apply(split_only_map));
	zassert_equal(tailscale_net_query(host, NET_AF_INET, NULL, NULL), DNS_EAI_AGAIN);
	servers[0] = "127.0.0.1:" STRINGIFY(UNDERLAY_PORT);
	servers[1] = NULL;
	zassert_ok(dns_resolve_reconfigure(dns_resolve_get_default(), servers, NULL,
					   DNS_SOURCE_MANUAL));

	server.type = DNS_QUERY_TYPE_A;
	k_thread_create(&responder, responder_stack, K_THREAD_STACK_SIZEOF(responder_stack),
			respond, &server, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	int count = tailscale_underlay_resolve(underlay, host, NET_AF_INET, port, &address, 1);

	zassert_ok(k_thread_join(&responder, K_SECONDS(2)));
	zassert_equal(count, 1);
	zassert_equal(address.ss_family, NET_AF_INET);
	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.9", &v4));
	zassert_true(net_ipv4_addr_cmp(&net_sin(net_sad(&address))->sin_addr, &v4));
	zassert_equal(net_sin(net_sad(&address))->sin_port, net_htons(port));

	server.type = DNS_QUERY_TYPE_AAAA;
	k_thread_create(&responder, responder_stack, K_THREAD_STACK_SIZEOF(responder_stack),
			respond, &server, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	count = tailscale_underlay_resolve(underlay, host, NET_AF_INET6, port, &address, 1);

	zassert_ok(k_thread_join(&responder, K_SECONDS(2)));
	zassert_equal(count, 1);
	zassert_equal(address.ss_family, NET_AF_INET6);
	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8::9", &v6));
	zassert_true(net_ipv6_addr_cmp(&net_sin6(net_sad(&address))->sin6_addr, &v6));
	zassert_equal(net_sin6(net_sad(&address))->sin6_port, net_htons(port));
	zassert_ok(dns_resolve_close(dns_resolve_get_default()));
	zassert_ok(zsock_close(server.udp));
	zassert_equal(atomic_get(&dial_attempts), 0);
}

static struct net_sockaddr_in offload_address = {
	.sin_family = NET_AF_INET,
	.sin_port = net_htons(443),
	.sin_addr = {.s_addr = net_htonl(0xc000020a)},
};
static struct zsock_addrinfo offload_result = {
	.ai_family = NET_AF_INET,
	.ai_addr = (struct net_sockaddr *)&offload_address,
	.ai_addrlen = sizeof(offload_address),
};
static bool offload_released;

static int offload_lookup(const char *host, const char *service, const struct zsock_addrinfo *hints,
			  struct zsock_addrinfo **result)
{
	zassert_mem_equal(host, "relay.example", sizeof("relay.example"));
	zassert_mem_equal(service, "443", sizeof("443"));
	zassert_equal(hints->ai_family, NET_AF_INET);
	*result = &offload_result;
	return 0;
}

static void offload_free(struct zsock_addrinfo *result)
{
	zassert_equal_ptr(result, &offload_result);
	offload_released = true;
}

ZTEST(tailscale_dns, test_underlay_uses_configured_dns_offload)
{
	static const struct socket_dns_offload ops = {
		.getaddrinfo = offload_lookup,
		.freeaddrinfo = offload_free,
	};
	struct net_sockaddr_storage address;

	offload_released = false;
	socket_offload_dns_register(&ops);
	int count = tailscale_underlay_resolve(net_if_get_default(), "relay.example",
					      NET_AF_INET, 443, &address, 1);

	zassert_ok(socket_offload_dns_deregister(&ops));
	zassert_equal(count, 1);
	zassert_true(offload_released);
	zassert_mem_equal(&address, &offload_address, sizeof(offload_address));
}

ZTEST(tailscale_dns, test_native_getaddrinfo_respects_managed_scope_with_offload)
{
	static const struct socket_dns_offload ops = {
		.getaddrinfo = offload_lookup,
		.freeaddrinfo = offload_free,
	};
	const struct zsock_addrinfo hints = {.ai_family = NET_AF_INET};
	struct zsock_addrinfo *result;
	struct net_in_addr expected;

	offload_released = false;
	socket_offload_dns_register(&ops);
	zassert_ok(apply(split_only_map));
	zassert_ok(zsock_getaddrinfo("peer", "443", &hints, &result));
	zassert_ok(net_addr_pton(NET_AF_INET, "100.64.0.2", &expected));
	zassert_equal(result->ai_family, NET_AF_INET);
	zassert_true(net_ipv4_addr_cmp(&net_sin(result->ai_addr)->sin_addr, &expected));
	zsock_freeaddrinfo(result);
	zassert_false(offload_released);

	dns_policy_ready = false;
	zassert_equal(zsock_getaddrinfo("db.corp.example", "443", &hints, &result), DNS_EAI_AGAIN);
	zassert_is_null(result);
	zassert_false(offload_released);

	zassert_ok(zsock_getaddrinfo("relay.example", "443", &hints, &result));
	zassert_ok(socket_offload_dns_deregister(&ops));
	zassert_true(offload_released);
	zassert_mem_equal(result->ai_addr, &offload_address, sizeof(offload_address));
	zsock_freeaddrinfo(result);
}

ZTEST_SUITE(tailscale_dns, NULL, NULL, reset, NULL, NULL);
