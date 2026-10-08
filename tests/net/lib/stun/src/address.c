/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * XOR-MAPPED-ADDRESS and the attributes that share its format (RFC 8489 §14.2),
 * for IPv4 and IPv6.
 */
#include <errno.h>

#include "stun_test.h"

/* XOR-PEER-ADDRESS of TURN: the same format, a type the codec does not know. */
#define ATTR_XOR_PEER_ADDRESS 0x0012U

/* 2001:db8:1234:5678:11:2233:4455:6677, the address of RFC 5769 §2.3. */
static const uint8_t kV6Rfc[16] __maybe_unused = {0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0x56, 0x78,
						  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

/* 2001:db8::1:2, the address of the kV6Response vector. */
static const uint8_t kV6Gen[16] __maybe_unused = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
						  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02};

ZTEST(stun, test_xor_mapped_address_ipv4)
{
	struct net_sockaddr addr;
	struct stun_msg m;

	zassert_ok(stun_parse(kResponse, sizeof(kResponse), &m));
	zassert_ok(stun_get_xor_mapped_address(kResponse, sizeof(kResponse), &m, &addr));
	zassert_true(sa_is_v4(&addr, 192, 0, 2, 1, 32853), "expected 192.0.2.1 port 32853");
}

ZTEST(stun, test_xor_mapped_address_ipv6)
{
	struct net_sockaddr addr;
	struct stun_msg m;
	int rc;

	/* A well-formed message whatever the IP stack is built with. */
	zassert_ok(stun_parse(kResponseV6, sizeof(kResponseV6), &m));
	rc = stun_get_xor_mapped_address(kResponseV6, sizeof(kResponseV6), &m, &addr);
#if defined(CONFIG_NET_IPV6)
	zassert_ok(rc);
	zassert_true(sa_is_v6(&addr, kV6Rfc, 32853),
		     "expected 2001:db8:1234:5678:11:2233:4455:6677 port 32853");
#else
	zassert_equal(rc, -EAFNOSUPPORT, "an IPv6 address without the IPv6 stack");
#endif
}

/* Build a response that carries one address attribute, parse it, read it back. */
static void roundtrip(uint16_t type, const struct net_sockaddr *in, struct net_sockaddr *out)
{
	struct stun_builder b;
	struct stun_msg m;
	uint8_t msg[64];
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	fill_txid(txid, 0x90);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);
	zassert_ok(stun_add_xor_address(&b, type, in));
	len = stun_builder_len(&b);
	zassert_true(len > 0);
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_ok(stun_get_xor_address(msg, (size_t)len, &m, type, out));
}

ZTEST(stun, test_xor_address_roundtrip)
{
	static const uint16_t kTypes[] = {STUN_ATTR_XOR_MAPPED_ADDRESS, ATTR_XOR_PEER_ADDRESS};
	struct net_sockaddr in, out;

	for (size_t i = 0; i < ARRAY_SIZE(kTypes); i++) {
		sa_v4(&in, 203, 0, 113, 7, 1);
		roundtrip(kTypes[i], &in, &out);
		zassert_true(sa_is_v4(&out, 203, 0, 113, 7, 1), "type 0x%04x", kTypes[i]);

		sa_v4(&in, 255, 255, 255, 255, 65535);
		roundtrip(kTypes[i], &in, &out);
		zassert_true(sa_is_v4(&out, 255, 255, 255, 255, 65535), "type 0x%04x", kTypes[i]);

#if defined(CONFIG_NET_IPV6)
		sa_v6(&in, kV6Gen, 50000);
		roundtrip(kTypes[i], &in, &out);
		zassert_true(sa_is_v6(&out, kV6Gen, 50000), "type 0x%04x", kTypes[i]);

		sa_v6(&in, kV6Rfc, 1);
		roundtrip(kTypes[i], &in, &out);
		zassert_true(sa_is_v6(&out, kV6Rfc, 1), "type 0x%04x", kTypes[i]);
#endif
	}
}

ZTEST(stun, test_xor_mapped_address_ipv6_byte_exact)
{
#if defined(CONFIG_NET_IPV6)
	struct net_sockaddr addr;
	struct stun_builder b;
	uint8_t msg[128];
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	/* Against the independent encoder: the address is XORed with the magic
	 * cookie and the transaction id (RFC 8489 §14.2).
	 */
	ensure_psa();
	fill_txid(txid, 0xB0);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);
	sa_v6(&addr, kV6Gen, 50000);
	zassert_ok(stun_add_xor_mapped_address(&b, &addr));
	len = stun_finish(&b, (const uint8_t *)kBaselinePwd, strlen(kBaselinePwd));
	zassert_equal(len, (int)sizeof(kV6Response), "length %d", len);
	zassert_mem_equal(msg, kV6Response, sizeof(kV6Response));
#else
	ztest_test_skip();
#endif
}

ZTEST(stun, test_xor_address_absent_and_bad_arguments)
{
	struct net_sockaddr addr;
	struct stun_builder b;
	struct raw_msg r;
	struct stun_msg m;
	uint8_t msg[64];
	uint8_t txid[STUN_TXID_SIZE];

	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x91);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_mapped_address(r.buf, r.len, &m, &addr), -ENOENT);
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, ATTR_XOR_PEER_ADDRESS, &addr),
		      -ENOENT);

	zassert_equal(stun_get_xor_mapped_address(r.buf, r.len, &m, NULL), -EINVAL);
	zassert_equal(stun_get_xor_mapped_address(NULL, r.len, &m, &addr), -EINVAL);
	zassert_equal(stun_get_xor_mapped_address(r.buf, r.len, NULL, &addr), -EINVAL);

	fill_txid(txid, 0x92);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);
	sa_v4(&addr, 192, 0, 2, 1, 3478);
	zassert_equal(stun_add_xor_mapped_address(&b, NULL), -EINVAL);
	zassert_equal(stun_add_xor_mapped_address(NULL, &addr), -EINVAL);
	zassert_equal(stun_add_xor_address(&b, STUN_ATTR_FINGERPRINT, &addr), -EINVAL,
		      "FINGERPRINT is not an address attribute");
	zassert_equal(b.err, 0, "a refused argument must not poison the builder");
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE, "and must not write anything");
}

ZTEST(stun, test_xor_address_unsupported_family)
{
	struct net_sockaddr addr;
	struct stun_builder b;
	uint8_t msg[64];
	uint8_t txid[STUN_TXID_SIZE];

	fill_txid(txid, 0x93);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);

	sa_v4(&addr, 192, 0, 2, 1, 3478);
	net_sin(&addr)->sin_family = NET_AF_UNSPEC;
	zassert_equal(stun_add_xor_mapped_address(&b, &addr), -EAFNOSUPPORT);
	zassert_equal(b.err, 0);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);

#if !defined(CONFIG_NET_IPV6)
	{
		/* Bigger than struct net_sockaddr in this build: only its family
		 * may be looked at.
		 */
		struct net_sockaddr_in6 sin6 = {.sin6_family = NET_AF_INET6};

		zassert_equal(stun_add_xor_mapped_address(&b, (struct net_sockaddr *)&sin6),
			      -EAFNOSUPPORT, "IPv6 without the IPv6 stack");
		zassert_equal(b.err, 0);
		zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);
	}
#endif

	/* The builder is still usable. */
	sa_v4(&addr, 192, 0, 2, 1, 3478);
	zassert_ok(stun_add_xor_mapped_address(&b, &addr));
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE + 12);
}

ZTEST(stun, test_xor_address_of_an_extension_is_checked_by_the_getter)
{
	static const uint8_t kFamily3[8] = {0x00, 0x03};
	static const uint8_t kV4In20[20] = {0x00, 0x01};
	static const uint8_t kShort[7] = {0x00, 0x01};
	struct net_sockaddr addr;
	struct raw_msg r;
	struct stun_msg m;

	/* The parser cannot judge an attribute it does not know; whoever reads
	 * it as an address gets the verdict.
	 */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x94);
	raw_attr(&r, ATTR_XOR_PEER_ADDRESS, sizeof(kFamily3), kFamily3, sizeof(kFamily3));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, ATTR_XOR_PEER_ADDRESS, &addr),
		      -EBADMSG, "address family 3");

	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x95);
	raw_attr(&r, ATTR_XOR_PEER_ADDRESS, sizeof(kV4In20), kV4In20, sizeof(kV4In20));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, ATTR_XOR_PEER_ADDRESS, &addr),
		      -EBADMSG, "IPv4 in 20 bytes");

	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x96);
	raw_attr(&r, ATTR_XOR_PEER_ADDRESS, sizeof(kShort), kShort, sizeof(kShort));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, ATTR_XOR_PEER_ADDRESS, &addr),
		      -EBADMSG, "address of 7 bytes");
}
