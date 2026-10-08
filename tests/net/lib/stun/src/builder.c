/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * The message builder: generic attributes, the closing attributes and their
 * order, the limits RFC 8489 puts on what is sent, and what an error leaves
 * behind.
 */
#include <errno.h>

#include "stun_test.h"

#define CHECK_KEY     ((const uint8_t *)kCheckPwd)
#define CHECK_KEY_LEN (sizeof(kCheckPwd) - 1U)

/* A type no RFC assigns. */
#define ATTR_PRIVATE 0x7F10U

/* Longer than any key the PSA key store takes: importing it fails. */
static uint8_t huge_key[9000];

/* Room for the longest strings the tests build, and a message to hold them. */
static char text[4U * (STUN_TEXT_MAX_CHARS + 2U)];
static uint8_t big_msg[1024];

static void start(struct stun_builder *b, uint8_t *buf, size_t cap, uint16_t type, uint8_t txid0)
{
	uint8_t txid[STUN_TXID_SIZE];

	fill_txid(txid, txid0);
	stun_builder_init(b, buf, cap, type, txid);
}

/* The USERNAME and PRIORITY of the reference vectors. */
static void add_check_head(struct stun_builder *b)
{
	zassert_ok(stun_add_username(b, CHECK_USERNAME));
	zassert_ok(stun_add_priority(b, 0x6E0001FFU));
}

/* `n` characters of `utf8` (one character, 1 to 4 bytes) into text[]. */
static const char *repeat(const char *utf8, size_t n)
{
	size_t clen = strlen(utf8);

	zassert_true(n * clen < sizeof(text));
	for (size_t i = 0; i < n; i++) {
		memcpy(text + i * clen, utf8, clen);
	}
	text[n * clen] = '\0';
	return text;
}

ZTEST(stun, test_add_attr)
{
	static const uint8_t kVal[5] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5};
	struct stun_builder b;
	uint8_t msg[64];

	for (size_t vlen = 0; vlen <= sizeof(kVal); vlen++) {
		size_t padded = (vlen + 3U) & ~3U;

		memset(msg, 0xFF, sizeof(msg));
		start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x70);
		zassert_ok(stun_add_attr(&b, ATTR_PRIVATE, kVal, vlen), "length %zu", vlen);
		zassert_equal(stun_builder_len(&b), (int)(STUN_HEADER_SIZE + 4U + padded));

		/* type, length without padding, value, zero padding */
		zassert_equal(msg[20], ATTR_PRIVATE >> 8);
		zassert_equal(msg[21], ATTR_PRIVATE & 0xFFU);
		zassert_equal(msg[22], 0);
		zassert_equal(msg[23], vlen);
		zassert_mem_equal(msg + 24, kVal, vlen);
		for (size_t i = vlen; i < padded; i++) {
			zassert_equal(msg[24 + i], 0, "padding byte %zu of length %zu", i, vlen);
		}
		/* the header counts it */
		zassert_equal(msg[2], 0);
		zassert_equal(msg[3], 4U + padded);
		zassert_true(stun_is_message(msg, STUN_HEADER_SIZE + 4U + padded));
	}

	/* What does not fit an attribute, or is not for this function. */
	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x71);
	zassert_equal(stun_add_attr(&b, ATTR_PRIVATE, kVal, 65536), -EINVAL);
	zassert_equal(stun_add_attr(&b, ATTR_PRIVATE, NULL, 4), -EINVAL);
	zassert_equal(stun_add_attr(&b, STUN_ATTR_MESSAGE_INTEGRITY, kVal, 4), -EINVAL);
	zassert_equal(stun_add_attr(&b, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, kVal, 4), -EINVAL);
	zassert_equal(stun_add_attr(&b, STUN_ATTR_FINGERPRINT, kVal, 4), -EINVAL);
	zassert_equal(stun_add_attr(NULL, ATTR_PRIVATE, kVal, 4), -EINVAL);
	zassert_equal(b.err, 0, "a refused argument must not poison the builder");
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE, "and must not write anything");
	zassert_ok(stun_add_attr(&b, ATTR_PRIVATE, NULL, 0));
}

ZTEST(stun, test_builder_len)
{
	struct stun_builder b;
	uint8_t msg[64];
	int len;

	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x72);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);
	zassert_ok(stun_add_priority(&b, 1));
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE + 8);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE + 8, "asking twice changes nothing");

	len = stun_finish_plain(&b);
	zassert_equal(len, STUN_HEADER_SIZE + 8 + 8);
	zassert_equal(stun_builder_len(&b), len);

	zassert_equal(stun_builder_len(NULL), -EINVAL);

	/* The error an overflow left behind. */
	start(&b, msg, STUN_HEADER_SIZE + 4U, STUN_BINDING_REQUEST, 0x73);
	zassert_equal(stun_add_priority(&b, 1), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
}

/* A call out of order: -EPERM, and neither the message nor the builder changes. */
#define EXPECT_EPERM(call)                                                                         \
	do {                                                                                       \
		uint8_t before_[sizeof(msg)];                                                      \
		int len_ = stun_builder_len(&b);                                                   \
                                                                                                   \
		memcpy(before_, msg, sizeof(msg));                                                 \
		zassert_equal(call, -EPERM, #call);                                                \
		zassert_equal(b.err, 0, #call " must not poison the builder");                     \
		zassert_equal(stun_builder_len(&b), len_, #call " must not change the length");    \
		zassert_mem_equal(msg, before_, sizeof(msg), #call " must not touch the message"); \
	} while (false)

ZTEST(stun, test_closing_attributes_order)
{
	struct net_sockaddr addr;
	struct stun_builder b;
	struct stun_msg m;
	uint8_t msg[192];
	int len;

	ensure_psa();
	sa_v4(&addr, 192, 0, 2, 1, 3478);

	/* RFC 8489 §9, §14.7: MESSAGE-INTEGRITY, then MESSAGE-INTEGRITY-SHA256,
	 * then FINGERPRINT, and nothing else from the first of them on.
	 */
	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x74);
	add_check_head(&b);
	zassert_ok(stun_add_integrity(&b, CHECK_KEY, CHECK_KEY_LEN));

	EXPECT_EPERM(stun_add_priority(&b, 1));
	EXPECT_EPERM(stun_add_use_candidate(&b));
	EXPECT_EPERM(stun_add_username(&b, "late"));
	EXPECT_EPERM(stun_add_software(&b, "late"));
	EXPECT_EPERM(stun_add_error_code(&b, 400, NULL));
	EXPECT_EPERM(stun_add_xor_mapped_address(&b, &addr));
	EXPECT_EPERM(stun_add_ice_controlling(&b, 1));
	EXPECT_EPERM(stun_add_ice_controlled(&b, 1));
	EXPECT_EPERM(stun_add_unknown_attributes(&b, NULL, 0));
	EXPECT_EPERM(stun_add_attr(&b, ATTR_PRIVATE, NULL, 0));
	EXPECT_EPERM(stun_add_integrity(&b, CHECK_KEY, CHECK_KEY_LEN));

	zassert_ok(stun_add_integrity_sha256(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_add_integrity(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_add_integrity_sha256(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_add_priority(&b, 1));

	zassert_ok(stun_add_fingerprint(&b));
	EXPECT_EPERM(stun_add_fingerprint(&b));
	EXPECT_EPERM(stun_add_integrity(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_add_integrity_sha256(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_add_priority(&b, 1));
	EXPECT_EPERM(stun_finish(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_finish_plain(&b));

	/* What was built is a message the parser takes, with all three. */
	len = stun_builder_len(&b);
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_ok(stun_verify_integrity(msg, (size_t)len, &m, CHECK_KEY, CHECK_KEY_LEN));
	zassert_ok(
		stun_verify_integrity_sha256(msg, (size_t)len, &m, CHECK_KEY, CHECK_KEY_LEN, 32));
	zassert_ok(stun_verify_fingerprint(msg, (size_t)len, &m));

	/* MESSAGE-INTEGRITY cannot follow MESSAGE-INTEGRITY-SHA256. */
	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x75);
	zassert_ok(stun_add_integrity_sha256(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_add_integrity(&b, CHECK_KEY, CHECK_KEY_LEN));

	/* Closing twice with the wrappers. */
	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x76);
	zassert_true(stun_finish(&b, CHECK_KEY, CHECK_KEY_LEN) > 0);
	EXPECT_EPERM(stun_finish(&b, CHECK_KEY, CHECK_KEY_LEN));
	EXPECT_EPERM(stun_finish_plain(&b));
}

ZTEST(stun, test_integrity_failure_leaves_the_message_intact)
{
	uint8_t before[64];
	struct stun_builder b;
	uint8_t msg[64];

	ensure_psa();

	/* An empty or missing key is a refused argument. */
	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x77);
	zassert_ok(stun_add_priority(&b, 1));
	zassert_equal(stun_add_integrity(&b, NULL, 4), -EINVAL);
	zassert_equal(stun_add_integrity(&b, CHECK_KEY, 0), -EINVAL);
	zassert_equal(stun_add_integrity_sha256(&b, NULL, 4), -EINVAL);
	zassert_equal(stun_add_integrity_sha256(&b, CHECK_KEY, 0), -EINVAL);
	zassert_equal(stun_finish(&b, NULL, 4), -EINVAL);
	zassert_equal(b.err, 0);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE + 8);

	/* A backend that fails is remembered, and nothing half-written stays:
	 * the header still describes the message as it was.
	 */
	memcpy(before, msg, sizeof(msg));
	zassert_equal(stun_add_integrity(&b, huge_key, sizeof(huge_key)), -EIO);
	zassert_equal(b.err, -EIO);
	zassert_equal(b.len, STUN_HEADER_SIZE + 8U);
	zassert_mem_equal(msg, before, sizeof(msg));
	zassert_true(stun_is_message(msg, b.len));
	zassert_equal(stun_add_priority(&b, 2), -EIO);
	zassert_equal(stun_builder_len(&b), -EIO);
	zassert_equal(stun_finish(&b, CHECK_KEY, CHECK_KEY_LEN), -EIO);
	zassert_equal(stun_finish_plain(&b), -EIO);

	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x78);
	memcpy(before, msg, sizeof(msg));
	zassert_equal(stun_add_integrity_sha256(&b, huge_key, sizeof(huge_key)), -EIO);
	zassert_equal(b.len, STUN_HEADER_SIZE);
	zassert_mem_equal(msg, before, sizeof(msg));

	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0x79);
	zassert_equal(stun_finish(&b, huge_key, sizeof(huge_key)), -EIO);
	zassert_equal(b.err, -EIO);
	zassert_equal(b.len, STUN_HEADER_SIZE);
}

ZTEST(stun, test_text_limits_on_sending)
{
	struct stun_builder b;
	struct stun_msg m;
	int len;

	/* RFC 8489 §14.3: USERNAME is fewer than 509 bytes. */
	start(&b, big_msg, sizeof(big_msg), STUN_BINDING_REQUEST, 0x7A);
	zassert_equal(stun_add_username(&b, repeat("u", STUN_USERNAME_MAX_LEN + 1U)), -EINVAL);
	zassert_equal(b.err, 0);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);
	zassert_ok(stun_add_username(&b, repeat("u", STUN_USERNAME_MAX_LEN)));
	len = stun_builder_len(&b);
	zassert_ok(stun_parse(big_msg, (size_t)len, &m));
	zassert_equal(m.username_len, STUN_USERNAME_MAX_LEN);

	/* §14.14: SOFTWARE is fewer than 128 characters, whatever they take in
	 * bytes: U+20AC is three.
	 */
	start(&b, big_msg, sizeof(big_msg), STUN_BINDING_REQUEST, 0x7B);
	zassert_equal(stun_add_software(&b, repeat("s", STUN_TEXT_MAX_CHARS + 1U)), -EINVAL);
	zassert_equal(stun_add_software(&b, repeat("\xe2\x82\xac", STUN_TEXT_MAX_CHARS + 1U)),
		      -EINVAL);
	zassert_equal(b.err, 0);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);
	zassert_ok(stun_add_software(&b, repeat("\xe2\x82\xac", STUN_TEXT_MAX_CHARS)));
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE + 4 + 384, "381 bytes, padded");

	start(&b, big_msg, sizeof(big_msg), STUN_BINDING_REQUEST, 0x7C);
	zassert_ok(stun_add_software(&b, repeat("s", STUN_TEXT_MAX_CHARS)));

	/* §14.8: the same limit for the reason phrase. */
	start(&b, big_msg, sizeof(big_msg), STUN_BINDING_ERROR_RESPONSE, 0x7D);
	zassert_equal(stun_add_error_code(&b, 400, repeat("r", STUN_TEXT_MAX_CHARS + 1U)), -EINVAL);
	zassert_equal(
		stun_add_error_code(&b, 400, repeat("\xe2\x82\xac", STUN_TEXT_MAX_CHARS + 1U)),
		-EINVAL);
	zassert_equal(b.err, 0);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);
	zassert_ok(stun_add_error_code(&b, 400, repeat("\xe2\x82\xac", STUN_TEXT_MAX_CHARS)));
	len = stun_builder_len(&b);
	zassert_equal(len, STUN_HEADER_SIZE + 4 + 4 + 384, "the phrase is sent whole");
	zassert_ok(stun_parse(big_msg, (size_t)len, &m));
	zassert_equal(m.error_code, 400U);

	/* A string that is not there. NULL is legal only for the reason phrase. */
	start(&b, big_msg, sizeof(big_msg), STUN_BINDING_REQUEST, 0x7E);
	zassert_equal(stun_add_username(&b, NULL), -EINVAL);
	zassert_equal(stun_add_software(&b, NULL), -EINVAL);
	zassert_equal(b.err, 0);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);
}

ZTEST(stun, test_unknown_attributes_error_byte_exact)
{
	static const uint16_t kTypes[] = {0x7F00U, 0x7F01U, 0x7F02U};
	struct stun_builder b;
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;
	uint8_t msg[128];
	int len;

	/* The 420 response of RFC 8489 §6.3.1, against the independent encoder;
	 * it also pins stun_finish_plain().
	 */
	start(&b, msg, sizeof(msg), STUN_BINDING_ERROR_RESPONSE, 0xB0);
	zassert_ok(stun_add_error_code(&b, 420, "Unknown Attribute"));
	zassert_ok(stun_add_unknown_attributes(&b, kTypes, ARRAY_SIZE(kTypes)));
	len = stun_finish_plain(&b);
	zassert_equal(len, (int)sizeof(kUnknownAttributesError), "length %d", len);
	zassert_mem_equal(msg, kUnknownAttributesError, sizeof(kUnknownAttributesError));

	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_equal(m.error_code, 420U);
	zassert_false(m.has_integrity);
	zassert_ok(stun_verify_fingerprint(msg, (size_t)len, &m));
	zassert_ok(stun_find_attr(msg, (size_t)len, &m, STUN_ATTR_UNKNOWN_ATTRIBUTES, &val, &vlen));
	zassert_equal(vlen, 6);

	/* One and two types: 2 bytes each, padded to 4. An empty list is legal. */
	for (size_t n = 0; n <= 2; n++) {
		start(&b, msg, sizeof(msg), STUN_BINDING_ERROR_RESPONSE, 0xB1);
		zassert_ok(stun_add_unknown_attributes(&b, kTypes, n));
		len = stun_builder_len(&b);
		zassert_equal(len, (int)(STUN_HEADER_SIZE + 4U + ((2U * n + 3U) & ~3U)));
		zassert_ok(stun_parse(msg, (size_t)len, &m));
		zassert_ok(stun_find_attr(msg, (size_t)len, &m, STUN_ATTR_UNKNOWN_ATTRIBUTES, &val,
					  &vlen));
		zassert_equal(vlen, 2U * n);
	}

	start(&b, msg, sizeof(msg), STUN_BINDING_ERROR_RESPONSE, 0xB2);
	zassert_equal(stun_add_unknown_attributes(&b, NULL, 2), -EINVAL);
	zassert_equal(stun_add_unknown_attributes(&b, kTypes, 40000), -EINVAL);
	zassert_equal(b.err, 0);
	zassert_equal(stun_builder_len(&b), STUN_HEADER_SIZE);
}

ZTEST(stun, test_integrity_attributes_byte_exact)
{
	struct stun_builder b;
	uint8_t msg[160];
	int len;

	/* Against the independent encoder, in the three combinations. */
	ensure_psa();

	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0xC0);
	add_check_head(&b);
	zassert_ok(stun_add_integrity(&b, CHECK_KEY, CHECK_KEY_LEN));
	len = stun_builder_len(&b);
	zassert_equal(len, (int)sizeof(kIntegrityOnlyRequest), "length %d", len);
	zassert_mem_equal(msg, kIntegrityOnlyRequest, sizeof(kIntegrityOnlyRequest));

	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0xC0);
	add_check_head(&b);
	zassert_ok(stun_add_integrity_sha256(&b, CHECK_KEY, CHECK_KEY_LEN));
	len = stun_builder_len(&b);
	zassert_equal(len, (int)sizeof(kSha256Request), "length %d", len);
	zassert_mem_equal(msg, kSha256Request, sizeof(kSha256Request));

	start(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, 0xC0);
	add_check_head(&b);
	zassert_ok(stun_add_integrity(&b, CHECK_KEY, CHECK_KEY_LEN));
	zassert_ok(stun_add_integrity_sha256(&b, CHECK_KEY, CHECK_KEY_LEN));
	zassert_ok(stun_add_fingerprint(&b));
	len = stun_builder_len(&b);
	zassert_equal(len, (int)sizeof(kSha256BothRequest), "length %d", len);
	zassert_mem_equal(msg, kSha256BothRequest, sizeof(kSha256BothRequest));
}
