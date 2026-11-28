/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/net_ip.h>
#include "tailscale_internal.h"
#include <zephyr/ztest.h>

#define SELF_KEY "nodekey:0101010101010101010101010101010101010101010101010101010101010101"
#define PEER_KEY "nodekey:0202020202020202020202020202020202020202020202020202020202020202"
#define SELF "{\"ID\":1,\"Key\":\"" SELF_KEY "\",\"Name\":\"node.Tail.Example.\"," \
	     "\"Addresses\":[\"100.64.0.1/32\"]}"
#define NAMED_PEER(name, fields) \
	"{\"ID\":2,\"Key\":\"" PEER_KEY "\",\"Name\":\"" name "\",\"Addresses\":[" \
	"\"100.64.0.2/32\",\"fd7a:115c:a1e0::2/128\"]" fields "}"
#define PEER NAMED_PEER("peer.shared.example.", "")
#define SERVER(address) "{\"Addr\":\"" address "\"}"
#define FULL_DNS "{\"Proxied\":true,\"Resolvers\":[" SERVER("192.0.2.53") "]," \
		 "\"Routes\":{\"corp.Example.\":[" SERVER("100.64.0.2") "," \
		 SERVER("[fd7a:115c:a1e0::2]:5353") "],\"private.corp.example\":[]," \
		 "\"dev.private.corp.example\":[" SERVER("100.64.0.3:5354") "]}}"
#define INITIAL "{\"Seq\":1,\"Node\":" SELF ",\"Peers\":[" PEER "],\"DNSConfig\":" FULL_DNS "}"
#define REPLACEMENT "{\"Seq\":2,\"PeersRemoved\":[2],\"PacketFilter\":[],\"DNSConfig\":{}}"

static struct tailscale_policy policy;
static struct {
	const struct tailscale_packet_filter *filter;
	size_t peer_count;
	unsigned int attempts;
	int error;
	bool pause;
} installed;

static K_SEM_DEFINE(install_ready, 0, 1);
static K_SEM_DEFINE(install_release, 0, 1);
static K_SEM_DEFINE(reader_ready, 0, 1);
static K_SEM_DEFINE(reader_done, 0, 1);
static K_THREAD_STACK_DEFINE(installer_stack, CONFIG_ZTEST_STACK_SIZE);
static K_THREAD_STACK_DEFINE(reader_stack, CONFIG_ZTEST_STACK_SIZE);
static struct k_thread installer;
static struct k_thread reader;

static int install(const struct tailscale_peer_snapshot *peers,
		   const struct tailscale_packet_filter *filter, void *context)
{
	ARG_UNUSED(context);

	installed.attempts++;
	if (installed.pause) {
		k_sem_give(&install_ready);
		zassert_ok(k_sem_take(&install_release, K_SECONDS(1)));
	}
	if (installed.error != 0) {
		return installed.error;
	}
	installed.filter = filter;
	installed.peer_count = peers->count;
	return 0;
}

static int apply(uint32_t generation, const char *json)
{
	return tailscale_policy_apply(&policy, generation, (uint8_t *)json, strlen(json),
				      install, NULL, NULL);
}

static void reset(void *fixture)
{
	ARG_UNUSED(fixture);

	tailscale_policy_init(&policy);
	memset(&installed, 0, sizeof(installed));
}

static struct tailscale_dns_selection select_dns(const char *name, uint32_t generation,
						uint64_t revision, enum tailscale_dns_source source)
{
	struct tailscale_dns_selection selection;

	zassert_ok(tailscale_policy_select_dns(&policy, name, &selection));
	zassert_equal(selection.generation, generation);
	zassert_equal(selection.revision, revision);
	zassert_equal(selection.source, source, "Name %s", name);
	return selection;
}

static void expect_server(const struct tailscale_dns_server *server, uint8_t family,
			  const char *address, uint16_t port)
{
	uint8_t bytes[16] = {0};

	zassert_ok(net_addr_pton(family, address, bytes));
	zassert_equal(server->family, family);
	zassert_equal(server->port, port);
	zassert_mem_equal(server->address, bytes, sizeof(bytes));
}

ZTEST(tailscale_dns_policy, test_effective_absolute_dns_selection)
{
	struct tailscale_dns_selection dns;

	zassert_equal(tailscale_policy_select_dns(&policy, "public.example", &dns), -ENODATA);
	zassert_ok(apply(1U, INITIAL));
	dns = select_dns("_dns.Host.CoRP.EXAMPLE.", 1U, 1U, TAILSCALE_DNS_SPLIT);
	zassert_equal(dns.server_count, 2U);
	expect_server(&dns.servers[0], NET_AF_INET, "100.64.0.2", 53U);
	expect_server(&dns.servers[1], NET_AF_INET6, "fd7a:115c:a1e0::2", 5353U);
	dns = select_dns("dev.private.corp.example", 1U, 1U, TAILSCALE_DNS_SPLIT);
	zassert_equal(dns.server_count, 1U);
	expect_server(&dns.servers[0], NET_AF_INET, "100.64.0.3", 5354U);
	dns = select_dns("private.corp.example", 1U, 1U, TAILSCALE_DNS_LOCAL);
	zassert_equal(dns.server_count, 0U);
	dns = select_dns("another.Tail.Example", 1U, 1U, TAILSCALE_DNS_LOCAL);
	zassert_equal(dns.server_count, 0U);
	dns = select_dns("2.0.64.100.in-addr.arpa.", 1U, 1U, TAILSCALE_DNS_LOCAL);
	zassert_equal(dns.server_count, 0U);
	dns = select_dns("0.e.1.a.c.5.1.1.a.7.d.f.ip6.arpa.", 1U, 1U, TAILSCALE_DNS_LOCAL);
	zassert_equal(dns.server_count, 0U);
	dns = select_dns("2.0.63.100.in-addr.arpa.", 1U, 1U, TAILSCALE_DNS_DEFAULT);
	zassert_equal(dns.server_count, 1U);
	dns = select_dns("notcorp.example", 1U, 1U, TAILSCALE_DNS_DEFAULT);
	zassert_equal(dns.server_count, 1U);
	expect_server(&dns.servers[0], NET_AF_INET, "192.0.2.53", 53U);
	zassert_equal(tailscale_policy_select_dns(&policy, "broken..example", &dns), -EINVAL);
}

ZTEST(tailscale_dns_policy, test_stream_inheritance_and_explicit_clear)
{
	zassert_ok(apply(1U, INITIAL));
	zassert_ok(apply(1U, "{\"Seq\":2,\"Peers\":[" PEER "]}"));
	(void)select_dns("tail.example", 1U, 2U, TAILSCALE_DNS_LOCAL);
	zassert_ok(apply(1U, "{\"Seq\":3,\"PacketFilter\":[],\"DNSConfig\":null}"));
	(void)select_dns("a.corp.example", 1U, 3U, TAILSCALE_DNS_SPLIT);
	zassert_ok(apply(1U, "{\"KeepAlive\":true,\"DNSConfig\":{}}"));
	(void)select_dns("public.example", 1U, 3U, TAILSCALE_DNS_DEFAULT);
	zassert_ok(apply(1U, "{\"Seq\":4,\"DNSConfig\":{}}"));
	(void)select_dns("a.corp.example", 1U, 4U, TAILSCALE_DNS_NONE);
	zassert_ok(apply(1U, "{\"Seq\":5,\"DNSConfig\":{\"Proxied\":true}}"));
	(void)select_dns("tail.example", 1U, 5U, TAILSCALE_DNS_LOCAL);
	zassert_ok(apply(2U, "{\"Seq\":1,\"Node\":" SELF ",\"Peers\":[]}"));
	(void)select_dns("tail.example", 2U, 6U, TAILSCALE_DNS_NONE);
}

ZTEST(tailscale_dns_policy, test_magic_dns_uses_active_node_addresses_only)
{
	struct tailscale_dns_answer answer;
	uint8_t ipv6[16];

	zassert_equal(tailscale_policy_resolve_magic_dns(&policy, "peer.shared.example",
							NET_AF_INET, &answer), -ENODATA);
	zassert_ok(apply(1U, INITIAL));
	(void)select_dns("peer.shared.example.", 1U, 1U, TAILSCALE_DNS_LOCAL);
	zassert_ok(tailscale_policy_resolve_magic_dns(&policy, "NODE.tail.example.", NET_AF_INET,
						    &answer));
	zassert_equal(answer.generation, 1U);
	zassert_equal(answer.revision, 1U);
	zassert_true(answer.self);
	zassert_equal(answer.address_count, 1U);
	zassert_mem_equal(answer.addresses[0].address, ((uint8_t[]){100U, 64U, 0U, 1U}), 4U);
	zassert_equal(tailscale_policy_resolve_magic_dns(&policy, "node.tail.example", NET_AF_INET6,
							&answer), -ENOENT);
	zassert_ok(tailscale_policy_resolve_magic_dns(&policy, "PEER.shared.example.", NET_AF_INET6,
						    &answer));
	zassert_false(answer.self);
	zassert_equal(answer.address_count, 1U);
	zassert_ok(net_addr_pton(NET_AF_INET6, "fd7a:115c:a1e0::2", ipv6));
	zassert_mem_equal(answer.addresses[0].address, ipv6, sizeof(ipv6));
	zassert_equal(tailscale_policy_resolve_magic_dns(&policy, "public.example", NET_AF_INET,
							&answer), -ENOENT);
	zassert_ok(apply(1U, "{\"Seq\":2,\"PeersChanged\":["
			   NAMED_PEER("NODE.tail.example", "") "]}"));
	zassert_equal(tailscale_policy_resolve_magic_dns(&policy, "node.tail.example", NET_AF_INET,
							&answer), -EADDRINUSE);
	zassert_ok(apply(1U, "{\"Seq\":3,\"PeersChanged\":["
			   NAMED_PEER("peer.shared.example", ",\"Expired\":true") "]}"));
	zassert_equal(tailscale_policy_resolve_magic_dns(&policy, "peer.shared.example",
							NET_AF_INET6, &answer), -ENOENT);
	zassert_ok(apply(1U, "{\"Seq\":4,\"DNSConfig\":{}}"));
	zassert_equal(tailscale_policy_resolve_magic_dns(&policy, "node.tail.example", NET_AF_INET,
							&answer), -ENOTSUP);
}

ZTEST(tailscale_dns_policy, test_host_search_list_does_not_expand_absolute_names)
{
	struct tailscale_dns_answer answer;

	zassert_ok(apply(1U, INITIAL));
	zassert_ok(apply(1U, "{\"Seq\":2,\"DNSConfig\":{\"Proxied\":true,"
			     "\"Domains\":[\"elsewhere.test\",\"tail.example.\"],"
			     "\"CertDomains\":[\"node.tail.example\"],"
			     "\"ExitNodeFilteredSet\":[\".example.test\"]}}"));
	zassert_ok(tailscale_policy_resolve_magic_dns(&policy, "node", NET_AF_INET, &answer));
	zassert_equal(answer.revision, 2);
	zassert_equal(tailscale_policy_resolve_magic_dns(&policy, "node.", NET_AF_INET, &answer),
		      -ENOENT);
	zassert_equal(apply(1U, "{\"Seq\":3,\"DNSConfig\":{\"Domains\":[\"bad..test\"]}}"),
		      -EBADMSG);
	zassert_ok(tailscale_policy_resolve_magic_dns(&policy, "node", NET_AF_INET, &answer));
	zassert_equal(answer.revision, 2);
}

ZTEST(tailscale_dns_policy, test_rejected_candidates_retain_effective_dns)
{
	zassert_ok(apply(1U, INITIAL));
	const struct tailscale_packet_filter *previous = installed.filter;

	installed.error = -ENOMEM;
	zassert_equal(apply(1U, REPLACEMENT), -ENOMEM);
	zassert_equal(installed.attempts, 2U);
	zassert_equal(installed.peer_count, 1U);
	zassert_equal_ptr(installed.filter, previous);
	(void)select_dns("a.corp.example", 1U, 1U, TAILSCALE_DNS_SPLIT);
	installed.error = 0;

	static const struct {
		const char *config;
		int error;
	} invalid[] = {
		{"{\"Resolvers\":[" SERVER("https://r.example/dns-query") "]}", -ENOTSUP},
		{"{\"Resolvers\":[" SERVER("100.64.0.2") "," SERVER("100.64.0.3") ","
		 SERVER("100.64.0.4") "," SERVER("100.64.0.5") "," SERVER("100.64.0.6") "]}",
		 -E2BIG},
		{"{\"Resolvers\":[" SERVER("12345::1") "]}", -EBADMSG},
		{"{\"Resolvers\":[" SERVER("[1:2:3:4:5:6:7:8:9]:53") "]}", -EBADMSG},
		{"{\"Routes\":{\"corp.Example\":[],\"CORP.example.\":[]}}", -EBADMSG},
		{"{\"Routes\":{\"bad..example\":[]}}", -EBADMSG},
		{"{\"Resolvers\":[{\"Addr\":\"192.0.2.53\",\"UseWithExitNode\":true}]}", -ENOTSUP},
		{"{\"Resolvers\":[{\"Addr\":\"192.0.2.53\",\"NewRequiredField\":true}]}", -ENOTSUP},
		{"[]", -EBADMSG},
	};
	for (size_t i = 0; i < ARRAY_SIZE(invalid); i++) {
		char body[512];
		int len = snprintk(body, sizeof(body), "{\"Seq\":1,\"Node\":%s,\"PacketFilter\":[],"
				   "\"DNSConfig\":%s}", SELF, invalid[i].config);

		zassert_true(len > 0 && len < sizeof(body));
		zassert_equal(apply(i + 2U, body), invalid[i].error, "Case %zu", i);
		zassert_equal(installed.attempts, 2U);
		zassert_equal_ptr(installed.filter, previous);
		(void)select_dns("a.corp.example", 1U, 1U, TAILSCALE_DNS_SPLIT);
	}
}

static void make_reverse_routes(char *body, size_t capacity, unsigned int sequence, size_t count)
{
	size_t used =
		snprintk(body, capacity, "{\"Seq\":%u, \"DNSConfig\":{\"Routes\":{", sequence);

	for (size_t i = 0; i < count; i++) {
		const char *separator = i == 0U ? "" : ", ";
		int length = snprintk(body + used, capacity - used,
				      "%s\"%zu.100.in-addr.arpa\":[]", separator, 64U + i);

		zassert_true(length > 0 && length < capacity - used);
		used += length;
	}
	zassert_true(capacity - used >= sizeof("}}}"));
	strcpy(body + used, "}}}");
}

ZTEST(tailscale_dns_policy, test_headscale_reverse_routes_fit)
{
	static char body[4096];

	zassert_ok(apply(1U, INITIAL));
	make_reverse_routes(body, sizeof(body), 2U, 64U);
	zassert_ok(apply(1U, body));
	(void)select_dns("1.127.100.in-addr.arpa", 1U, 2U, TAILSCALE_DNS_LOCAL);

	/* Rejected replacements leave the accepted DNS policy installed. */
	make_reverse_routes(body, sizeof(body), 3U, TAILSCALE_DNS_MAX_ROUTES + 1U);
	zassert_equal(apply(1U, body), -E2BIG);
	(void)select_dns("1.127.100.in-addr.arpa", 1U, 2U, TAILSCALE_DNS_LOCAL);
}

static void update_policy(void *result, void *unused2, void *unused3)
{
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	*(int *)result = apply(1U, REPLACEMENT);
}

static void read_selection(void *result, void *unused2, void *unused3)
{
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	k_sem_give(&reader_ready);
	*(struct tailscale_dns_selection *)result =
		select_dns("corp.example", 1U, 2U, TAILSCALE_DNS_NONE);
	k_sem_give(&reader_done);
}

ZTEST(tailscale_dns_policy, test_selection_waits_for_installation)
{
	struct tailscale_dns_selection selection;
	int result = -EBUSY;

	zassert_ok(apply(1U, INITIAL));
	installed.pause = true;
	k_thread_create(&installer, installer_stack, K_THREAD_STACK_SIZEOF(installer_stack),
			update_policy, &result, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&install_ready, K_SECONDS(1)));
	k_thread_create(&reader, reader_stack, K_THREAD_STACK_SIZEOF(reader_stack),
			read_selection, &selection, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&reader_ready, K_SECONDS(1)));
	zassert_equal(k_sem_take(&reader_done, K_MSEC(1)), -EAGAIN);
	k_sem_give(&install_release);
	zassert_ok(k_thread_join(&installer, K_SECONDS(1)));
	zassert_ok(k_thread_join(&reader, K_SECONDS(1)));
	zassert_ok(k_sem_take(&reader_done, K_NO_WAIT));
	zassert_ok(result);
	zassert_equal(installed.peer_count, 0U);
	zassert_equal(selection.server_count, 0U);
}

ZTEST_SUITE(tailscale_dns_policy, NULL, NULL, reset, NULL, NULL);
