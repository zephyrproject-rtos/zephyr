/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/net_ip.h>
#include "tailscale_internal.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define KEY1 "nodekey:0101010101010101010101010101010101010101010101010101010101010101"
#define KEY2 "nodekey:0202020202020202020202020202020202020202020202020202020202020202"
#define KEY3 "nodekey:0303030303030303030303030303030303030303030303030303030303030303"
#define KEY4 "nodekey:0404040404040404040404040404040404040404040404040404040404040404"
#define KEY5 "nodekey:0505050505050505050505050505050505050505050505050505050505050505"
#define KEY6 "nodekey:0606060606060606060606060606060606060606060606060606060606060606"
#define PEER1 "{\"ID\":1,\"Key\":\"" KEY1 "\",\"Addresses\":[\"198.51.100.2/32\"]}"
#define PEER2 "{\"ID\":2,\"Key\":\"" KEY2 "\",\"Addresses\":[\"198.51.100.3/32\"]}"
#define PEER3 "{\"ID\":3,\"Key\":\"" KEY4 "\",\"Addresses\":[\"198.51.100.4/32\"]}"
#define PEER4 "{\"ID\":4,\"Key\":\"" KEY5 "\",\"Addresses\":[\"198.51.100.5/32\"]}"
#define PEER5 "{\"ID\":5,\"Key\":\"" KEY6 "\",\"Addresses\":[\"198.51.100.6/32\"]}"
#define SELF "{\"ID\":9,\"Key\":\"" KEY3 "\",\"Addresses\":[\"198.51.100.1/32\"]}"
#define RULE4 "{\"SrcIPs\":[\"198.51.100.0/24\"],\"DstPorts\":[{" \
	      "\"IP\":\"198.51.100.1\",\"Ports\":{\"First\":9000,\"Last\":9000}}],\"IPProto\":[17]}"
#define RULE6 "{\"SrcIPs\":[\"fd7a:115c:a1e0:1::/73\"],\"DstPorts\":[{" \
	      "\"IP\":\"fd7a:115c:a1e0:1::1\",\"Ports\":{\"First\":9000,\"Last\":9001}}]," \
	      "\"IPProto\":[6,17]}"
#define INITIAL "{\"Seq\":1,\"Node\":" SELF ",\"Peers\":[" PEER1 "," PEER2 "]," \
		"\"PacketFilter\":[" RULE4 "]}"

static struct tailscale_policy policy;
static struct {
	const struct tailscale_packet_filter *filter;
	const struct tailscale_peer_snapshot *peers;
	uint64_t ids[TAILSCALE_PEER_INVENTORY_CAPACITY];
	uint64_t self_id;
	size_t count;
	int reject;
	unsigned int attempts;
} installed;

static int install(const struct tailscale_peer_snapshot *peers,
		   const struct tailscale_packet_filter *filter, void *context)
{
	zassert_equal_ptr(context, &installed);
	installed.attempts++;
	if (installed.reject != 0) {
		return installed.reject;
	}
	installed.filter = filter;
	installed.peers = peers;
	installed.count = peers->count;
	installed.self_id = peers->have_self ? peers->self.id : 0U;
	for (size_t i = 0; i < peers->count; i++) {
		installed.ids[i] = peers->peers[i].id;
		zassert_equal(peers->peers[i].allowed_ip_count, 1U);
		zassert_mem_equal(peers->peers[i].addresses, peers->peers[i].allowed_ips,
				  sizeof(peers->peers[i].addresses[0]));
	}
	return 0;
}

static int apply(uint32_t generation, const char *json, enum tailscale_peer_inventory_frame *frame)
{
	return tailscale_policy_apply(&policy, generation, (uint8_t *)json, strlen(json),
				      install, &installed, frame);
}

static struct tailscale_peer_inventory_summary summary(void)
{
	struct tailscale_peer_inventory_summary current;

	zassert_ok(tailscale_policy_get_summary(&policy, &current));
	return current;
}

static void reset_policy(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(&installed, 0, sizeof(installed));
	tailscale_policy_init(&policy);
}

static void ipv4_udp(uint8_t packet[32], uint8_t ihl)
{
	memset(packet, 0, 32U);
	packet[0] = 0x40U | ihl;
	packet[9] = NET_IPPROTO_UDP;
	memcpy(packet + 12U, (uint8_t[]){198U, 51U, 100U, 2U}, 4U);
	memcpy(packet + 16U, (uint8_t[]){198U, 51U, 100U, 1U}, 4U);
	sys_put_be16(ihl * 4U + 8U, packet + 2U);
	sys_put_be16(9000U, packet + ihl * 4U + 2U);
	sys_put_be16(8U, packet + ihl * 4U + 4U);
}

ZTEST(tailscale_policy, test_hosted_initial_changes_and_protocol_default)
{
	static const char frame[] =
		"{\"Node\":" SELF ",\"PeersChanged\":[" PEER1 "],"
		"\"SSHPolicy\":{\"rules\":[{}]},\"PacketFilter\":[{"
		"\"SrcIPs\":[\"100.115.94.0-100.127.255.255\","
		"\"100.64.0.0-100.115.91.255\",\"fd7a:115c:a1e0::/48\"],"
		"\"DstPorts\":[{\"IP\":\"*\",\"Ports\":{\"First\":0,\"Last\":65535}}]}]}";
	uint8_t address[16] = {100, 115, 91, 255};
	struct tailscale_filter_tuple flow = {
		.family = NET_AF_INET,
		.protocol = NET_IPPROTO_UDP,
		.source = address,
		.destination = address,
		.destination_port = 9000,
	};

	zassert_ok(apply(1, frame, NULL));
	zassert_equal(summary().peer_count, 1);
	zassert_true(tailscale_packet_filter_match(installed.filter, &flow));
	address[2] = 92;
	zassert_false(tailscale_packet_filter_match(installed.filter, &flow));
	address[2] = 94;
	zassert_true(tailscale_packet_filter_match(installed.filter, &flow));
	zassert_ok(net_addr_pton(NET_AF_INET6, "fd7a:115c:a1e0::103b:df75", address));
	flow.family = NET_AF_INET6;
	flow.protocol = NET_IPPROTO_TCP;
	zassert_true(tailscale_packet_filter_match(installed.filter, &flow));
	flow.destination_port = 22;
	zassert_false(tailscale_packet_filter_match(installed.filter, &flow));
	zassert_ok(apply(1, "{\"PeersChanged\":[" PEER2 "]}", NULL));
	zassert_false(tailscale_packet_filter_match(installed.filter, &flow));
	zassert_equal(apply(1, "{\"SSHPolicy\":{\"unknown\":true}}", NULL), -ENOTSUP);
	zassert_equal(summary().revision, 2);
	zassert_false(tailscale_packet_filter_match(installed.filter, &flow));
}

ZTEST(tailscale_policy, test_host_caps_are_exact_and_unknown_requirements_retain_policy)
{
	zassert_ok(apply(1,
			 "{\"Node\":{\"ID\":9,\"Key\":\"" KEY3 "\","
			 "\"Addresses\":[\"198.51.100.1/32\"],\"CapMap\":{\"https\":null}}}",
			 NULL));
	zassert_equal(apply(1,
			    "{\"Node\":{\"ID\":9,\"Key\":\"" KEY3 "\","
			    "\"CapMap\":{\"HTTPS\":null}}}",
			    NULL),
		      -ENOTSUP);
	zassert_equal(summary().revision, 1);
	zassert_equal(apply(2, "{\"TKAInfo\":{}}", NULL), -EKEYREJECTED);
	zassert_equal(summary().generation, 1);
}

ZTEST(tailscale_policy, test_atomic_rejection_and_stream_resync)
{
	enum tailscale_peer_inventory_frame frame;
	uint8_t packet[32];

	ipv4_udp(packet, 5U);
	zassert_false(summary().valid);
	zassert_ok(apply(1U, INITIAL, &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_FULL);
	zassert_equal(installed.self_id, 9U);
	zassert_equal(installed.attempts, 1U);
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, sizeof(packet)));
	zassert_ok(apply(1U, "{\"KeepAlive\":true}", &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_KEEPALIVE);
	zassert_equal(summary().revision, 1U);
	zassert_equal(installed.attempts, 1U);

	const struct tailscale_packet_filter *original = installed.filter;

	installed.reject = -ENOMEM;
	zassert_equal(apply(1U, "{\"Seq\":2,\"PeersRemoved\":[1],\"PacketFilter\":[]}", NULL),
		      -ENOMEM);
	zassert_equal(installed.attempts, 2U);
	zassert_equal_ptr(installed.filter, original);
	zassert_true(tailscale_packet_filter_accept(original, packet, sizeof(packet)));
	zassert_equal(summary().generation, 1U);
	zassert_equal(summary().revision, 1U);
	zassert_equal(summary().peer_count, 2U);
	zassert_equal(installed.ids[0], 1U);
	zassert_equal(apply(1U, "{\"Seq\":3,\"PeersRemoved\":[2]}", NULL), -ESTALE);
	zassert_equal(apply(2U, "{\"Seq\":1,\"PacketFilter\":[]}", NULL), -EBADMSG);
	zassert_equal(summary().generation, 1U);
	zassert_equal(summary().revision, 1U);

	installed.reject = 0;
	zassert_ok(apply(3U, "{\"Seq\":1,\"Peers\":[" PEER2 "]}", &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_FULL);
	zassert_equal(summary().generation, 3U);
	zassert_equal(summary().revision, 2U);
	zassert_equal(summary().peer_count, 1U);
	zassert_equal(installed.ids[0], 2U);
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, sizeof(packet)));
	zassert_equal(installed.self_id, 0U);
	zassert_ok(apply(3U, "{\"Seq\":2,\"PeersRemoved\":[2]}", NULL));
	zassert_equal(summary().peer_count, 0U);
	zassert_ok(apply(3U, "{\"Seq\":3,\"PeersChanged\":[" PEER1 "]}", NULL));
	zassert_equal(summary().peer_count, 1U);
}

ZTEST(tailscale_policy, test_filter_and_peer_deltas)
{
	uint8_t packet[32];
	enum tailscale_peer_inventory_frame frame;

	ipv4_udp(packet, 5U);
	zassert_ok(apply(1U, INITIAL, NULL));
	zassert_ok(apply(1U, "{\"Seq\":2,\"Peers\":[],\"PeersRemoved\":[1],\"PacketFilter\":null}",
			 &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_UPDATE);
	zassert_equal(installed.self_id, 9U);
	zassert_equal(installed.count, 1U);
	zassert_equal(installed.ids[0], 2U);
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, sizeof(packet)));
	zassert_ok(apply(1U, "{\"Seq\":3,\"PacketFilter\":[]}", &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_UPDATE);
	zassert_equal(installed.count, 1U);
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, sizeof(packet)));
	zassert_ok(apply(1U, "{\"Seq\":4,\"Node\":" SELF ",\"PacketFilter\":[" RULE4 "]}", &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_UPDATE);
	zassert_equal(installed.attempts, 4U);
	zassert_equal(summary().revision, 4U);
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, sizeof(packet)));
	zassert_equal(apply(1U, "{\"Seq\":6,\"PacketFilter\":[]}", NULL), -EILSEQ);
	zassert_equal(summary().revision, 4U);
}

ZTEST(tailscale_policy, test_complete_peer_update_inherits_same_stream_policy)
{
	uint8_t packet[32];
	enum tailscale_peer_inventory_frame frame;

	ipv4_udp(packet, 5U);
	zassert_ok(apply(1U, INITIAL, NULL));
	zassert_ok(apply(1U, "{\"Seq\":2,\"Peers\":[" PEER1 "," PEER2 "," PEER3 "," PEER4 "]}",
			 &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_FULL);
	zassert_equal(installed.self_id, 9U);
	zassert_equal(summary().peer_count, 4U);
	zassert_equal(summary().revision, 2U);
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, sizeof(packet)));
	zassert_equal(apply(1U, "{\"Seq\":3,\"PacketFilter\":[],\"PeersChanged\":[" PEER5 "]}",
			    NULL), -E2BIG);
	zassert_equal(summary().peer_count, 4U);
	zassert_equal(summary().revision, 2U);
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, sizeof(packet)));
}

ZTEST(tailscale_policy, test_headscale_map_disables_log_upload)
{
	zassert_ok(apply(1U, "{\"Node\":" SELF ",\"Peers\":[" PEER1 "],"
			     "\"PacketFilters\":{\"base\":[" RULE4 "]},"
			     "\"Debug\":{\"DisableLogTail\":true}}", NULL));
	zassert_true(summary().valid);
	zassert_equal(summary().peer_count, 1U);
}

ZTEST(tailscale_policy, test_reject_invalid_or_unsupported_maps)
{
	static const struct {
		const char *frame;
		int error;
	} bad[] = {
		{"{\"Peers\":[],\"PacketFilter\":[],\"packetfilter\":[]}", -EBADMSG},
		{"{\"Peers\":[],\"Debug\":{\"DisableLogTail\":\"true\"}}", -EBADMSG},
		{"{\"Peers\":[],\"Debug\":{\"DisableLogTail\":true,\"SleepSeconds\":1}}", -ENOTSUP},
		{"{\"Peers\":[],\"DNSConfig\":{\"Proxied\":true}}", -ENOTSUP},
		{"{\"Peers\":[],\"TKAInfo\":{}}", -EKEYREJECTED},
		{"{\"Peers\":[" PEER1 "," PEER1 "]}", -EBADMSG},
		{"{\"Peers\":[" PEER1 ",{\"ID\":2,\"Key\":\"" KEY1 "\","
		 "\"Addresses\":[\"198.51.100.3/32\"]}]}", -EBADMSG},
		{"{\"Peers\":[" PEER1 ",{\"ID\":2,\"Key\":\"" KEY2 "\","
		 "\"Addresses\":[\"198.51.100.3/32\"],\"AllowedIPs\":[\"198.51.100.2/32\"]}]}",
		 -EBADMSG},
		{"{\"Peers\":[{\"ID\":1,\"Key\":\"" KEY1 "\","
		 "\"Addresses\":[\"198.51.100.2/24\"]}]}", -EBADMSG},
		{"{\"Peers\":[{\"ID\":1,\"Key\":\"" KEY1 "\","
		 "\"Addresses\":[\"198.51.100.2/32\"],\"UnknownTrustField\":true}]}", -ENOTSUP},
		{"{\"Node\":{\"ID\":9,\"Key\":\"" KEY3 "\",\"Expired\":true,"
		 "\"Addresses\":[\"198.51.100.1/32\"]}}", -EKEYREJECTED},
		{"{\"Node\":{\"ID\":9,\"Key\":\"" KEY3 "\","
		 "\"KeyExpiry\":\"2100-02-29T11:12:00Z\","
		 "\"Addresses\":[\"198.51.100.1/32\"]}}", -EBADMSG},
		{"{\"Node\":{\"ID\":9,\"Key\":\"" KEY3 "\","
		 "\"KeyExpiry\":\"2026-11-25T11:12:00+24:00\","
		 "\"Addresses\":[\"198.51.100.1/32\"]}}", -EBADMSG},
		{"{\"Node\":{\"ID\":9,\"Key\":\"" KEY3 "\","
		 "\"KeyExpiry\":\"2026-11-25T11:12:00.1234567899Z\","
		 "\"Addresses\":[\"198.51.100.1/32\"]}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilter\":[{\"SrcIPs\":[\"*\"],"
		 "\"DstPorts\":[],\"IPProto\":[1]}]}",
		 -ENOTSUP},
		{"{\"Peers\":[],\"PacketFilter\":[{\"SrcIPs\":[\"*\"],\"DstPorts\":[{\"IP\":\"*\","
		 "\"Ports\":{\"First\":9000,\"first\":0,\"Last\":9000}}],\"IPProto\":[17]}]}",
		 -EBADMSG},
		{"{\"Peers\":[],\"PacketFilter\":[{\"SrcIPs\":[\"*\"],\"DstPorts\":[{\"IP\":\"*\","
		 "\"Ports\":{\"First\":9001,\"Last\":9000}}],\"IPProto\":[17]}]}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilter\":[" RULE4 "," RULE4 "," RULE4 "," RULE4 ","
		 RULE4 "," RULE4 "," RULE4 "," RULE4 "," RULE4 "]}", -E2BIG},
	};

	zassert_ok(apply(1U, INITIAL, NULL));
	for (size_t i = 0; i < ARRAY_SIZE(bad); i++) {
		zassert_equal(apply(i + 2U, bad[i].frame, NULL), bad[i].error, "frame %zu", i);
		zassert_equal(summary().generation, 1U);
		zassert_equal(summary().revision, 1U);
		zassert_equal(installed.attempts, 1U);
	}
}

ZTEST(tailscale_policy, test_verified_credential_timestamps)
{
	const char *control_time_map =
		"{\"KeepAlive\":true,\"ControlTime\":\"2038-01-19T03:14:07Z\"}";
	const char *offset = "{\"Seq\":2,\"PeersChangedPatch\":[{\"NodeID\":1,"
		"\"KeyExpiry\":\"2038-01-19T04:14:08.123456789+01:00\"}]}";
	const char *zero = "{\"Seq\":3,\"PeersChangedPatch\":[{\"NodeID\":1,"
		"\"KeyExpiry\":\"0001-01-01T01:00:00+01:00\"}]}";
	enum tailscale_peer_inventory_frame frame;
	int64_t anchor;
	unsigned int attempts;

	zassert_ok(apply(1U, control_time_map, &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_KEEPALIVE);
	zassert_equal(installed.attempts, 0U);
	zassert_ok(apply(1U, INITIAL, NULL));
	zassert_true(installed.peers->have_control_time);
	anchor = installed.peers->control_epoch_at_boot_ms;
	zassert_ok(apply(1U, offset, NULL));
	zassert_true(installed.peers->peers[0].key_expires);
	zassert_equal(installed.peers->peers[0].key_expiry_ms, INT64_C(2147483648123));
	zassert_false(installed.peers->peers[1].key_expires);
	zassert_ok(apply(1U, zero, NULL));
	zassert_false(installed.peers->peers[0].key_expires);

	attempts = installed.attempts;
	zassert_ok(apply(1U, "{\"KeepAlive\":true,\"ControlTime\":\"2030-01-01T00:00:00Z\"}",
			 &frame));
	zassert_equal(installed.attempts, attempts);
	zassert_equal(installed.peers->control_epoch_at_boot_ms, anchor);
	zassert_ok(apply(1U, "{\"ControlTime\":\"2038-01-19T03:14:09Z\"}", &frame));
	zassert_equal(frame, TAILSCALE_PEER_INVENTORY_OTHER);
	zassert_equal(installed.attempts, attempts + 1U);
	zassert_equal(summary().revision, 3U);
	zassert_true(installed.peers->control_epoch_at_boot_ms > anchor);
	anchor = installed.peers->control_epoch_at_boot_ms;

	zassert_equal(apply(1U, "{\"Seq\":4,\"ControlTime\":\"2050-01-01T00:00:00Z\","
				 "\"PeersChangedPatch\":[{\"NodeID\":1,\"KeyExpiry\":\"\"}]}",
			    NULL), -EBADMSG);
	zassert_equal(summary().revision, 3U);
	zassert_equal(installed.peers->control_epoch_at_boot_ms, anchor);
	zassert_ok(apply(2U, INITIAL, NULL));
	zassert_equal(installed.peers->control_epoch_at_boot_ms, anchor);

	installed.reject = -EBUSY;
	zassert_equal(apply(2U, "{\"KeepAlive\":true,\"ControlTime\":\"2050-01-01T00:00:00Z\"}",
			    NULL), -EBUSY);
	zassert_equal(installed.peers->control_epoch_at_boot_ms, anchor);
	installed.reject = 0;
	zassert_ok(apply(3U, INITIAL, NULL));
	zassert_equal(installed.peers->control_epoch_at_boot_ms, anchor);
}

ZTEST(tailscale_policy, test_ipv4_uses_ip_and_udp_declared_lengths)
{
	uint8_t packet[32];

	zassert_ok(apply(1U, INITIAL, NULL));
	ipv4_udp(packet, 5U);
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, 32U));
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 27U));
	packet[12] = 192U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 28U));
	ipv4_udp(packet, 5U);
	packet[19] = 3U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 28U));
	ipv4_udp(packet, 5U);
	sys_put_be16(9001U, packet + 22U);
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 28U));
	ipv4_udp(packet, 5U);
	sys_put_be16(9U, packet + 24U);
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 32U));
	ipv4_udp(packet, 5U);
	packet[6] = 0x20U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 28U));
	packet[6] = 0U;
	packet[7] = 2U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 28U));
	ipv4_udp(packet, 6U);
	packet[20] = 1U;
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, 32U));
	packet[20] = 131U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 32U));
	packet[0] = 0x44U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 32U));
}

ZTEST(tailscale_policy, test_ipv6_filters_tcp_udp_and_denies_extensions)
{
	uint8_t packet[64] = {0x60};

	zassert_ok(apply(1U, "{\"Peers\":[],\"PacketFilter\":[" RULE6 "]}", NULL));
	zassert_ok(net_addr_pton(NET_AF_INET6, "fd7a:115c:a1e0:1::2", packet + 8U));
	zassert_ok(net_addr_pton(NET_AF_INET6, "fd7a:115c:a1e0:1::1", packet + 24U));
	sys_put_be16(8U, packet + 4U);
	packet[6] = NET_IPPROTO_UDP;
	sys_put_be16(9001U, packet + 42U);
	sys_put_be16(8U, packet + 44U);
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, 48U));
	packet[17] = 0x80U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 48U));
	packet[17] = 0U;
	packet[6] = NET_IPV6_NEXTHDR_FRAG;
	packet[40] = NET_IPPROTO_UDP;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 48U));
	packet[6] = NET_IPV6_NEXTHDR_DESTO;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 48U));
	packet[6] = NET_IPPROTO_TCP;
	sys_put_be16(20U, packet + 4U);
	packet[52] = 0x50U;
	packet[53] = 0x10U;
	zassert_true(tailscale_packet_filter_accept(installed.filter, packet, 60U));
	sys_put_be16(8999U, packet + 42U);
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 60U));
	sys_put_be16(9000U, packet + 42U);
	packet[52] = 0x60U;
	zassert_false(tailscale_packet_filter_accept(installed.filter, packet, 64U));
}

ZTEST_SUITE(tailscale_policy, NULL, NULL, reset_policy, NULL, NULL);

#ifndef CONFIG_TAILSCALE_DERP_MAP
ZTEST(tailscale_policy, test_direct_policy_ignores_relay_metadata)
{
	static const char relay_map[] =
		"{\"Seq\":1, \"Node\":" SELF ", \"Peers\":[" PEER1 "], "
		"\"DERPMap\":{\"Regions\":{\"1\":{\"RegionID\":1, \"Nodes\":[]}}}}";
	static const char bad_map[] = "{\"Seq\":3, \"DERPMap\":[]}";
	struct tailscale_peer_inventory_summary before;

	zassert_ok(apply(1, relay_map, NULL));
	before = summary();
	zassert_equal(before.peer_count, 1);
	zassert_ok(apply(1, "{\"Seq\":2, \"DERPMap\":{\"Regions\":{}}}", NULL));
	zassert_equal(apply(1, bad_map, NULL), -EBADMSG);
	zassert_equal(summary().revision, before.revision);
	zassert_equal(installed.ids[0], 1);
}
#endif

#ifdef CONFIG_TAILSCALE_DERP_MAP
#if CONFIG_TAILSCALE_DERP_MAX_NODES >= 96
ZTEST(tailscale_policy, test_hosted_scale_derp_map)
{
	static char body[16384];
	static const char node[] =
		"{\"RegionID\":%u,\"HostName\":\"relay-%u.example\",\"IPv4\":\"192.0.2.1\","
		"\"IPv6\":\"2001:db8::1\"}";
	size_t used = snprintk(body, sizeof(body), "{\"Seq\":2,\"DERPMap\":{\"Regions\":{");

	zassert_ok(apply(1, INITIAL, NULL));
	for (unsigned int region = 1U; region <= 32U; region++) {
		const char *separator = region == 1U ? "" : ",";
		int n = snprintk(body + used, sizeof(body) - used,
				 "%s\"%u\":{\"RegionID\":%u,\"Nodes\":[", separator, region,
				 region);

		zassert_true(n > 0 && n < sizeof(body) - used);
		used += n;
		for (unsigned int index = 0U; index < 3U; index++) {
			n = snprintk(body + used, sizeof(body) - used, node, region,
				     (region - 1U) * 3U + index);
			zassert_true(n > 0 && n < sizeof(body) - used - 3U);
			used += n;
			body[used++] = index == 2U ? ']' : ',';
		}
		body[used++] = '}';
	}
	zassert_true(sizeof(body) - used >= sizeof("}}}"));
	memcpy(body + used, "}}}", sizeof("}}}"));

	zassert_ok(apply(1, body, NULL));
	zassert_equal(installed.peers->derp.count, 96U);
	zassert_equal(installed.peers->derp.nodes[95].region, 32U);
	zassert_str_equal(installed.peers->derp.nodes[95].host, "relay-95.example");
	zassert_equal(installed.count, 2U);
}
#endif

ZTEST(tailscale_policy, test_derp_stun_port)
{
	char frame[256];
	const int ports[] = {-1, 0, 3900, 65536};
	const uint16_t expected[] = {0, 3478, 3900};

	for (size_t i = 0; i < ARRAY_SIZE(ports); i++) {
		snprintk(frame, sizeof(frame), "{\"Peers\":[],\"DERPMap\":{\"Regions\":{\"1\":{"
			 "\"RegionID\":1,\"Nodes\":[{\"RegionID\":1,\"HostName\":\"relay.example\","
			 "\"STUNPort\":%d}]}}}}", ports[i]);
		int rc = apply(i + 1, frame, NULL);

		if (i < ARRAY_SIZE(expected)) {
			zassert_ok(rc);
			zassert_equal(installed.peers->derp.nodes[0].stun_port, expected[i]);
		} else {
			zassert_equal(rc, -EBADMSG);
			zassert_equal(installed.peers->derp.nodes[0].stun_port, 3900);
		}
	}
}

ZTEST(tailscale_policy, test_derp_map_replacement_and_rejection)
{
	static const char node[] =
		"{\"RegionID\":901,\"HostName\":\"relay.example\",\"IPv4\":\"192.0.2.10\","
		"\"IPv6\":\"none\",\"DERPPort\":443}";
	char body[768];

	zassert_ok(apply(1, INITIAL, NULL));
	snprintk(body, sizeof(body),
		 "{\"Seq\":2,\"DERPMap\":{\"omitDefaultRegions\":true,"
		 "\"HomeParams\":{\"RegionScore\":{\"901\":0.5}},\"Regions\":{\"901\":{"
		 "\"RegionID\":901,\"NoMeasureNoHome\":true,\"Nodes\":[%s]}}}}", node);
	zassert_ok(apply(1, body, NULL));
	const struct tailscale_derp_map *map = &installed.peers->derp;

	zassert_equal(map->count, 1);
	zassert_equal(map->nodes[0].region, 901);
	zassert_equal(map->nodes[0].stun_port, 3478);
	zassert_true(map->nodes[0].no_home);
	zassert_str_equal(map->nodes[0].host, "relay.example");
	zassert_ok(apply(1, "{\"Seq\":3,\"DERPMap\":{\"Regions\":null}}", NULL));
	zassert_equal(installed.peers->derp.count, 1);
	const uint64_t revision = summary().revision;

	if (CONFIG_TAILSCALE_DERP_MAX_NODES == 2) {
		snprintk(body, sizeof(body), "{\"Seq\":4,\"DERPMap\":{\"Regions\":{\"901\":{"
			 "\"RegionID\":901,\"Nodes\":[%s,%s,%s]}}}}", node, node, node);
		zassert_equal(apply(1, body, NULL), -ENOSPC);
		zassert_equal(summary().revision, revision);
		zassert_equal(installed.peers->derp.count, 1);
		zassert_equal(installed.count, 2);
	}
	zassert_equal(apply(2, "{\"Node\":" SELF ",\"DERPMap\":{\"Regions\":{\"1\":{"
			    "\"RegionID\":2,\"Nodes\":[]}}}}", NULL), -EBADMSG);
	zassert_equal(apply(3, "{\"Node\":" SELF ",\"DERPMap\":{\"Regions\":{\"1\":{"
			    "\"RegionID\":1,\"Nodes\":[{\"RegionID\":1,"
			    "\"HostName\":\"a\",\"InsecureForTests\":true}]}}}}", NULL), -ENOTSUP);
	zassert_equal(summary().revision, revision);
	zassert_ok(apply(4, INITIAL, NULL));
	zassert_equal(installed.peers->derp.count, 0);
}
#endif
