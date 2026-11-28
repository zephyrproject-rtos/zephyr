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

#define KEY "nodekey:0101010101010101010101010101010101010101010101010101010101010101"
#define PEER "{\"ID\":1,\"Key\":\"" KEY "\",\"Addresses\":[\"198.51.100.2/32\"]}"
#define RULE(port, proto) \
	"{\"SrcIPs\":[\"*\"],\"DstPorts\":[{\"IP\":\"*\",\"Ports\":{\"First\":" #port \
	",\"Last\":" #port "}}],\"IPProto\":[" #proto "]}"
#define UDP(port) RULE(port, 17)
#define NAME_32 "01234567890123456789012345678901"

static struct tailscale_policy policy;
static const struct tailscale_packet_filter *installed;
static int install_error;
static size_t install_attempts;

static int install(const struct tailscale_peer_snapshot *peers,
		   const struct tailscale_packet_filter *filter, void *context)
{
	ARG_UNUSED(peers);
	ARG_UNUSED(context);
	install_attempts++;
	if (install_error != 0) {
		return install_error;
	}
	installed = filter;
	return 0;
}

static int apply(uint32_t generation, const char *body, enum tailscale_peer_inventory_frame *effect)
{
	return tailscale_policy_apply(&policy, generation, (uint8_t *)body, strlen(body), install,
				      NULL, effect);
}

static void assert_published(uint32_t generation, uint64_t revision)
{
	struct tailscale_peer_inventory_summary summary;

	zassert_ok(tailscale_policy_get_summary(&policy, &summary));
	zassert_true(summary.valid);
	zassert_equal(summary.generation, generation);
	zassert_equal(summary.revision, revision);
}

static void assert_ports(const uint16_t *ports, size_t count)
{
	zassert_not_null(installed);
	zassert_equal(installed->count, count);
	for (size_t i = 0; i < count; i++) {
		zassert_equal(installed->rules[i].destinations[0].first_port, ports[i]);
	}
}

static bool allows_port(uint16_t port, uint8_t protocol)
{
	uint8_t packet[40] = {0x45U};
	size_t length = protocol == NET_IPPROTO_TCP ? sizeof(packet) : 28U;

	packet[9] = protocol;
	sys_put_be16(length, packet + 2U);
	sys_put_be16(port, packet + 22U);
	if (protocol == NET_IPPROTO_TCP) {
		packet[32] = 0x50U;
	} else {
		sys_put_be16(8U, packet + 24U);
	}
	return tailscale_packet_filter_accept(installed, packet, length);
}

static void reset_policy(void *fixture)
{
	ARG_UNUSED(fixture);
	tailscale_policy_init(&policy);
	installed = NULL;
	install_error = 0;
	install_attempts = 0U;
}

ZTEST(tailscale_named_filter, test_precedence_and_stream_lifetime)
{
	enum tailscale_peer_inventory_frame effect;
	const uint16_t initial[] = {9001U, 9002U, 9003U, 9000U, 9004U};

	zassert_ok(apply(1U, "{\"Seq\":1,\"Peers\":[" PEER "],\"PacketFilter\":[" UDP(9000)
			    "],\"PacketFilters\":{\"z\":[" UDP(9004) "],\"a\":[" UDP(9002) ","
			    RULE(9003, 6) "],\"Base\":[" UDP(9001) "]}}", &effect));
	zassert_equal(effect, TAILSCALE_PEER_INVENTORY_FULL);
	assert_ports(initial, ARRAY_SIZE(initial));
	zassert_true(allows_port(9003U, NET_IPPROTO_TCP));
	zassert_false(allows_port(9003U, NET_IPPROTO_UDP));
	zassert_true(allows_port(9004U, NET_IPPROTO_UDP));
	zassert_false(allows_port(9004U, NET_IPPROTO_TCP));

	zassert_ok(apply(1U, "{\"Seq\":2,\"Peers\":[" PEER "],\"PacketFilters\":null}", &effect));
	zassert_equal(effect, TAILSCALE_PEER_INVENTORY_FULL);
	assert_ports(initial, ARRAY_SIZE(initial));

	zassert_ok(apply(1U, "{\"Seq\":3,\"PacketFilter\":[]}", &effect));
	zassert_equal(effect, TAILSCALE_PEER_INVENTORY_UPDATE);
	assert_ports((uint16_t[]){9001U, 9002U, 9003U, 9004U}, 4U);

	zassert_ok(apply(1U, "{\"Seq\":4,\"PacketFilter\":[" UDP(9010)
			    "],\"PacketFilters\":{\"base\":[" UDP(9011)
			    "],\"a\":[],\"z\":null}}", &effect));
	zassert_equal(effect, TAILSCALE_PEER_INVENTORY_UPDATE);
	assert_ports((uint16_t[]){9001U, 9011U}, 2U);

	zassert_ok(apply(1U, "{\"Seq\":5,\"PacketFilter\":[" UDP(9010)
			    "],\"PacketFilters\":{\"base\":null,\"Base\":[]}}", &effect));
	assert_ports(NULL, 0U);

	zassert_ok(apply(1U, "{\"Seq\":6,\"PacketFilters\":{\"z\":[" UDP(9012) "]}}", NULL));
	zassert_ok(apply(1U, "{\"Seq\":7,\"PacketFilter\":[" UDP(9013)
			    "],\"PacketFilters\":{\"a\":[" UDP(9014) "],\"*\":null}}", NULL));
	assert_ports((uint16_t[]){9014U}, 1U);
	zassert_ok(apply(1U, "{\"Seq\":8,\"PacketFilters\":{\"*\":[" UDP(9015) "],\"x\":null}}",
			 NULL));
	zassert_ok(apply(1U, "{\"Seq\":9,\"PacketFilters\":{}}", NULL));
	zassert_ok(apply(1U, "{\"Seq\":10,\"PacketFilter\":null,\"PacketFilters\":null}", &effect));
	zassert_equal(effect, TAILSCALE_PEER_INVENTORY_OTHER);
	assert_ports((uint16_t[]){9014U}, 1U);
	assert_published(1U, 9U);

	zassert_equal(apply(2U, "{\"Seq\":1,\"PacketFilters\":{\"a\":null}}", NULL), -EBADMSG);
	assert_published(1U, 9U);
	assert_ports((uint16_t[]){9014U}, 1U);
	zassert_ok(apply(3U, "{\"Seq\":1,\"Peers\":[]}", &effect));
	zassert_equal(effect, TAILSCALE_PEER_INVENTORY_FULL);
	assert_published(3U, 10U);
	assert_ports(NULL, 0U);
}

ZTEST(tailscale_named_filter, test_name_decoding_and_case)
{
	zassert_ok(apply(1U, "{\"Peers\":[],\"PacketFilter\":[" UDP(9000)
			    "],\"PacketFilters\":{\"\\u0062ase\":[" UDP(9001)
			    "],\"\\u00e9\":[" UDP(9002) "],\"\\u2e80\":[" UDP(9003)
			    "],\"\\ud83d\\ude00\":[" UDP(9004) "],\"\\u0000\":[" UDP(9005)
			    "],\"\":[" UDP(9006) "]}}", NULL));
	assert_ports((uint16_t[]){9006U, 9005U, 9001U, 9002U, 9003U, 9004U}, 6U);
	zassert_ok(apply(1U, "{\"PacketFilters\":{\"\xc3\xa9\":null,\""
			    "\xe2\xba\x80\":null,\"\xf0\x9f\x98\x80\":null}}", NULL));
	assert_ports((uint16_t[]){9006U, 9005U, 9001U}, 3U);
	zassert_equal(apply(1U, "{\"PacketFilters\":{\"base\\\\\":[" UDP(9007)
			       "],\"base\":\"bad\"}}", NULL), -EBADMSG);
	assert_ports((uint16_t[]){9006U, 9005U, 9001U}, 3U);

	zassert_ok(apply(2U, "{\"Peers\":[],\"PacketFilter\":[" UDP(9000)
			    "],\"PacketFilters\":{\"\\u002a\":null,\"base\\\\\":[" UDP(9008)
			    "],\"base\":[" UDP(9009) "]}}", NULL));
	assert_ports((uint16_t[]){9009U, 9008U}, 2U);
}

ZTEST(tailscale_named_filter, test_chunk_and_edit_limits_use_final_state)
{
	zassert_ok(apply(1U, "{\"Peers\":[],\"PacketFilters\":{\"a\":[],\"b\":[],\"c\":[],"
			    "\"d\":[],\"e\":[],\"f\":[],\"g\":[],\"h\":[]}}", NULL));
	zassert_ok(apply(1U, "{\"PacketFilters\":{\"a\":null,\"b\":null,\"c\":null,\"d\":null,"
			    "\"e\":null,\"f\":null,\"g\":null,\"h\":null,"
			    "\"i\":[" UDP(9001) "],\"j\":[],\"k\":[],\"l\":[],\"m\":[],"
			    "\"n\":[],\"o\":[],\"" NAME_32 "\":[" UDP(9000) "]}}", NULL));
	assert_ports((uint16_t[]){9000U, 9001U}, 2U);

	zassert_equal(apply(1U, "{\"PacketFilters\":{\"p\":[]}}", NULL), -E2BIG);
	assert_published(1U, 2U);
	assert_ports((uint16_t[]){9000U, 9001U}, 2U);
	zassert_equal(apply(2U, "{\"Peers\":[],\"PacketFilters\":{\"a\":null,\"b\":null,"
			    "\"c\":null,\"d\":null,\"e\":null,\"f\":null,\"g\":null,\"h\":null,"
			    "\"i\":null,\"j\":null,\"k\":null,\"l\":null,\"m\":null,\"n\":null,"
			    "\"o\":null,\"p\":null,\"q\":null}}", NULL), -E2BIG);
	assert_published(1U, 2U);
	zassert_equal(install_attempts, 2U);
}

ZTEST(tailscale_named_filter, test_rule_limit_and_installer_rollback)
{
	const uint16_t expected[] = {9000U, 9001U, 9002U, 9003U, 9004U, 9005U, 9006U, 9007U};

	zassert_ok(apply(1U, "{\"Peers\":[],\"PacketFilters\":{\"z\":[" UDP(9010) "," UDP(9011)
			    "],\"a\":[" UDP(9012) "]}}", NULL));
	zassert_ok(apply(1U, "{\"PacketFilters\":{\"a\":[" UDP(9000) "," UDP(9001) "," UDP(9002)
			    "," UDP(9003) "," UDP(9004) "," UDP(9005) "," UDP(9006) "," UDP(9007)
			    "],\"z\":null}}", NULL));
	assert_ports(expected, ARRAY_SIZE(expected));
	const struct tailscale_packet_filter *previous = installed;

	install_error = -ENOMEM;
	zassert_equal(apply(1U, "{\"PacketFilters\":{\"*\":null,\"x\":[" UDP(9010) "]}}", NULL),
		      -ENOMEM);
	zassert_equal_ptr(installed, previous);
	assert_published(1U, 2U);
	assert_ports(expected, ARRAY_SIZE(expected));
	zassert_equal(apply(1U, "{\"PacketFilters\":{\"x\":null}}", NULL), -ESTALE);
	zassert_equal(install_attempts, 3U);

	install_error = 0;
	zassert_ok(apply(2U, "{\"Peers\":[],\"PacketFilters\":{\"z\":[" UDP(9000) "," UDP(9001)
			    "," UDP(9002) "," UDP(9003) "," UDP(9004) "," UDP(9005) "," UDP(9006)
			    "," UDP(9007) "]}}", NULL));
	previous = installed;
	zassert_equal(apply(2U, "{\"PacketFilters\":{\"a\":[" UDP(9010) "]}}", NULL), -E2BIG);
	zassert_equal_ptr(installed, previous);
	assert_published(2U, 3U);
	assert_ports(expected, ARRAY_SIZE(expected));
	zassert_equal(install_attempts, 4U);
}

ZTEST(tailscale_named_filter, test_rejects_invalid_names_types_and_rules)
{
	static const struct {
		const char *json;
		int error;
	} bad[] = {
		{"{\"Peers\":[],\"PacketFilters\":[]}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"a\":{}}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"a\":[],\"a\":null}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"a\":[],\"\\u0061\":null}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"*\":[],\"\\u002a\":null}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"" NAME_32 "0\":null}}", -E2BIG},
		{"{\"Peers\":[],\"PacketFilters\":{\"\xc0\x80\":null}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"\\ud800\":null}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"\\ud800\\u0000\":null}}", -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"x\":[" RULE(9000, 1) "]}}", -ENOTSUP},
		{"{\"Peers\":[], "
		 "\"PacketFilters\":{\"x\":[{\"SrcIPs\":[\"*\"],\"DstPorts\":[],\"IPProto\":true}]}}",
		 -EBADMSG},
		{"{\"Peers\":[],\"PacketFilters\":{\"x\":[{\"SrcIPs\":[\"*\"],\"DstPorts\":[],"
		 "\"IPProto\":[17],\"SrcCaps\":[\"a\"]}]}}", -ENOTSUP},
	};

	zassert_ok(apply(1U, "{\"Peers\":[],\"PacketFilters\":{\"x\":[" UDP(9000) "]}}", NULL));
	for (size_t i = 0U; i < ARRAY_SIZE(bad); i++) {
		zassert_equal(apply(i + 2U, bad[i].json, NULL), bad[i].error, "frame %zu", i);
		assert_published(1U, 1U);
		assert_ports((uint16_t[]){9000U}, 1U);
	}
	zassert_equal(install_attempts, 1U);
}

ZTEST_SUITE(tailscale_named_filter, NULL, NULL, reset_policy, NULL, NULL);
