/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * The codec against the RFC 5769 test vectors and against the vectors of an
 * independent encoder: parsing, MESSAGE-INTEGRITY, FINGERPRINT, and the bytes
 * the builder produces.
 */
#include <errno.h>

#include "stun_test.h"

/* clang-format off */
/* --- Vectors of an independent Python encoder written from RFC 8489, not
 * recordings of this library's output.
 * ---
 */

/* Baseline Binding Request: SOFTWARE, USERNAME, PRIORITY, USE-CANDIDATE,
 * XOR-MAPPED-ADDRESS 192.168.88.20:50000, MI over kBaselinePwd + FINGERPRINT.
 */
static const uint8_t kBaselineRequest[] = {
	0x00, 0x01, 0x00, 0x64, 0x21, 0x12, 0xa4, 0x42,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0x80, 0x22, 0x00, 0x10,
	0x7a, 0x65, 0x70, 0x68, 0x79, 0x72, 0x2d, 0x73,
	0x74, 0x75, 0x6e, 0x2d, 0x74, 0x65, 0x73, 0x74,
	0x00, 0x06, 0x00, 0x11, 0x43, 0x41, 0x4d, 0x75,
	0x66, 0x72, 0x61, 0x67, 0x3a, 0x50, 0x41, 0x4e,
	0x75, 0x66, 0x72, 0x61, 0x67, 0x00, 0x00, 0x00,
	0x00, 0x24, 0x00, 0x04, 0x7e, 0x00, 0x00, 0xff,
	0x00, 0x25, 0x00, 0x00, 0x00, 0x20, 0x00, 0x08,
	0x00, 0x01, 0xe2, 0x42, 0xe1, 0xba, 0xfc, 0x56,
	0x00, 0x08, 0x00, 0x14, 0xf3, 0x84, 0x05, 0x91,
	0x5e, 0xc4, 0x1b, 0x55, 0x22, 0xa9, 0x0a, 0x98,
	0xd3, 0xe6, 0x80, 0xf9, 0x27, 0x24, 0xa0, 0x59,
	0x80, 0x28, 0x00, 0x04, 0x01, 0xfd, 0x8d, 0x9b,
};

/* Controlling check: USERNAME, PRIORITY, ICE-CONTROLLING tie-breaker
 * 0x0123456789abcdef, USE-CANDIDATE, MI over kCheckPwd + FINGERPRINT.
 */
static const uint8_t kControllingRequest[] = {
	0x00, 0x01, 0x00, 0x50, 0x21, 0x12, 0xa4, 0x42,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0x00, 0x06, 0x00, 0x11,
	0x43, 0x41, 0x4d, 0x75, 0x66, 0x72, 0x61, 0x67,
	0x3a, 0x50, 0x41, 0x4e, 0x75, 0x66, 0x72, 0x61,
	0x67, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x04,
	0x6e, 0x00, 0x01, 0xff, 0x80, 0x2a, 0x00, 0x08,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x00, 0x25, 0x00, 0x00, 0x00, 0x08, 0x00, 0x14,
	0xd3, 0xae, 0x2c, 0x28, 0x90, 0xdf, 0x12, 0xd5,
	0x67, 0x77, 0x3c, 0x17, 0x8c, 0x2b, 0x9d, 0x48,
	0x7b, 0x87, 0x06, 0x19, 0x80, 0x28, 0x00, 0x04,
	0x97, 0x0a, 0x54, 0x09,
};

/* The same check sent as the controlled agent: ICE-CONTROLLED tie-breaker
 * 0xfedcba9876543210, no USE-CANDIDATE.
 */
static const uint8_t kControlledRequest[] = {
	0x00, 0x01, 0x00, 0x4c, 0x21, 0x12, 0xa4, 0x42,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0x00, 0x06, 0x00, 0x11,
	0x43, 0x41, 0x4d, 0x75, 0x66, 0x72, 0x61, 0x67,
	0x3a, 0x50, 0x41, 0x4e, 0x75, 0x66, 0x72, 0x61,
	0x67, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x04,
	0x6e, 0x00, 0x01, 0xff, 0x80, 0x29, 0x00, 0x08,
	0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
	0x00, 0x08, 0x00, 0x14, 0x9a, 0x3d, 0x5c, 0x8f,
	0x9f, 0x37, 0x04, 0x57, 0x75, 0x89, 0x5b, 0x5a,
	0xb4, 0xce, 0x80, 0x5a, 0xc9, 0xbd, 0xcc, 0x26,
	0x80, 0x28, 0x00, 0x04, 0xe4, 0x6f, 0x6f, 0x36,
};
/* clang-format on */

ZTEST_SUITE(stun, NULL, NULL, NULL, NULL, NULL);

ZTEST(stun, test_parse_request_vector)
{
	struct stun_msg m;

	zassert_ok(stun_parse(kRequest, sizeof(kRequest), &m));
	zassert_equal(m.type, STUN_BINDING_REQUEST);
	zassert_equal(m.username_len, 9);
	zassert_mem_equal(m.username, "evtj:h6vY", 9);
	zassert_true(m.has_priority);
	zassert_equal(m.priority, 0x6e0001ffU);
	/* The §2.1 vector is from a *controlled* agent (ICE-CONTROLLED, 0x8029);
	 * we parse ICE-CONTROLLING (0x802A) — so this one is absent here.
	 */
	zassert_false(m.has_ice_controlling);
	zassert_true(m.has_integrity);
	zassert_true(m.has_fingerprint);
	zassert_false(m.has_use_candidate);
}

ZTEST(stun, test_verify_request_integrity_and_fingerprint)
{
	struct stun_msg m;

	ensure_psa();
	zassert_ok(stun_parse(kRequest, sizeof(kRequest), &m));
	zassert_ok(stun_verify_fingerprint(kRequest, sizeof(kRequest), &m));
	zassert_ok(stun_verify_integrity(kRequest, sizeof(kRequest), &m, (const uint8_t *)kPassword,
					 strlen(kPassword)));
}

ZTEST(stun, test_verify_response_and_xor_mapped_address)
{
	struct net_sockaddr addr;
	struct stun_msg m;

	ensure_psa();
	zassert_ok(stun_parse(kResponse, sizeof(kResponse), &m));
	zassert_equal(m.type, STUN_BINDING_SUCCESS_RESPONSE);
	zassert_ok(stun_verify_fingerprint(kResponse, sizeof(kResponse), &m));
	zassert_ok(stun_verify_integrity(kResponse, sizeof(kResponse), &m,
					 (const uint8_t *)kPassword, strlen(kPassword)));

	zassert_ok(stun_get_xor_mapped_address(kResponse, sizeof(kResponse), &m, &addr));
	zassert_true(sa_is_v4(&addr, 192, 0, 2, 1, 32853), "expected 192.0.2.1 port 32853");
}

ZTEST(stun, test_build_roundtrip)
{
	struct stun_builder b;
	struct stun_msg m;
	uint8_t out[128];
	uint8_t txid[STUN_TXID_SIZE];
	struct net_sockaddr addr;
	int len;

	ensure_psa();
	/* Build a Binding Success Response like the one we send to answer a
	 * connectivity check, then parse it back through our own codec and
	 * verify MESSAGE-INTEGRITY + FINGERPRINT + XOR-MAPPED-ADDRESS. The
	 * RFC-vector verify tests already prove the HMAC/CRC match the RFC
	 * byte-exact; this proves build<->parse self-consistency. (The RFC
	 * 5769 §2.2 vector pads SOFTWARE with 0x20 spaces rather than zeros,
	 * so a byte-for-byte rebuild is intentionally not required.)
	 */
	memset(txid, 0xA5, sizeof(txid));
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_SUCCESS_RESPONSE, txid);
	zassert_ok(stun_add_software(&b, "zephyr-stun-test"));
	sa_v4(&addr, 192, 168, 88, 20, 50000);
	zassert_ok(stun_add_xor_mapped_address(&b, &addr));
	len = stun_finish(&b, (const uint8_t *)kPassword, strlen(kPassword));
	zassert_true(len > 0, "stun_finish failed (%d)", len);

	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_equal(m.type, STUN_BINDING_SUCCESS_RESPONSE);
	zassert_mem_equal(m.txid, txid, STUN_TXID_SIZE);
	zassert_ok(stun_verify_fingerprint(out, (size_t)len, &m));
	zassert_ok(stun_verify_integrity(out, (size_t)len, &m, (const uint8_t *)kPassword,
					 strlen(kPassword)));
	zassert_ok(stun_get_xor_mapped_address(out, (size_t)len, &m, &addr));
	zassert_true(sa_is_v4(&addr, 192, 168, 88, 20, 50000));
}

ZTEST(stun, test_negative_cases)
{
	struct stun_msg m;
	uint8_t tampered[sizeof(kRequest)];

	ensure_psa();

	/* Bad magic cookie -> parse rejects. */
	memcpy(tampered, kRequest, sizeof(kRequest));
	tampered[4] ^= 0xFF;
	zassert_equal(stun_parse(tampered, sizeof(tampered), &m), -EINVAL);

	/* Tampered MESSAGE-INTEGRITY value -> integrity fails, others ok. */
	memcpy(tampered, kRequest, sizeof(kRequest));
	zassert_ok(stun_parse(tampered, sizeof(tampered), &m));
	tampered[m.integrity_off + 4] ^= 0x01;
	zassert_equal(stun_verify_integrity(tampered, sizeof(tampered), &m,
					    (const uint8_t *)kPassword, strlen(kPassword)),
		      -EBADMSG);

	/* Tampered payload -> FINGERPRINT fails. */
	memcpy(tampered, kRequest, sizeof(kRequest));
	zassert_ok(stun_parse(tampered, sizeof(tampered), &m));
	tampered[24] ^= 0x01; /* a SOFTWARE byte */
	zassert_equal(stun_verify_fingerprint(tampered, sizeof(tampered), &m), -EBADMSG);

	/* Wrong password -> integrity fails. */
	zassert_ok(stun_parse(kRequest, sizeof(kRequest), &m));
	zassert_equal(stun_verify_integrity(kRequest, sizeof(kRequest), &m,
					    (const uint8_t *)"wrongpass", 9),
		      -EBADMSG);

	/* Truncated below the header. */
	zassert_equal(stun_parse(kRequest, 12, &m), -EINVAL);
}

/* ==================== the builder, byte for byte ==================== */

ZTEST(stun, test_baseline_builder_byte_exact)
{
	struct net_sockaddr addr;
	struct stun_builder b;
	uint8_t out[256];
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	ensure_psa();

	/* One of every attribute the plain adders emit, in call order, against
	 * the independent encoder.
	 */
	fill_txid(txid, 0xA0);
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_software(&b, "zephyr-stun-test"));
	zassert_ok(stun_add_username(&b, CHECK_USERNAME));
	zassert_ok(stun_add_priority(&b, 0x7E0000FFU));
	zassert_ok(stun_add_use_candidate(&b));
	sa_v4(&addr, 192, 168, 88, 20, 50000);
	zassert_ok(stun_add_xor_mapped_address(&b, &addr));
	len = stun_finish(&b, (const uint8_t *)kBaselinePwd, strlen(kBaselinePwd));

	zassert_equal(len, (int)sizeof(kBaselineRequest), "length %d", len);
	zassert_mem_equal(out, kBaselineRequest, sizeof(kBaselineRequest),
			  "baseline encoding changed");
}

ZTEST(stun, test_baseline_builder_overflow)
{
	struct stun_builder b;
	uint8_t out[128];

	ensure_psa();

	/* Too small for the header: init itself fails and stays failed. */
	stun_builder_init(&b, out, STUN_HEADER_SIZE - 1U, STUN_BINDING_REQUEST,
			  (const uint8_t *)"txid-12-bytes");
	zassert_equal(b.err, -EMSGSIZE);
	zassert_equal(stun_add_priority(&b, 1), -EMSGSIZE);

	/* Header fits, the first attribute does not; the error is sticky and
	 * stun_finish reports it rather than writing anything.
	 */
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_REQUEST,
			  (const uint8_t *)"txid-12-bytes");
	zassert_ok(b.err);
	zassert_equal(stun_add_priority(&b, 1), -EMSGSIZE);
	zassert_equal(stun_add_use_candidate(&b), -EMSGSIZE);
	zassert_equal(stun_finish(&b, (const uint8_t *)kBaselinePwd, strlen(kBaselinePwd)),
		      -EMSGSIZE);

	/* One PRIORITY (8) + MI (24) + FINGERPRINT (8) needs exactly 60 bytes:
	 * 51 fails inside MI, 59 inside FINGERPRINT, 60 succeeds.
	 */
	static const size_t kCaps[] = {51, 59, 60};

	for (size_t i = 0; i < ARRAY_SIZE(kCaps); i++) {
		size_t cap = kCaps[i];

		stun_builder_init(&b, out, cap, STUN_BINDING_REQUEST,
				  (const uint8_t *)"txid-12-bytes");
		zassert_ok(stun_add_priority(&b, 1));
		int rc = stun_finish(&b, (const uint8_t *)kBaselinePwd, strlen(kBaselinePwd));

		if (cap < 60) {
			zassert_equal(rc, -EMSGSIZE, "cap %zu", cap);
		} else {
			zassert_equal(rc, 60, "cap %zu -> %d", cap, rc);
		}
	}
}

ZTEST(stun, test_absent_attributes)
{
	struct raw_msg r;
	struct net_sockaddr addr;
	struct stun_msg m;

	/* An attribute-less message is valid. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x01);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(m.length, 0);
	zassert_equal(m.attrs_end, r.len);
	zassert_is_null(m.username);
	zassert_false(m.has_priority);
	zassert_false(m.has_use_candidate);
	zassert_false(m.has_error_code);
	zassert_false(m.has_ice_controlling);
	zassert_false(m.has_ice_controlled);
	zassert_false(m.has_integrity);
	zassert_false(m.has_integrity_sha256);
	zassert_false(m.has_fingerprint);

	/* Verifiers report absence rather than failure. */
	zassert_equal(stun_verify_integrity(r.buf, r.len, &m, (const uint8_t *)kBaselinePwd,
					    strlen(kBaselinePwd)),
		      -ENOENT);
	zassert_equal(stun_verify_fingerprint(r.buf, r.len, &m), -ENOENT);
	zassert_equal(stun_get_xor_mapped_address(r.buf, r.len, &m, &addr), -ENOENT);
}

ZTEST(stun, test_baseline_parser_strictness)
{
	struct raw_msg r;
	struct stun_msg m;

	/* MESSAGE-INTEGRITY and FINGERPRINT lengths are load-bearing: a wrong
	 * one is -EBADMSG (the verifiers must never read a short value).
	 */
	raw_init(&r, STUN_BINDING_REQUEST, 0x02);
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY, 19, "0123456789012345678", 19);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	raw_init(&r, STUN_BINDING_REQUEST, 0x03);
	raw_u64(&r, STUN_ATTR_FINGERPRINT, 8, 0);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Attribute value running past the advertised attribute area. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x04);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	r.buf[2] = 0;
	r.buf[3] = 4; /* claim 4 attribute bytes, PRIORITY needs 8 */
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Length field not a multiple of 4. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x05);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	r.buf[3] = 6;
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Length field longer than the datagram. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x06);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	r.buf[3] = 0x40;
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Header-level rejects: reserved top bits, NULL args. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x07);
	r.buf[0] |= 0x40U;
	zassert_equal(stun_parse(r.buf, r.len, &m), -EINVAL);
	zassert_equal(stun_parse(NULL, 20, &m), -EINVAL);
	zassert_equal(stun_parse(r.buf, r.len, NULL), -EINVAL);
}

/* ======================= ICE role attributes ======================= */

ZTEST(stun, test_ice_controlling_builder_byte_exact)
{
	struct stun_builder b;
	struct stun_msg m;
	uint8_t out[256];
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	ensure_psa();

	/* The check a controlling agent (the panel) sends, against the
	 * independent encoder: 0x802A, 8-byte tie-breaker in network order, no
	 * padding, and the attributes that follow it land where they should.
	 */
	fill_txid(txid, 0xC0);
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, CHECK_USERNAME));
	zassert_ok(stun_add_priority(&b, 0x6E0001FFU));
	zassert_ok(stun_add_ice_controlling(&b, CONTROLLING_TB));
	zassert_ok(stun_add_use_candidate(&b));
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));

	zassert_equal(len, (int)sizeof(kControllingRequest), "length %d", len);
	zassert_mem_equal(out, kControllingRequest, sizeof(kControllingRequest));

	/* Roundtrip through our own parser, and the peer-side checks pass. */
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_true(m.has_ice_controlling);
	zassert_false(m.has_ice_controlled);
	zassert_equal(m.ice_tiebreaker, CONTROLLING_TB);
	zassert_true(m.has_use_candidate);
	zassert_equal(m.priority, 0x6E0001FFU);
	zassert_ok(stun_verify_fingerprint(out, (size_t)len, &m));
	zassert_ok(stun_verify_integrity(out, (size_t)len, &m, (const uint8_t *)kCheckPwd,
					 strlen(kCheckPwd)));
}

ZTEST(stun, test_role_tiebreaker_roundtrip)
{
	static const uint64_t kTbs[] = {
		0ULL,
		1ULL,
		0x00000000FFFFFFFFULL,
		0xFFFFFFFF00000000ULL,
		0x0000000100000000ULL,
		UINT64_MAX,
	};

	ensure_psa();

	/* Both 32-bit halves survive in the right order (a swap would pass the
	 * symmetric values and fail the asymmetric ones).
	 */
	for (size_t i = 0; i < ARRAY_SIZE(kTbs); i++) {
		struct stun_builder b;
		struct stun_msg m;
		uint8_t out[128];
		uint8_t txid[STUN_TXID_SIZE];
		int len;

		fill_txid(txid, (uint8_t)(0x30 + i));

		stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
		zassert_ok(stun_add_ice_controlling(&b, kTbs[i]));
		len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
		zassert_true(len > 0);
		zassert_ok(stun_parse(out, (size_t)len, &m));
		zassert_true(m.has_ice_controlling);
		zassert_equal(m.ice_tiebreaker, kTbs[i], "controlling tb #%zu", i);

		stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
		zassert_ok(stun_add_ice_controlled(&b, kTbs[i]));
		len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
		zassert_true(len > 0);
		zassert_ok(stun_parse(out, (size_t)len, &m));
		zassert_true(m.has_ice_controlled);
		zassert_false(m.has_ice_controlling);
		zassert_equal(m.ice_tiebreaker, kTbs[i], "controlled tb #%zu", i);
	}

	/* Tight buffer: the role attribute needs 12 bytes like any 8-byte TLV. */
	struct stun_builder b;
	uint8_t small[STUN_HEADER_SIZE + 11];
	uint8_t txid[STUN_TXID_SIZE];

	fill_txid(txid, 0x40);
	stun_builder_init(&b, small, sizeof(small), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_ice_controlled(&b, CONTROLLED_TB), -EMSGSIZE);
}

ZTEST(stun, test_ice_controlled_builder_byte_exact)
{
	struct stun_builder b;
	struct stun_msg m;
	uint8_t out[256];
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	ensure_psa();

	fill_txid(txid, 0xC0);
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, CHECK_USERNAME));
	zassert_ok(stun_add_priority(&b, 0x6E0001FFU));
	zassert_ok(stun_add_ice_controlled(&b, CONTROLLED_TB));
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));

	zassert_equal(len, (int)sizeof(kControlledRequest), "length %d", len);
	zassert_mem_equal(out, kControlledRequest, sizeof(kControlledRequest));

	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_true(m.has_ice_controlled);
	zassert_false(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, CONTROLLED_TB);

	/* The RFC 5769 §2.1 vector is a *controlled* agent's request, so it now
	 * exercises the parse path against an RFC-authored tie-breaker.
	 */
	zassert_ok(stun_parse(kRequest, sizeof(kRequest), &m));
	zassert_true(m.has_ice_controlled);
	zassert_false(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, 0x932ff9b151263b36ULL);
}

/* ============================ ERROR-CODE ============================ */

ZTEST(stun, test_error_code_builder_and_roundtrip)
{
	struct stun_builder b;
	struct stun_msg m;
	uint8_t out[256];
	uint8_t txid[STUN_TXID_SIZE];
	int len;
	static const uint16_t kCodes[] = {300U, 400U, 487U, 500U, 699U};

	ensure_psa();
	fill_txid(txid, 0x70);

	/* Every code in the RFC's range survives the class/number split. */
	for (size_t i = 0; i < ARRAY_SIZE(kCodes); i++) {
		stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
		zassert_ok(stun_add_error_code(&b, kCodes[i], "Role Conflict"), "code %u",
			   kCodes[i]);
		len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
		zassert_true(len > 0);

		zassert_ok(stun_parse(out, (size_t)len, &m));
		zassert_true(m.has_error_code, "code %u lost", kCodes[i]);
		zassert_equal(m.error_code, kCodes[i]);
		zassert_ok(stun_verify_integrity(out, (size_t)len, &m, (const uint8_t *)kCheckPwd,
						 strlen(kCheckPwd)));
	}

	/* No reason phrase at all is legal: the code is what matters. */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_ok(stun_add_error_code(&b, 487U, NULL));
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
	zassert_true(len > 0);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_equal(m.error_code, 487U);

	/* A long phrase is sent whole... */
	static const char kLong[] = "a reason phrase that is longer than most, to show that the "
				    "builder sends it whole";

	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_ok(stun_add_error_code(&b, 487U, kLong));
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
	zassert_true(len > 0);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_equal(m.error_code, 487U);

	/* ...but a code outside 300..699 is refused, without touching the
	 * message being built.
	 */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_equal(stun_add_error_code(&b, 299U, "nope"), -EINVAL);
	zassert_equal(stun_add_error_code(&b, 700U, "nope"), -EINVAL);
	zassert_equal(b.err, 0, "a refused code must not poison the builder");
	zassert_ok(stun_add_error_code(&b, 487U, "Role Conflict"));
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
	zassert_true(len > 0);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_equal(m.error_code, 487U);

	/* A message with no ERROR-CODE reports none (what a bare error response
	 * from a terse peer looks like).
	 */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	len = stun_finish(&b, (const uint8_t *)kCheckPwd, strlen(kCheckPwd));
	zassert_true(len > 0);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_false(m.has_error_code);
	zassert_equal(m.error_code, 0U);
}
