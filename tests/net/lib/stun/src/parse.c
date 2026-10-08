/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * The parser against RFC 8489: what is a STUN message, which attributes are
 * reported, which are ignored, and what makes a message malformed.
 */
#include <errno.h>

#include "stun_test.h"

/* Types no RFC assigns: comprehension-required and comprehension-optional. */
#define ATTR_UNKNOWN_REQ_A 0x7F00U
#define ATTR_UNKNOWN_REQ_B 0x7F01U
#define ATTR_UNKNOWN_REQ_C 0x7F02U
#define ATTR_UNKNOWN_OPT   0x8000U

/* Comprehension-optional attributes real servers send (RFC 5780). */
#define ATTR_RESPONSE_ORIGIN 0x802BU
#define ATTR_OTHER_ADDRESS   0x802CU

/* XOR-MAPPED-ADDRESS values for 192.0.2.1 port 32853 (the RFC 5769 one) and
 * for 192.0.2.2 port 32853; MAPPED-ADDRESS for 192.0.2.1 port 32853.
 */
static const uint8_t kXorAddrA[] = {0x00, 0x01, 0xa1, 0x47, 0xe1, 0x12, 0xa6, 0x43};
static const uint8_t kXorAddrB[] = {0x00, 0x01, 0xa1, 0x47, 0xe1, 0x12, 0xa6, 0x40};
static const uint8_t kMappedAddr[] = {0x00, 0x01, 0x80, 0x55, 0xc0, 0x00, 0x02, 0x01};

/* ERROR-CODE values: 2 reserved bytes, class, number (RFC 8489 §14.8). */
static const uint8_t kErr400[] = {0x00, 0x00, 0x04, 0x00};
static const uint8_t kErr487[] = {0x00, 0x00, 0x04, 0x57};

/* A value big enough for the longest text attribute and one byte more. */
static uint8_t big_text[STUN_TEXT_MAX_LEN + 8U];

/* The predicate and the parser must agree on every input: nothing parses that
 * is not a message, and the predicate holds for everything that parses.
 */
static void expect_message(const uint8_t *buf, size_t len, bool want, const char *what)
{
	struct stun_msg m;
	bool is = stun_is_message(buf, len);
	int rc = stun_parse(buf, len, &m);

	zassert_equal(is, want, "stun_is_message: %s", what);
	if (rc == 0) {
		zassert_true(is, "parsed, but not a message: %s", what);
	}
	if (!is) {
		zassert_not_equal(rc, 0, "not a message, but parsed: %s", what);
	}
}

/* A connectivity check signed by the library's own builder with kCheckPwd:
 * USERNAME, PRIORITY, ICE-CONTROLLED, MESSAGE-INTEGRITY, FINGERPRINT.
 */
static size_t signed_check(uint8_t *out, size_t cap)
{
	struct stun_builder b;
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	ensure_psa();
	fill_txid(txid, 0x30);
	stun_builder_init(&b, out, cap, STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, CHECK_USERNAME));
	zassert_ok(stun_add_priority(&b, 0x6E0001FFU));
	zassert_ok(stun_add_ice_controlled(&b, CONTROLLED_TB));
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	return (size_t)len;
}

/* The same for a success response that carries no address. */
static size_t signed_response(uint8_t *out, size_t cap)
{
	struct stun_builder b;
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	ensure_psa();
	fill_txid(txid, 0x31);
	stun_builder_init(&b, out, cap, STUN_BINDING_SUCCESS_RESPONSE, txid);
	zassert_ok(stun_add_software(&b, "zephyr"));
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	return (size_t)len;
}

/* What an attacker on the path can do without the key: insert an attribute
 * between MESSAGE-INTEGRITY and FINGERPRINT and recompute FINGERPRINT, which is
 * not keyed. msg ends with the FINGERPRINT the builder wrote; returns the new
 * length. Appending after FINGERPRINT instead would be caught by another rule
 * and prove nothing about the integrity boundary.
 */
static size_t splice_after_integrity(uint8_t *msg, size_t len, size_t cap, uint16_t type,
				     uint16_t alen, const void *val, size_t vlen)
{
	size_t padded = (alen + 3U) & ~3U;

	len -= 8U;
	zassert_true(len + 4U + padded + 8U <= cap, "splice overflow");
	put16(msg + len, type);
	put16(msg + len + 2, alen);
	memset(msg + len + 4, 0, padded);
	if (vlen != 0U) {
		memcpy(msg + len + 4, val, vlen);
	}
	len += 4U + padded;

	put16(msg + 2, (uint16_t)(len + 8U - STUN_HEADER_SIZE));
	put16(msg + len, STUN_ATTR_FINGERPRINT);
	put16(msg + len + 2, 4);
	put32(msg + len + 4, crc32_ieee(msg, len) ^ FINGERPRINT_XOR);
	return len + 8U;
}

/* The forgery is only a forgery if both checks still pass on it. */
static void expect_still_authentic(const uint8_t *msg, size_t len, const struct stun_msg *m)
{
	zassert_ok(
		stun_verify_integrity(msg, len, m, (const uint8_t *)kCheckPwd, strlen(kCheckPwd)),
		"the forged message must still pass MESSAGE-INTEGRITY");
	zassert_ok(stun_verify_fingerprint(msg, len, m),
		   "the forged message must still pass FINGERPRINT");
}

/* ======================= what is a STUN message ======================= */

ZTEST(stun, test_is_message)
{
	static const uint8_t kDtls[24] = {22, 0xfe, 0xfd, 0x00};
	static const uint8_t kRtp[24] = {0x80, 96, 0x00, 0x01};
	static const uint8_t kChannelData[24] = {0x40, 0x00, 0x00, 0x14};
	static const uint8_t kTopBits[] = {0x40, 0x80, 0xC0};
	uint8_t buf[sizeof(kRequest) + 4U];
	struct raw_msg r;

	expect_message(kRequest, sizeof(kRequest), true, "RFC 5769 request");
	expect_message(kResponse, sizeof(kResponse), true, "RFC 5769 IPv4 response");
	expect_message(kResponseV6, sizeof(kResponseV6), true, "RFC 5769 IPv6 response");

	raw_init(&r, STUN_BINDING_REQUEST, 0x11);
	expect_message(r.buf, r.len, true, "header without attributes");

	for (size_t i = 0; i < ARRAY_SIZE(kTopBits); i++) {
		memcpy(buf, kRequest, sizeof(kRequest));
		buf[0] |= kTopBits[i];
		expect_message(buf, sizeof(kRequest), false, "top bits of the type set");
	}

	memcpy(buf, kRequest, sizeof(kRequest));
	buf[7] ^= 0x01;
	expect_message(buf, sizeof(kRequest), false, "one bit of the magic cookie");

	expect_message(kRequest, STUN_HEADER_SIZE - 1U, false, "shorter than a header");
	expect_message(kRequest, 0, false, "empty");
	expect_message(NULL, sizeof(kRequest), false, "NULL");

	for (uint8_t length = 1; length <= 3; length++) {
		raw_init(&r, STUN_BINDING_REQUEST, 0x12);
		r.buf[3] = length;
		expect_message(r.buf, STUN_HEADER_SIZE + length, false,
			       "Length not a multiple of 4");
	}

	memcpy(buf, kRequest, sizeof(kRequest));
	memset(buf + sizeof(kRequest), 0, 4);
	expect_message(buf, sizeof(buf), false, "bytes after the message");
	expect_message(kRequest, sizeof(kRequest) - 4U, false, "shorter than its Length");

	expect_message(kDtls, sizeof(kDtls), false, "DTLS record");
	expect_message(kRtp, sizeof(kRtp), false, "RTP packet");
	expect_message(kChannelData, sizeof(kChannelData), false, "TURN ChannelData");
}

ZTEST(stun, test_parse_rejects_trailing_bytes)
{
	struct raw_msg r;
	struct stun_msg m;

	/* The message must account for the whole datagram (RFC 8489 §5, §6.3). */
	raw_init(&r, STUN_BINDING_REQUEST, 0x13);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_parse(r.buf, r.len + 4U, &m), -EBADMSG);
	zassert_equal(stun_parse(r.buf, r.len + 1U, &m), -EBADMSG);
}

/* ================= the integrity boundary (RFC 8489 §9) ================= */

ZTEST(stun, test_attributes_after_integrity_are_ignored)
{
	static const uint8_t kOtherTb[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	uint8_t msg[256];
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;
	struct net_sockaddr addr;
	size_t len;

	/* USE-CANDIDATE: would nominate a pair the peer never nominated. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_USE_CANDIDATE, 0, NULL, 0);
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_false(m.has_use_candidate);
	zassert_equal(stun_find_attr(msg, len, &m, STUN_ATTR_USE_CANDIDATE, &val, &vlen), -ENOENT);

	/* PRIORITY: the signed value stands. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_PRIORITY, 4,
				     "\x01\x02\x03\x04", 4);
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_true(m.has_priority);
	zassert_equal(m.priority, 0x6E0001FFU);

	/* USERNAME: the signed value stands. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_USERNAME, 4, "evil", 4);
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_equal(m.username_len, strlen(CHECK_USERNAME));
	zassert_mem_equal(m.username, CHECK_USERNAME, strlen(CHECK_USERNAME));

	/* The other ICE role: neither a role change nor a malformed message. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_ICE_CONTROLLING, 8, kOtherTb,
				     sizeof(kOtherTb));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_true(m.has_ice_controlled);
	zassert_false(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, CONTROLLED_TB);

	/* Not even looked at: a malformed attribute there does not matter. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_PRIORITY, 3, "\x01\x02\x03",
				     3);
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_equal(m.priority, 0x6E0001FFU);

	/* XOR-MAPPED-ADDRESS in a response that has none: a planted address
	 * must be invisible to the getter and to the generic lookup alike.
	 */
	len = signed_response(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_XOR_MAPPED_ADDRESS, 8,
				     kXorAddrA, sizeof(kXorAddrA));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_equal(stun_get_xor_mapped_address(msg, len, &m, &addr), -ENOENT);
	zassert_equal(stun_find_attr(msg, len, &m, STUN_ATTR_XOR_MAPPED_ADDRESS, &val, &vlen),
		      -ENOENT);
}

ZTEST(stun, test_sha256_and_fingerprint_after_integrity_are_found)
{
	uint8_t sha256[STUN_INTEGRITY_SHA256_MAX_LEN];
	uint8_t msg[256];
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;
	size_t len, mi_off;

	/* The two exceptions of RFC 8489 §9. */
	memset(sha256, 0xAB, sizeof(sha256));
	len = signed_check(msg, sizeof(msg));
	mi_off = len - 8U - 24U;
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_MESSAGE_INTEGRITY_SHA256,
				     sizeof(sha256), sha256, sizeof(sha256));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);

	zassert_true(m.has_integrity);
	zassert_equal(m.integrity_off, mi_off);
	zassert_equal(m.attrs_end, mi_off);
	zassert_true(m.has_integrity_sha256);
	zassert_equal(m.integrity_sha256_off, mi_off + 24U);
	zassert_equal(m.integrity_sha256_len, sizeof(sha256));
	zassert_true(m.has_fingerprint);
	zassert_equal(m.fingerprint_off, len - 8U);

	zassert_ok(stun_find_attr(msg, len, &m, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, &val, &vlen));
	zassert_equal(vlen, sizeof(sha256));
	zassert_mem_equal(val, sha256, sizeof(sha256));
	zassert_ok(stun_find_attr(msg, len, &m, STUN_ATTR_MESSAGE_INTEGRITY, &val, &vlen));
	zassert_equal(vlen, STUN_INTEGRITY_LEN);
	zassert_ok(stun_find_attr(msg, len, &m, STUN_ATTR_FINGERPRINT, &val, &vlen));
	zassert_equal(vlen, 4);
}

ZTEST(stun, test_sha256_is_the_boundary_without_integrity)
{
	uint8_t sha256[STUN_INTEGRITY_SHA256_MAX_LEN] = {0};
	struct raw_msg r;
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;
	size_t sha_off;

	raw_init(&r, STUN_BINDING_REQUEST, 0x14);
	raw_attr(&r, STUN_ATTR_USERNAME, 4, "user", 4);
	sha_off = r.len;
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, sizeof(sha256), sha256, sizeof(sha256));
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 7);
	raw_fingerprint(&r);

	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_ok(stun_verify_fingerprint(r.buf, r.len, &m));
	zassert_false(m.has_integrity);
	zassert_true(m.has_integrity_sha256);
	zassert_equal(m.integrity_sha256_off, sha_off);
	zassert_equal(m.attrs_end, sha_off);
	zassert_equal(m.username_len, 4);
	zassert_false(m.has_priority, "PRIORITY follows MESSAGE-INTEGRITY-SHA256");
	zassert_equal(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_PRIORITY, &val, &vlen), -ENOENT);
	zassert_true(m.has_fingerprint);
}

ZTEST(stun, test_second_integrity_is_ignored)
{
	uint8_t junk[STUN_INTEGRITY_LEN];
	uint8_t msg[256];
	struct stun_msg m;
	size_t len, mi_off;

	memset(junk, 0x5A, sizeof(junk));
	len = signed_check(msg, sizeof(msg));
	mi_off = len - 8U - 24U;
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_MESSAGE_INTEGRITY,
				     sizeof(junk), junk, sizeof(junk));
	zassert_ok(stun_parse(msg, len, &m));
	zassert_equal(m.integrity_off, mi_off, "the first MESSAGE-INTEGRITY is the one");
	expect_still_authentic(msg, len, &m);
}

/* ================= duplicates (RFC 8489 §14) and FINGERPRINT ================= */

ZTEST(stun, test_first_occurrence_wins)
{
	struct raw_msg r;
	struct stun_msg m;
	struct net_sockaddr addr;

	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x15);
	raw_attr(&r, STUN_ATTR_USERNAME, 5, "first", 5);
	raw_attr(&r, STUN_ATTR_USERNAME, 6, "second", 6);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 2);
	raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, 8, kXorAddrA, sizeof(kXorAddrA));
	raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, 8, kXorAddrB, sizeof(kXorAddrB));
	raw_attr(&r, STUN_ATTR_ERROR_CODE, 4, kErr400, sizeof(kErr400));
	raw_attr(&r, STUN_ATTR_ERROR_CODE, 4, kErr487, sizeof(kErr487));
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, 8, CONTROLLING_TB);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, 8, CONTROLLED_TB);

	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(m.username_len, 5);
	zassert_mem_equal(m.username, "first", 5);
	zassert_equal(m.priority, 1U);
	zassert_ok(stun_get_xor_mapped_address(r.buf, r.len, &m, &addr));
	zassert_true(sa_is_v4(&addr, 192, 0, 2, 1, 32853), "192.0.2.1, not the second 192.0.2.2");
	zassert_equal(m.error_code, 400U);
	zassert_true(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, CONTROLLING_TB);

	/* Same role twice, the controlled way round. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x16);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, 8, CONTROLLED_TB);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, 8, CONTROLLING_TB);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_true(m.has_ice_controlled);
	zassert_equal(m.ice_tiebreaker, CONTROLLED_TB);

	/* A duplicate is skipped, not examined: a malformed one does no harm. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x17);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 9);
	raw_u32(&r, STUN_ATTR_PRIORITY, 3, 0xAABBCCDDU);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(m.priority, 9U);
}

ZTEST(stun, test_both_ice_roles_are_malformed)
{
	struct raw_msg r;
	struct stun_msg m;

	/* RFC 8445 §7.1.3: an agent has one role. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x18);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, 8, CONTROLLING_TB);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, 8, CONTROLLED_TB);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	raw_init(&r, STUN_BINDING_REQUEST, 0x19);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, 8, CONTROLLED_TB);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, 8, CONTROLLING_TB);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);
}

ZTEST(stun, test_fingerprint_must_be_last)
{
	struct raw_msg r;
	struct stun_msg m;

	raw_init(&r, STUN_BINDING_REQUEST, 0x1A);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	raw_fingerprint(&r);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_ok(stun_verify_fingerprint(r.buf, r.len, &m));

	/* RFC 8489 §14.7. */
	raw_attr(&r, STUN_ATTR_SOFTWARE, 4, "late", 4);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	raw_init(&r, STUN_BINDING_REQUEST, 0x1B);
	raw_fingerprint(&r);
	raw_fingerprint(&r);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG, "two FINGERPRINTs");
}

/* ============ a known attribute must be well-formed ============ */

static void expect_malformed(uint16_t type, uint16_t alen, const void *val, size_t vlen,
			     const char *what)
{
	struct raw_msg r;
	struct stun_msg m;

	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, 0x20);
	raw_attr(&r, type, alen, val, vlen);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG, "%s", what);
}

static void expect_well_formed(uint16_t type, uint16_t alen, const void *val, size_t vlen,
			       const char *what)
{
	struct raw_msg r;
	struct stun_msg m;

	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, 0x21);
	raw_attr(&r, type, alen, val, vlen);
	zassert_ok(stun_parse(r.buf, r.len, &m), "%s", what);
}

ZTEST(stun, test_known_attribute_of_wrong_length_is_malformed)
{
	static const uint8_t kZero[40] = {0};
	/* family 3; IPv4 family in an IPv6-sized value; and the reverse */
	static const uint8_t kFamily3[8] = {0x00, 0x03};
	static const uint8_t kV4In20[20] = {0x00, 0x01};
	static const uint8_t kV6In8[8] = {0x00, 0x02};

	expect_malformed(STUN_ATTR_PRIORITY, 3, kZero, 3, "PRIORITY of 3 bytes");
	expect_malformed(STUN_ATTR_PRIORITY, 8, kZero, 8, "PRIORITY of 8 bytes");
	expect_malformed(STUN_ATTR_USE_CANDIDATE, 4, kZero, 4, "USE-CANDIDATE with a value");
	expect_malformed(STUN_ATTR_ICE_CONTROLLING, 4, kZero, 4, "ICE-CONTROLLING of 4 bytes");
	expect_malformed(STUN_ATTR_ICE_CONTROLLING, 12, kZero, 12, "ICE-CONTROLLING of 12 bytes");
	expect_malformed(STUN_ATTR_ICE_CONTROLLED, 4, kZero, 4, "ICE-CONTROLLED of 4 bytes");
	expect_malformed(STUN_ATTR_MESSAGE_INTEGRITY, 19, kZero, 19, "MESSAGE-INTEGRITY of 19");
	expect_malformed(STUN_ATTR_FINGERPRINT, 8, kZero, 8, "FINGERPRINT of 8 bytes");

	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, 8, kFamily3, 8, "address family 3");
	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, 20, kV4In20, 20, "IPv4 in 20 bytes");
	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, 8, kV6In8, 8, "IPv6 in 8 bytes");
	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, 7, kXorAddrA, 7, "address of 7 bytes");
	expect_malformed(STUN_ATTR_MAPPED_ADDRESS, 5, kMappedAddr, 5, "MAPPED-ADDRESS of 5");
	expect_malformed(STUN_ATTR_UNKNOWN_ATTRIBUTES, 3, kZero, 3, "UNKNOWN-ATTRIBUTES of 3");

	expect_well_formed(STUN_ATTR_XOR_MAPPED_ADDRESS, 8, kXorAddrA, 8, "IPv4 address");
	expect_well_formed(STUN_ATTR_MAPPED_ADDRESS, 8, kMappedAddr, 8, "MAPPED-ADDRESS");
	expect_well_formed(STUN_ATTR_UNKNOWN_ATTRIBUTES, 2, kZero, 2, "one unknown attribute");
	expect_well_formed(STUN_ATTR_UNKNOWN_ATTRIBUTES, 0, NULL, 0, "empty UNKNOWN-ATTRIBUTES");
}

ZTEST(stun, test_integrity_sha256_length)
{
	static const uint8_t kZero[40] = {0};
	static const uint16_t kBad[] = {0, 12, 18, 36};
	static const uint16_t kGood[] = {16, 20, 24, 28, 32};

	/* RFC 8489 §14.6: 16..32 bytes, a multiple of 4. */
	for (size_t i = 0; i < ARRAY_SIZE(kBad); i++) {
		expect_malformed(STUN_ATTR_MESSAGE_INTEGRITY_SHA256, kBad[i], kZero, kBad[i],
				 "MESSAGE-INTEGRITY-SHA256 length");
	}
	for (size_t i = 0; i < ARRAY_SIZE(kGood); i++) {
		expect_well_formed(STUN_ATTR_MESSAGE_INTEGRITY_SHA256, kGood[i], kZero, kGood[i],
				   "MESSAGE-INTEGRITY-SHA256 length");
	}
}

ZTEST(stun, test_error_code_values)
{
	static const uint8_t kShort[] = {0x00, 0x00, 0x04};
	static const uint8_t kClass2[] = {0x00, 0x00, 0x02, 0x00};
	static const uint8_t kClass7[] = {0x00, 0x00, 0x07, 0x00};
	static const uint8_t kNumber100[] = {0x00, 0x00, 0x04, 0x64};
	static const uint8_t kReservedSet[] = {0xFF, 0xFF, 0xE4, 0x57};
	struct raw_msg r;
	struct stun_msg m;

	/* RFC 8489 §14.8: the class MUST be 3..6 and the number 0..99. */
	expect_malformed(STUN_ATTR_ERROR_CODE, sizeof(kShort), kShort, sizeof(kShort),
			 "too short to hold a code");
	expect_malformed(STUN_ATTR_ERROR_CODE, 4, kClass2, 4, "class 2");
	expect_malformed(STUN_ATTR_ERROR_CODE, 4, kClass7, 4, "class 7");
	expect_malformed(STUN_ATTR_ERROR_CODE, 4, kNumber100, 4, "number 100");

	/* ...while the reserved bits are to be ignored by the receiver. */
	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, 0x22);
	raw_attr(&r, STUN_ATTR_ERROR_CODE, 4, kReservedSet, sizeof(kReservedSet));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_true(m.has_error_code);
	zassert_equal(m.error_code, 487U);
}

ZTEST(stun, test_text_attribute_limits)
{
	static const uint16_t kText[] = {STUN_ATTR_USERNAME, STUN_ATTR_SOFTWARE, STUN_ATTR_REALM,
					 STUN_ATTR_NONCE};
	struct raw_msg r;
	struct stun_msg m;

	memset(big_text, 'a', sizeof(big_text));

	/* A compliant parser accepts 763 bytes (RFC 8489 §14.3); refusing more
	 * is this codec's own limit.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(kText); i++) {
		expect_well_formed(kText[i], STUN_TEXT_MAX_LEN, big_text, STUN_TEXT_MAX_LEN,
				   "text attribute of 763 bytes");
		expect_malformed(kText[i], STUN_TEXT_MAX_LEN + 1U, big_text, STUN_TEXT_MAX_LEN + 1U,
				 "text attribute of 764 bytes");
	}

	raw_init(&r, STUN_BINDING_REQUEST, 0x23);
	raw_attr(&r, STUN_ATTR_USERNAME, STUN_TEXT_MAX_LEN, big_text, STUN_TEXT_MAX_LEN);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(m.username_len, STUN_TEXT_MAX_LEN);

	/* The reason phrase of ERROR-CODE, after its 4 bytes of code. */
	memcpy(big_text, kErr487, sizeof(kErr487));
	expect_well_formed(STUN_ATTR_ERROR_CODE, 4U + STUN_TEXT_MAX_LEN, big_text,
			   4U + STUN_TEXT_MAX_LEN, "reason phrase of 763 bytes");
	expect_malformed(STUN_ATTR_ERROR_CODE, 4U + STUN_TEXT_MAX_LEN + 1U, big_text,
			 4U + STUN_TEXT_MAX_LEN + 1U, "reason phrase of 764 bytes");
}

/* ============ unknown comprehension-required attributes ============ */

ZTEST(stun, test_unknown_attrs)
{
	static const uint16_t kMine[] = {0x000DU, 0x0016U};
	uint16_t out[4];
	struct raw_msg r;
	struct stun_msg m;

	/* Nothing to report: known attributes and comprehension-optional ones. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x40);
	raw_attr(&r, STUN_ATTR_USERNAME, 4, "user", 4);
	raw_attr(&r, ATTR_UNKNOWN_OPT, 0, NULL, 0);
	raw_attr(&r, 0xC057U, 4, "\0\0\0\1", 4);
	raw_attr(&r, ATTR_RESPONSE_ORIGIN, 8, kMappedAddr, 8);
	raw_attr(&r, ATTR_OTHER_ADDRESS, 8, kMappedAddr, 8);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, ARRAY_SIZE(out)), 0);

	/* One. */
	raw_attr(&r, ATTR_UNKNOWN_REQ_A, 4, "abcd", 4);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	memset(out, 0, sizeof(out));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, ARRAY_SIZE(out)), 1);
	zassert_equal(out[0], ATTR_UNKNOWN_REQ_A);

	/* Each type once, in order of appearance. */
	raw_attr(&r, ATTR_UNKNOWN_REQ_B, 0, NULL, 0);
	raw_attr(&r, ATTR_UNKNOWN_REQ_A, 0, NULL, 0);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	memset(out, 0, sizeof(out));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, ARRAY_SIZE(out)), 2);
	zassert_equal(out[0], ATTR_UNKNOWN_REQ_A);
	zassert_equal(out[1], ATTR_UNKNOWN_REQ_B);
	zassert_equal(out[2], 0, "nothing written past the count");

	/* The count is the full one whatever the room for the list. */
	raw_attr(&r, ATTR_UNKNOWN_REQ_C, 0, NULL, 0);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	memset(out, 0, sizeof(out));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, 1), 3);
	zassert_equal(out[0], ATTR_UNKNOWN_REQ_A);
	zassert_equal(out[1], 0, "nothing written past cap");
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, NULL, 0), 3);

	/* What the caller handles itself is not unknown. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x41);
	raw_u32(&r, 0x000DU, 4, 600);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, ARRAY_SIZE(out)), 1);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, kMine, ARRAY_SIZE(kMine), out,
					 ARRAY_SIZE(out)),
		      0);

	/* The types RFC 8489 only reserves (they were RFC 3489's) are unknown. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x42);
	raw_attr(&r, 0x0002U, 8, kMappedAddr, 8);
	raw_attr(&r, 0x0004U, 8, kMappedAddr, 8);
	raw_attr(&r, 0x0005U, 8, kMappedAddr, 8);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, ARRAY_SIZE(out)), 3);

	/* Bad arguments. */
	zassert_equal(stun_unknown_attrs(NULL, r.len, &m, NULL, 0, out, 1), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, NULL, NULL, 0, out, 1), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 1, out, 1), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, NULL, 1), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len - 4U, &m, NULL, 0, out, 1), -EINVAL,
		      "msg was parsed from another buffer length");
}

ZTEST(stun, test_unknown_attrs_knows_the_codec_set)
{
	static const uint8_t kZero[32] = {0};
	uint16_t out[4];
	struct raw_msg r;
	struct stun_msg m;

	/* Every comprehension-required type the codec itself understands. */
	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, 0x43);
	raw_attr(&r, STUN_ATTR_MAPPED_ADDRESS, 8, kMappedAddr, 8);
	raw_attr(&r, STUN_ATTR_USERNAME, 4, "user", 4);
	raw_attr(&r, STUN_ATTR_ERROR_CODE, 4, kErr487, 4);
	raw_attr(&r, STUN_ATTR_UNKNOWN_ATTRIBUTES, 2, kZero, 2);
	raw_attr(&r, STUN_ATTR_REALM, 5, "realm", 5);
	raw_attr(&r, STUN_ATTR_NONCE, 5, "nonce", 5);
	raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, 8, kXorAddrA, 8);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	raw_attr(&r, STUN_ATTR_USE_CANDIDATE, 0, NULL, 0);
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY, 20, kZero, 20);
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, 32, kZero, 32);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, ARRAY_SIZE(out)), 0);
}

ZTEST(stun, test_unknown_attrs_stops_at_integrity)
{
	uint16_t out[4];
	uint8_t msg[256];
	struct stun_msg m;
	size_t len;

	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), ATTR_UNKNOWN_REQ_A, 4, "abcd", 4);
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_equal(stun_unknown_attrs(msg, len, &m, NULL, 0, out, ARRAY_SIZE(out)), 0,
		      "an attribute nobody signed is not there at all");
}

ZTEST(stun, test_responses_of_live_servers)
{
	/* The attribute sets six public STUN servers returned to a Binding
	 * request in October 2026, with the address replaced by 192.0.2.1:
	 * none carries anything a strict parser has to refuse.
	 */
	static const char kSoftware[] = "example STUN server";
	uint16_t out[4];
	struct raw_msg r;
	struct stun_msg m;
	struct net_sockaddr addr;

	for (int shape = 0; shape < 5; shape++) {
		raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, (uint8_t)(0x50 + shape));
		raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, 8, kXorAddrA, 8);
		if (shape >= 1) {
			raw_attr(&r, STUN_ATTR_MAPPED_ADDRESS, 8, kMappedAddr, 8);
		}
		if (shape >= 2) {
			raw_attr(&r, ATTR_RESPONSE_ORIGIN, 8, kMappedAddr, 8);
		}
		if (shape == 2 || shape == 3) {
			raw_attr(&r, ATTR_OTHER_ADDRESS, 8, kMappedAddr, 8);
		}
		if (shape == 1 || shape >= 3) {
			raw_attr(&r, STUN_ATTR_SOFTWARE, sizeof(kSoftware) - 1U, kSoftware,
				 sizeof(kSoftware) - 1U);
		}
		if (shape == 4) {
			raw_fingerprint(&r);
		}

		zassert_true(stun_is_message(r.buf, r.len), "shape %d", shape);
		zassert_ok(stun_parse(r.buf, r.len, &m), "shape %d", shape);
		zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0, out, ARRAY_SIZE(out)),
			      0, "shape %d", shape);
		zassert_ok(stun_get_xor_mapped_address(r.buf, r.len, &m, &addr), "shape %d", shape);
		zassert_true(sa_is_v4(&addr, 192, 0, 2, 1, 32853), "shape %d", shape);
		if (shape == 4) {
			zassert_ok(stun_verify_fingerprint(r.buf, r.len, &m));
		}
	}
}

/* ================= message type, attribute lookup ================= */

ZTEST(stun, test_msg_type)
{
	static const uint16_t kMethods[] = {0x001, 0x003, 0x004, 0x006, 0x007,
					    0x008, 0x009, 0x080, 0xFFF};
	static const enum stun_class kClasses[] = {STUN_CLASS_REQUEST, STUN_CLASS_INDICATION,
						   STUN_CLASS_SUCCESS, STUN_CLASS_ERROR};

	zassert_equal(stun_msg_type(STUN_METHOD_BINDING, STUN_CLASS_REQUEST), 0x0001);
	zassert_equal(stun_msg_type(STUN_METHOD_BINDING, STUN_CLASS_INDICATION), 0x0011);
	zassert_equal(stun_msg_type(STUN_METHOD_BINDING, STUN_CLASS_SUCCESS), 0x0101);
	zassert_equal(stun_msg_type(STUN_METHOD_BINDING, STUN_CLASS_ERROR), 0x0111);
	zassert_equal(stun_msg_type(STUN_METHOD_BINDING, STUN_CLASS_REQUEST), STUN_BINDING_REQUEST);
	zassert_equal(stun_msg_type(STUN_METHOD_BINDING, STUN_CLASS_SUCCESS),
		      STUN_BINDING_SUCCESS_RESPONSE);
	zassert_equal(stun_msg_type(STUN_METHOD_BINDING, STUN_CLASS_ERROR),
		      STUN_BINDING_ERROR_RESPONSE);

	for (size_t i = 0; i < ARRAY_SIZE(kMethods); i++) {
		for (size_t k = 0; k < ARRAY_SIZE(kClasses); k++) {
			uint16_t type = stun_msg_type(kMethods[i], kClasses[k]);

			zassert_equal(type & 0xC000U, 0, "top bits of 0x%04x", type);
			zassert_equal(stun_msg_method(type), kMethods[i], "method of 0x%04x", type);
			zassert_equal(stun_msg_class(type), kClasses[k], "class of 0x%04x", type);
		}
	}
}

ZTEST(stun, test_find_attr)
{
	struct raw_msg r;
	struct stun_msg m;
	const uint8_t *val = NULL;
	uint16_t vlen = 0xFFFF;

	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x60);
	raw_attr(&r, 0x0016U, 8, kXorAddrA, 8);
	raw_attr(&r, 0x0016U, 8, kXorAddrB, 8);
	raw_attr(&r, 0x8050U, 0, NULL, 0);
	zassert_ok(stun_parse(r.buf, r.len, &m));

	/* The first of two, as a pointer into the caller's buffer. */
	zassert_ok(stun_find_attr(r.buf, r.len, &m, 0x0016U, &val, &vlen));
	zassert_equal(vlen, 8);
	zassert_equal_ptr(val, r.buf + STUN_HEADER_SIZE + 4U);
	zassert_mem_equal(val, kXorAddrA, 8);

	/* An attribute with no value is found, with length 0. */
	zassert_ok(stun_find_attr(r.buf, r.len, &m, 0x8050U, &val, &vlen));
	zassert_equal(vlen, 0);

	zassert_equal(stun_find_attr(r.buf, r.len, &m, 0x0017U, &val, &vlen), -ENOENT);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_FINGERPRINT, &val, &vlen),
		      -ENOENT);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_MESSAGE_INTEGRITY, &val, &vlen),
		      -ENOENT);

	zassert_equal(stun_find_attr(NULL, r.len, &m, 0x0016U, &val, &vlen), -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len, NULL, 0x0016U, &val, &vlen), -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, 0x0016U, NULL, &vlen), -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, 0x0016U, &val, NULL), -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len - 4U, &m, 0x0016U, &val, &vlen), -EINVAL,
		      "msg was parsed from another buffer length");
}
