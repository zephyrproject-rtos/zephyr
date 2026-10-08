/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * MESSAGE-INTEGRITY and MESSAGE-INTEGRITY-SHA256 (RFC 8489 §9, §14.5, §14.6).
 */
#include <errno.h>

#include "stun_test.h"

#define CHECK_KEY     ((const uint8_t *)kCheckPwd)
#define CHECK_KEY_LEN (sizeof(kCheckPwd) - 1U)

/* Longer than any key the PSA key store takes: importing it fails. */
static uint8_t huge_key[9000];

/* Offset of the value of the first attribute of `type` in a parsed message. */
static size_t value_offset(const uint8_t *buf, size_t len, uint16_t type)
{
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;

	zassert_ok(stun_parse(buf, len, &m));
	zassert_ok(stun_find_attr(buf, len, &m, type, &val, &vlen));
	zassert_true(vlen > 0);
	return (size_t)(val - buf);
}

ZTEST(stun, test_integrity_of_ipv6_response)
{
	struct stun_msg m;

	/* RFC 5769 §2.3; the other two vectors are checked in main.c. */
	ensure_psa();
	zassert_ok(stun_parse(kResponseV6, sizeof(kResponseV6), &m));
	zassert_ok(stun_verify_integrity(kResponseV6, sizeof(kResponseV6), &m,
					 (const uint8_t *)kPassword, strlen(kPassword)));
	zassert_ok(stun_verify_fingerprint(kResponseV6, sizeof(kResponseV6), &m));
}

ZTEST(stun, test_integrity_detects_a_changed_attribute)
{
	uint8_t msg[sizeof(kRequest)];
	struct stun_msg m;

	ensure_psa();
	memcpy(msg, kRequest, sizeof(kRequest));
	msg[value_offset(msg, sizeof(msg), STUN_ATTR_PRIORITY)] ^= 0x01;
	zassert_ok(stun_parse(msg, sizeof(msg), &m));
	zassert_equal(stun_verify_integrity(msg, sizeof(msg), &m, (const uint8_t *)kPassword,
					    strlen(kPassword)),
		      -EBADMSG);
}

ZTEST(stun, test_integrity_without_fingerprint)
{
	struct stun_msg m;

	ensure_psa();
	zassert_ok(stun_parse(kIntegrityOnlyRequest, sizeof(kIntegrityOnlyRequest), &m));
	zassert_true(m.has_integrity);
	zassert_false(m.has_fingerprint);
	zassert_ok(stun_verify_integrity(kIntegrityOnlyRequest, sizeof(kIntegrityOnlyRequest), &m,
					 CHECK_KEY, CHECK_KEY_LEN));
	zassert_equal(
		stun_verify_fingerprint(kIntegrityOnlyRequest, sizeof(kIntegrityOnlyRequest), &m),
		-ENOENT);
}

ZTEST(stun, test_integrity_bad_arguments)
{
	const uint8_t *req = kSha256BothRequest;
	size_t len = sizeof(kSha256BothRequest);
	struct stun_msg m;

	ensure_psa();
	zassert_ok(stun_parse(req, len, &m));

	zassert_equal(stun_verify_integrity(req, len, &m, NULL, CHECK_KEY_LEN), -EINVAL);
	zassert_equal(stun_verify_integrity(req, len, &m, CHECK_KEY, 0), -EINVAL);
	zassert_equal(stun_verify_integrity(req, len, NULL, CHECK_KEY, CHECK_KEY_LEN), -EINVAL);
	zassert_equal(stun_verify_integrity(NULL, len, &m, CHECK_KEY, CHECK_KEY_LEN), -EINVAL);
	zassert_equal(stun_verify_integrity(req, len - 4U, &m, CHECK_KEY, CHECK_KEY_LEN), -EINVAL,
		      "msg was parsed from another buffer length");

	zassert_equal(stun_verify_integrity_sha256(req, len, &m, NULL, CHECK_KEY_LEN, 32), -EINVAL);
	zassert_equal(stun_verify_integrity_sha256(req, len, &m, CHECK_KEY, 0, 32), -EINVAL);
	zassert_equal(stun_verify_integrity_sha256(req, len, NULL, CHECK_KEY, CHECK_KEY_LEN, 32),
		      -EINVAL);
	zassert_equal(stun_verify_integrity_sha256(NULL, len, &m, CHECK_KEY, CHECK_KEY_LEN, 32),
		      -EINVAL);

	zassert_equal(stun_verify_fingerprint(NULL, len, &m), -EINVAL);
	zassert_equal(stun_verify_fingerprint(req, len, NULL), -EINVAL);
}

ZTEST(stun, test_integrity_backend_failure_is_not_a_mismatch)
{
	const uint8_t *req = kSha256BothRequest;
	size_t len = sizeof(kSha256BothRequest);
	struct stun_msg m;

	/* A caller must be able to tell "forged" from "could not check". */
	ensure_psa();
	zassert_ok(stun_parse(req, len, &m));
	zassert_equal(stun_verify_integrity(req, len, &m, huge_key, sizeof(huge_key)), -EIO);
	zassert_equal(stun_verify_integrity_sha256(req, len, &m, huge_key, sizeof(huge_key), 32),
		      -EIO);
}

ZTEST(stun, test_integrity_sha256)
{
	struct stun_msg m;

	ensure_psa();

	/* MESSAGE-INTEGRITY-SHA256 alone. */
	zassert_ok(stun_parse(kSha256Request, sizeof(kSha256Request), &m));
	zassert_true(m.has_integrity_sha256);
	zassert_equal(m.integrity_sha256_len, STUN_INTEGRITY_SHA256_MAX_LEN);
	zassert_false(m.has_integrity);
	zassert_ok(stun_verify_integrity_sha256(kSha256Request, sizeof(kSha256Request), &m,
						CHECK_KEY, CHECK_KEY_LEN, 32));
	zassert_equal(stun_verify_integrity(kSha256Request, sizeof(kSha256Request), &m, CHECK_KEY,
					    CHECK_KEY_LEN),
		      -ENOENT);

	/* Both attributes and FINGERPRINT, as RFC 8489 §9.1.2 has a request. */
	zassert_ok(stun_parse(kSha256BothRequest, sizeof(kSha256BothRequest), &m));
	zassert_ok(stun_verify_integrity(kSha256BothRequest, sizeof(kSha256BothRequest), &m,
					 CHECK_KEY, CHECK_KEY_LEN));
	zassert_ok(stun_verify_integrity_sha256(kSha256BothRequest, sizeof(kSha256BothRequest), &m,
						CHECK_KEY, CHECK_KEY_LEN, 32));
	zassert_ok(stun_verify_fingerprint(kSha256BothRequest, sizeof(kSha256BothRequest), &m));

	/* A message without it. */
	zassert_ok(stun_parse(kRequest, sizeof(kRequest), &m));
	zassert_equal(stun_verify_integrity_sha256(kRequest, sizeof(kRequest), &m,
						   (const uint8_t *)kPassword, strlen(kPassword),
						   32),
		      -ENOENT);
}

ZTEST(stun, test_integrity_sha256_detects_changes)
{
	uint8_t msg[sizeof(kSha256Request)];
	struct stun_msg m;

	ensure_psa();

	memcpy(msg, kSha256Request, sizeof(msg));
	zassert_ok(stun_parse(msg, sizeof(msg), &m));
	msg[m.integrity_sha256_off + 4U] ^= 0x01;
	zassert_equal(
		stun_verify_integrity_sha256(msg, sizeof(msg), &m, CHECK_KEY, CHECK_KEY_LEN, 32),
		-EBADMSG, "a bit of the value");

	memcpy(msg, kSha256Request, sizeof(msg));
	msg[value_offset(msg, sizeof(msg), STUN_ATTR_PRIORITY)] ^= 0x01;
	zassert_ok(stun_parse(msg, sizeof(msg), &m));
	zassert_equal(
		stun_verify_integrity_sha256(msg, sizeof(msg), &m, CHECK_KEY, CHECK_KEY_LEN, 32),
		-EBADMSG, "a bit of a signed attribute");

	zassert_ok(stun_parse(kSha256Request, sizeof(kSha256Request), &m));
	zassert_equal(stun_verify_integrity_sha256(kSha256Request, sizeof(kSha256Request), &m,
						   (const uint8_t *)"wrongpass", 9, 32),
		      -EBADMSG, "another key");
}

ZTEST(stun, test_integrity_sha256_truncated)
{
	static const size_t kBadMin[] = {0, 12, 17, 36};
	uint8_t msg[sizeof(kSha256TruncatedRequest)];
	const uint8_t *req = kSha256TruncatedRequest;
	size_t len = sizeof(kSha256TruncatedRequest);
	struct stun_msg m;

	ensure_psa();
	zassert_ok(stun_parse(req, len, &m));
	zassert_equal(m.integrity_sha256_len, STUN_INTEGRITY_SHA256_MIN_LEN);

	/* RFC 8489 §14.6: the full 32 bytes unless the usage allows truncation. */
	zassert_equal(stun_verify_integrity_sha256(req, len, &m, CHECK_KEY, CHECK_KEY_LEN, 32),
		      -EBADMSG, "truncated, and the caller did not allow it");
	zassert_ok(stun_verify_integrity_sha256(req, len, &m, CHECK_KEY, CHECK_KEY_LEN, 16));

	memcpy(msg, req, len);
	msg[m.integrity_sha256_off + 4U + 15U] ^= 0x80;
	zassert_equal(stun_verify_integrity_sha256(msg, len, &m, CHECK_KEY, CHECK_KEY_LEN, 16),
		      -EBADMSG, "the last bit of the truncated value");

	for (size_t i = 0; i < ARRAY_SIZE(kBadMin); i++) {
		zassert_equal(stun_verify_integrity_sha256(req, len, &m, CHECK_KEY, CHECK_KEY_LEN,
							   kBadMin[i]),
			      -EINVAL, "min_len %zu", kBadMin[i]);
	}
}

ZTEST(stun, test_integrity_sha256_rfc8489_vector)
{
	/* Username U+30DE U+30C8 U+30EA U+30C3 U+30AF U+30B9, realm and password
	 * of RFC 8489 Appendix B.1; with the SHA-256 password algorithm the key
	 * is SHA-256(username ":" realm ":" password) (RFC 8489 §9.2.2).
	 */
	static const uint8_t kKeyInput[] = "\xe3\x83\x9e\xe3\x83\x88\xe3\x83\xaa\xe3\x83\x83"
					   "\xe3\x82\xaf\xe3\x82\xb9:example.org:TheMatrIX";
	uint8_t key[PSA_HASH_LENGTH(PSA_ALG_SHA_256)];
	uint16_t unknown[4] = {0};
	struct stun_msg m;
	size_t key_len = 0;

	ensure_psa();
	zassert_equal(psa_hash_compute(PSA_ALG_SHA_256, kKeyInput, sizeof(kKeyInput) - 1U, key,
				       sizeof(key), &key_len),
		      PSA_SUCCESS);
	zassert_equal(key_len, sizeof(key));

	zassert_true(stun_is_message(kRfc8489Request, sizeof(kRfc8489Request)));
	zassert_ok(stun_parse(kRfc8489Request, sizeof(kRfc8489Request), &m));
	zassert_true(m.has_integrity_sha256);
	zassert_false(m.has_integrity);
	zassert_ok(stun_verify_integrity_sha256(kRfc8489Request, sizeof(kRfc8489Request), &m, key,
						sizeof(key), 32));

	/* The codec does not implement long-term credentials: USERHASH and
	 * PASSWORD-ALGORITHM are for whoever does.
	 */
	zassert_equal(stun_unknown_attrs(kRfc8489Request, sizeof(kRfc8489Request), &m, NULL, 0,
					 unknown, ARRAY_SIZE(unknown)),
		      2);
	zassert_equal(unknown[0], 0x001EU, "USERHASH");
	zassert_equal(unknown[1], 0x001DU, "PASSWORD-ALGORITHM");
}

ZTEST(stun, test_attributes_after_integrity_sha256_are_ignored)
{
	uint8_t msg[sizeof(kSha256Request) + 4U + 8U];
	struct stun_msg m;
	size_t len = sizeof(kSha256Request);

	/* The forgery of parse.c, on a message that MESSAGE-INTEGRITY-SHA256
	 * protects: USE-CANDIDATE inserted after it, then a fresh FINGERPRINT.
	 */
	ensure_psa();
	memcpy(msg, kSha256Request, len);
	put16(msg + len, STUN_ATTR_USE_CANDIDATE);
	put16(msg + len + 2, 0);
	len += 4U;
	put16(msg + 2, (uint16_t)(len + 8U - STUN_HEADER_SIZE));
	put16(msg + len, STUN_ATTR_FINGERPRINT);
	put16(msg + len + 2, 4);
	put32(msg + len + 4, crc32_ieee(msg, len) ^ FINGERPRINT_XOR);
	len += 8U;

	zassert_ok(stun_parse(msg, len, &m));
	zassert_ok(stun_verify_integrity_sha256(msg, len, &m, CHECK_KEY, CHECK_KEY_LEN, 32),
		   "the forged message must still pass MESSAGE-INTEGRITY-SHA256");
	zassert_ok(stun_verify_fingerprint(msg, len, &m),
		   "the forged message must still pass FINGERPRINT");
	zassert_false(m.has_use_candidate);
	zassert_true(m.has_priority);
}
