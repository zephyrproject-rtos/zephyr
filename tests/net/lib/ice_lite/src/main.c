/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/net/ice_lite.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/stun.h>
#include <zephyr/ztest.h>

#define PEER_UFRAG "peer"

/* A comprehension-required attribute nobody understands. */
#define ATTR_UNKNOWN 0x7f00U

/* Room for a check with the longest credentials the Kconfig range allows. */
#define CHECK_SIZE 512U

#define CHECK_USE_CANDIDATE BIT(0)
#define CHECK_CONTROLLED    BIT(1)
#define CHECK_NO_USERNAME   BIT(2)
#define CHECK_NO_INTEGRITY  BIT(3)
#define CHECK_UNKNOWN_ATTR  BIT(4)
#define CHECK_NO_PRIORITY   BIT(5)
#define CHECK_NO_ROLE       BIT(6)

static const uint8_t txid[STUN_TXID_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

struct ice_lite_fixture {
	struct ice_lite_agent agent;
};

static void *setup(void)
{
	static struct ice_lite_fixture fixture;

	return &fixture;
}

static void before(void *data)
{
	struct ice_lite_fixture *fixture = data;

	zassert_ok(ice_lite_init(&fixture->agent));
}

static void peer_addr(struct net_sockaddr_storage *addr)
{
	struct net_sockaddr_in *addr4 = net_sin(net_sad(addr));

	memset(addr, 0, sizeof(*addr));
	addr4->sin_family = NET_AF_INET;
	addr4->sin_port = net_htons(40000);
	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.7", &addr4->sin_addr));
}

/* Build the connectivity check of a full agent. */
static size_t build_check(uint8_t *buf, size_t size, const char *ufrag, const char *pwd,
			  uint32_t flags)
{
	struct stun_builder builder;
	char username[ICE_LITE_UFRAG_LEN + 1U + sizeof(PEER_UFRAG)];
	int len;

	zassert_ok(stun_builder_init(&builder, buf, size, STUN_BINDING_REQUEST, txid));

	if ((flags & CHECK_NO_USERNAME) == 0U) {
		snprintf(username, sizeof(username), "%s:" PEER_UFRAG, ufrag);
		zassert_ok(stun_add_username(&builder, username));
	}

	if ((flags & CHECK_NO_PRIORITY) == 0U) {
		zassert_ok(stun_add_priority(&builder, ICE_LITE_HOST_PRIORITY(65535U, 1U)));
	}

	if ((flags & CHECK_NO_ROLE) != 0U) {
		/* no role attribute */
	} else if ((flags & CHECK_CONTROLLED) != 0U) {
		zassert_ok(stun_add_ice_controlled(&builder, 42U));
	} else {
		zassert_ok(stun_add_ice_controlling(&builder, 42U));
	}

	if ((flags & CHECK_USE_CANDIDATE) != 0U) {
		zassert_ok(stun_add_use_candidate(&builder));
	}

	if ((flags & CHECK_UNKNOWN_ATTR) != 0U) {
		zassert_ok(stun_add_attr(&builder, ATTR_UNKNOWN, NULL, 0U));
	}

	if ((flags & CHECK_NO_INTEGRITY) != 0U) {
		len = stun_finish_plain(&builder);
	} else {
		len = stun_finish(&builder, (const uint8_t *)pwd, strlen(pwd));
	}
	zassert_true(len > 0, "finish failed (%d)", len);

	return (size_t)len;
}

/* Pass a check to the agent and parse what it answers. */
static void exchange(struct ice_lite_agent *agent, const char *ufrag, const char *pwd,
		     uint32_t flags, uint8_t *resp, struct stun_msg *msg)
{
	struct net_sockaddr_storage src;
	uint8_t check[CHECK_SIZE];
	size_t len;
	int ret;

	peer_addr(&src);
	len = build_check(check, sizeof(check), ufrag, pwd, flags);

	ret = ice_lite_input(agent, check, len, net_sad(&src), resp, ICE_LITE_RESPONSE_SIZE);
	zassert_true(ret > 0, "no response (%d)", ret);

	zassert_ok(stun_parse(resp, (size_t)ret, msg));
	zassert_mem_equal(msg->txid, txid, sizeof(txid));
	zassert_ok(stun_verify_fingerprint(msg));
}

static void expect_error(struct ice_lite_agent *agent, const char *ufrag, const char *pwd,
			 uint32_t flags, uint16_t code, bool is_signed)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct stun_msg msg;

	exchange(agent, ufrag, pwd, flags | CHECK_USE_CANDIDATE, resp, &msg);

	zassert_equal(msg.type, STUN_BINDING_ERROR_RESPONSE);
	zassert_true(msg.has_error_code);
	zassert_equal(msg.error_code, code);
	zassert_false(agent->nominated);

	if (is_signed) {
		zassert_ok(
			stun_verify_integrity(&msg, (const uint8_t *)agent->pwd, ICE_LITE_PWD_LEN));
	} else {
		zassert_false(msg.has_integrity);
	}
}

ZTEST_F(ice_lite, test_credentials)
{
	static const char ice_chars[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	struct ice_lite_agent other;

	zassert_ok(ice_lite_init(&other));

	zassert_equal(strspn(fixture->agent.ufrag, ice_chars), ICE_LITE_UFRAG_LEN);
	zassert_equal(strspn(fixture->agent.pwd, ice_chars), ICE_LITE_PWD_LEN);
	zassert_not_equal(strcmp(fixture->agent.pwd, other.pwd), 0);
	zassert_equal(ice_lite_init(NULL), -EINVAL);
}

ZTEST_F(ice_lite, test_check_answered)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage mapped;
	struct net_sockaddr_storage src;
	struct stun_msg msg;

	exchange(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, 0U, resp, &msg);

	zassert_equal(msg.type, STUN_BINDING_SUCCESS_RESPONSE);
	zassert_ok(
		stun_verify_integrity(&msg, (const uint8_t *)fixture->agent.pwd, ICE_LITE_PWD_LEN));

	peer_addr(&src);
	zassert_ok(stun_get_xor_mapped_address(&msg, net_sad(&mapped)));
	zassert_mem_equal(&mapped, &src, sizeof(struct net_sockaddr_in));

	zassert_false(fixture->agent.nominated);
}

ZTEST_F(ice_lite, test_nomination)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage src;
	struct stun_msg msg;

	exchange(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_USE_CANDIDATE,
		 resp, &msg);

	zassert_equal(msg.type, STUN_BINDING_SUCCESS_RESPONSE);
	zassert_true(fixture->agent.nominated);

	peer_addr(&src);
	zassert_mem_equal(&fixture->agent.remote, &src, sizeof(struct net_sockaddr_in));

	zassert_ok(ice_lite_init(&fixture->agent));
	zassert_false(fixture->agent.nominated);
}

ZTEST_F(ice_lite, test_renomination)
{
	struct net_sockaddr_in6 src6 = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(40000),
	};
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage src;
	struct stun_msg msg;
	uint8_t check[CHECK_SIZE];
	size_t len;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6);

	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8::7", &src6.sin6_addr));
	len = build_check(check, sizeof(check), fixture->agent.ufrag, fixture->agent.pwd,
			  CHECK_USE_CANDIDATE);
	zassert_true(ice_lite_input(&fixture->agent, check, len, (const struct net_sockaddr *)&src6,
				    resp, sizeof(resp)) > 0);
	zassert_true(fixture->agent.nominated);

	/* The IPv4 address replaces the IPv6 one entirely. */
	exchange(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_USE_CANDIDATE,
		 resp, &msg);
	peer_addr(&src);
	zassert_mem_equal(&fixture->agent.remote, &src, sizeof(src));
}

ZTEST_F(ice_lite, test_ipv6_peer)
{
	struct net_sockaddr_in6 src = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(40000),
	};
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	uint8_t check[CHECK_SIZE];
	size_t len;
	int ret;

	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8::7", &src.sin6_addr));
	len = build_check(check, sizeof(check), fixture->agent.ufrag, fixture->agent.pwd,
			  CHECK_USE_CANDIDATE);

	ret = ice_lite_input(&fixture->agent, check, len, (const struct net_sockaddr *)&src, resp,
			     sizeof(resp));

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		zassert_true(ret > 0);
		zassert_true(fixture->agent.nominated);
		zassert_mem_equal(&fixture->agent.remote, &src, sizeof(struct net_sockaddr_in6));
	} else {
		zassert_equal(ret, -EAFNOSUPPORT);
		zassert_false(fixture->agent.nominated);
	}
}

ZTEST_F(ice_lite, test_bad_request)
{
	expect_error(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_NO_INTEGRITY,
		     400U, false);
	expect_error(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_NO_USERNAME,
		     400U, false);
	expect_error(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_NO_PRIORITY,
		     400U, false);
	expect_error(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_NO_ROLE, 400U,
		     false);
}

ZTEST_F(ice_lite, test_unauthenticated)
{
	expect_error(&fixture->agent, "someoneelse", fixture->agent.pwd, 0U, 401U, false);
	expect_error(&fixture->agent, fixture->agent.ufrag, "not-the-password-of-the-agent", 0U,
		     401U, false);
}

ZTEST_F(ice_lite, test_role_conflict)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct stun_msg msg;

	expect_error(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_CONTROLLED,
		     487U, true);

	/* The response carries the role of the agent and its tie-breaker. */
	exchange(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_CONTROLLED, resp,
		 &msg);
	zassert_true(msg.has_ice_controlled);
	zassert_false(msg.has_ice_controlling);
	zassert_equal(msg.ice_tiebreaker, fixture->agent.tiebreaker);
}

ZTEST_F(ice_lite, test_unknown_attribute)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	const uint8_t *value;
	struct stun_msg msg;
	uint16_t value_len;

	expect_error(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_UNKNOWN_ATTR,
		     420U, true);

	exchange(&fixture->agent, fixture->agent.ufrag, fixture->agent.pwd, CHECK_UNKNOWN_ATTR,
		 resp, &msg);
	zassert_ok(stun_find_attr(&msg, STUN_ATTR_UNKNOWN_ATTRIBUTES, &value, &value_len));
	zassert_equal(value_len, 2U);
	zassert_equal(sys_get_be16(value), ATTR_UNKNOWN);
}

ZTEST_F(ice_lite, test_not_stun)
{
	/* An RTP header and the start of a DTLS record (RFC 7983). */
	static const uint8_t rtp[] = {0x80, 0x60, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa0,
				      0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00,
				      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	static const uint8_t dtls[] = {0x16, 0xfe, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage src;

	peer_addr(&src);

	zassert_equal(ice_lite_input(&fixture->agent, rtp, sizeof(rtp), net_sad(&src), resp,
				     sizeof(resp)),
		      -ENOMSG);
	zassert_equal(ice_lite_input(&fixture->agent, dtls, sizeof(dtls), net_sad(&src), resp,
				     sizeof(resp)),
		      -ENOMSG);
}

ZTEST_F(ice_lite, test_keepalive)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage src;
	struct stun_msg msg;
	uint8_t buf[28];
	int len;

	len = ice_lite_keepalive(buf, sizeof(buf));
	zassert_equal(len, sizeof(buf));

	zassert_ok(stun_parse(buf, (size_t)len, &msg));
	zassert_equal(stun_msg_type_get_method(msg.type), STUN_METHOD_BINDING);
	zassert_equal(stun_msg_type_get_class(msg.type), STUN_CLASS_INDICATION);
	zassert_ok(stun_verify_fingerprint(&msg));

	/* The keepalive of the peer needs no answer. */
	peer_addr(&src);
	zassert_equal(ice_lite_input(&fixture->agent, buf, (size_t)len, net_sad(&src), resp,
				     sizeof(resp)),
		      0);

	zassert_equal(ice_lite_keepalive(buf, sizeof(buf) - 1U), -EMSGSIZE);
	zassert_equal(ice_lite_keepalive(NULL, sizeof(buf)), -EINVAL);
}

ZTEST_F(ice_lite, test_ignored)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct stun_builder builder;
	struct net_sockaddr_storage src;
	uint8_t buf[64];
	int len;

	peer_addr(&src);

	/* A check without FINGERPRINT. */
	zassert_ok(stun_builder_init(&builder, buf, sizeof(buf), STUN_BINDING_REQUEST, txid));
	zassert_ok(stun_add_integrity(&builder, (const uint8_t *)fixture->agent.pwd,
				      ICE_LITE_PWD_LEN));
	len = stun_builder_len(&builder);
	zassert_equal(ice_lite_input(&fixture->agent, buf, (size_t)len, net_sad(&src), resp,
				     sizeof(resp)),
		      0);

	/* A response: the agent has sent no request. */
	zassert_ok(
		stun_builder_init(&builder, buf, sizeof(buf), STUN_BINDING_SUCCESS_RESPONSE, txid));
	len = stun_finish_plain(&builder);
	zassert_equal(ice_lite_input(&fixture->agent, buf, (size_t)len, net_sad(&src), resp,
				     sizeof(resp)),
		      0);
}

ZTEST_F(ice_lite, test_damaged)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage src;
	uint8_t check[CHECK_SIZE];
	size_t len;

	peer_addr(&src);
	len = build_check(check, sizeof(check), fixture->agent.ufrag, fixture->agent.pwd,
			  CHECK_USE_CANDIDATE);

	/* A wrong FINGERPRINT: not STUN, so data. */
	check[len - 1U] ^= 0xffU;
	zassert_equal(
		ice_lite_input(&fixture->agent, check, len, net_sad(&src), resp, sizeof(resp)),
		-ENOMSG);

	/* A malformed attribute: PRIORITY, which follows USERNAME, of the wrong
	 * length. The low byte of its Length field is at the offset below.
	 */
	check[len - 1U] ^= 0xffU;
	check[STUN_HEADER_SIZE + 4U +
	      ROUND_UP(ICE_LITE_UFRAG_LEN + 1U + sizeof(PEER_UFRAG) - 1U, 4U) + 3U] = 5U;
	zassert_equal(
		ice_lite_input(&fixture->agent, check, len, net_sad(&src), resp, sizeof(resp)),
		-ENOMSG);

	zassert_false(fixture->agent.nominated);
}

ZTEST_F(ice_lite, test_arguments)
{
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage src;
	uint8_t check[CHECK_SIZE];
	size_t len;

	peer_addr(&src);
	len = build_check(check, sizeof(check), fixture->agent.ufrag, fixture->agent.pwd,
			  CHECK_USE_CANDIDATE);

	zassert_equal(ice_lite_input(NULL, check, len, net_sad(&src), resp, sizeof(resp)), -EINVAL);
	zassert_equal(ice_lite_input(&fixture->agent, NULL, len, net_sad(&src), resp, sizeof(resp)),
		      -EINVAL);
	zassert_equal(ice_lite_input(&fixture->agent, check, len, NULL, resp, sizeof(resp)),
		      -EINVAL);
	zassert_equal(
		ice_lite_input(&fixture->agent, check, len, net_sad(&src), NULL, sizeof(resp)),
		-EINVAL);
	zassert_equal(ice_lite_input(&fixture->agent, check, len, net_sad(&src), resp, 40U),
		      -EMSGSIZE);
	zassert_false(fixture->agent.nominated);
}

ZTEST_SUITE(ice_lite, NULL, setup, before, NULL, NULL);
