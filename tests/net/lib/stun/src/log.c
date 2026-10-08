/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * What the codec writes to the log, and what it must never write: traffic from
 * the network can make debug records only, and neither keys nor the text of
 * attributes get into any record.
 */
#include <errno.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "log_capture.h"
#include "stun_test.h"

/* What the module writes depends on the level it is built with, and on the
 * ceiling the system sets for every module.
 */
#define LEVEL  MIN(CONFIG_STUN_LOG_LEVEL, CONFIG_LOG_MAX_LEVEL)
#define DBG_ON (LEVEL >= LOG_LEVEL_DBG)
#define ERR_ON (LEVEL >= LOG_LEVEL_ERR)

/* Text that is easy to find, should it ever get into a record. */
#define SECRET_KEY   "LOGSECRET-0123456789"
#define WRONG_KEY    "WRONGSECRET-9876543210"
#define USERNAME     "LOGUSER:LOGPEER"
#define SOFTWARE     "LOGSOFTWARE 1.0"
#define KEY_FRAGMENT "HUGEKEY"

/* Longer than any key the PSA key store takes: importing it fails. */
static uint8_t huge_key[9000];

static void *suite_setup(void)
{
	for (size_t i = 0; i < sizeof(huge_key); i++) {
		huge_key[i] = (uint8_t)KEY_FRAGMENT[i % (sizeof(KEY_FRAGMENT) - 1U)];
	}
	log_capture_start();
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	ensure_psa();
	log_capture_clear();
}

ZTEST_SUITE(stun_log, NULL, suite_setup, before_each, NULL, NULL);

/* The records that are more severe than a debug one. */
static size_t above_debug(void)
{
	return log_capture_count() - log_capture_count_level(LOG_LEVEL_DBG);
}

/* A malformed message is refused, and that is one debug record or none. */
static void expect_refused(const uint8_t *buf, size_t len, const char *what)
{
	struct stun_msg m;

	log_capture_clear();
	zassert_equal(stun_parse(buf, len, &m), -EBADMSG, "%s", what);
	if (log_capture_count() != (DBG_ON ? 1U : 0U) || above_debug() != 0U) {
		log_capture_dump();
	}
	zassert_equal(log_capture_count(), DBG_ON ? 1U : 0U, "%s: %u records", what,
		      (unsigned int)log_capture_count());
	zassert_equal(above_debug(), 0U, "%s: a record above the debug level", what);
}

ZTEST(stun_log, test_a_refused_message_is_one_debug_record)
{
	static const uint8_t kZero[40];
	static const uint8_t kFamily3[8] = {0x00, 0x03};
	/* class 7, which no error code has */
	static const uint8_t kErrorClass7[4] = {0x00, 0x00, 0x07, 0x00};
	struct raw_msg r;

	/* One message for every family of rules the parser has. */

	/* The header: the message does not fill the datagram. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x01);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	expect_refused(r.buf, r.len + 4U, "a datagram longer than its message");

	/* An attribute that runs past the end of the message. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x02);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	put16(r.buf + STUN_HEADER_SIZE + 2, 8);
	expect_refused(r.buf, r.len, "an attribute longer than the message");

	/* A known attribute of a length it cannot have. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x03);
	raw_attr(&r, STUN_ATTR_PRIORITY, 3, kZero, 3);
	expect_refused(r.buf, r.len, "PRIORITY of 3 bytes");

	raw_init(&r, STUN_BINDING_REQUEST, 0x04);
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY, 16, kZero, 16);
	expect_refused(r.buf, r.len, "MESSAGE-INTEGRITY of 16 bytes");

	raw_init(&r, STUN_BINDING_REQUEST, 0x05);
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, 12, kZero, 12);
	expect_refused(r.buf, r.len, "MESSAGE-INTEGRITY-SHA256 of 12 bytes");

	/* A value that cannot be: an address family, an error class. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, 0x06);
	raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, sizeof(kFamily3), kFamily3, sizeof(kFamily3));
	expect_refused(r.buf, r.len, "address family 3");

	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, 0x07);
	raw_attr(&r, STUN_ATTR_ERROR_CODE, sizeof(kErrorClass7), kErrorClass7,
		 sizeof(kErrorClass7));
	expect_refused(r.buf, r.len, "error class 7");

	/* The order of the attributes: FINGERPRINT is the last one. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x08);
	raw_fingerprint(&r);
	raw_u32(&r, STUN_ATTR_PRIORITY, 4, 1);
	expect_refused(r.buf, r.len, "FINGERPRINT before PRIORITY");

	/* What ICE forbids: both roles at once. */
	raw_init(&r, STUN_BINDING_REQUEST, 0x09);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, 8, CONTROLLING_TB);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, 8, CONTROLLED_TB);
	expect_refused(r.buf, r.len, "both ICE roles");
}

ZTEST(stun_log, test_what_is_not_stun_leaves_no_record)
{
	/* The first bytes of an RTP and of a DTLS packet: on a socket that STUN
	 * shares they are the bulk of the traffic, and none of the log's
	 * business.
	 */
	static const uint8_t kRtp[24] = {0x80, 0x60, 0x12, 0x34};
	static const uint8_t kDtls[24] = {0x16, 0xfe, 0xfd, 0x00};
	struct stun_msg m;

	zassert_false(stun_is_message(kRtp, sizeof(kRtp)));
	zassert_false(stun_is_message(kDtls, sizeof(kDtls)));
	zassert_equal(stun_parse(kRtp, sizeof(kRtp), &m), -EINVAL);
	zassert_equal(stun_parse(kDtls, sizeof(kDtls), &m), -EINVAL);
	zassert_equal(log_capture_count(), 0U);
}

ZTEST(stun_log, test_valid_messages_leave_no_record)
{
	struct net_sockaddr addr;
	struct stun_builder b;
	struct stun_msg m;
	uint8_t msg[128];
	uint8_t txid[STUN_TXID_SIZE];
	int len;

	/* Parsing and checking what RFC 5769 has as its samples. */
	zassert_ok(stun_parse(kRequest, sizeof(kRequest), &m));
	zassert_ok(stun_verify_integrity(kRequest, sizeof(kRequest), &m, (const uint8_t *)kPassword,
					 strlen(kPassword)));
	zassert_ok(stun_verify_fingerprint(kRequest, sizeof(kRequest), &m));
	zassert_ok(stun_parse(kResponse, sizeof(kResponse), &m));
	zassert_ok(stun_get_xor_mapped_address(kResponse, sizeof(kResponse), &m, &addr));

	/* Building a message, and reading it back. */
	fill_txid(txid, 0x40);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, USERNAME));
	zassert_ok(stun_add_priority(&b, 1));
	len = stun_finish(&b, (const uint8_t *)SECRET_KEY, strlen(SECRET_KEY));
	zassert_true(len > 0);
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_ok(stun_verify_integrity(msg, (size_t)len, &m, (const uint8_t *)SECRET_KEY,
					 strlen(SECRET_KEY)));

	if (log_capture_count() != 0U) {
		log_capture_dump();
	}
	zassert_equal(log_capture_count(), 0U);
}

ZTEST(stun_log, test_a_forged_message_is_not_an_error)
{
	uint8_t msg[sizeof(kRequest)];
	struct stun_msg m;

	/* A signature that does not match and a fingerprint that is wrong are
	 * properties of somebody else's message, not faults of this system.
	 */
	memcpy(msg, kRequest, sizeof(msg));
	zassert_ok(stun_parse(msg, sizeof(msg), &m));
	zassert_equal(stun_verify_integrity(msg, sizeof(msg), &m, (const uint8_t *)WRONG_KEY,
					    strlen(WRONG_KEY)),
		      -EBADMSG);

	msg[sizeof(msg) - 1U] ^= 0x01;
	zassert_ok(stun_parse(msg, sizeof(msg), &m));
	zassert_equal(stun_verify_fingerprint(msg, sizeof(msg), &m), -EBADMSG);

	zassert_equal(above_debug(), 0U, "a record above the debug level");
}

ZTEST(stun_log, test_a_failing_backend_is_one_error_record)
{
	struct stun_builder b;
	struct stun_msg m;
	uint8_t msg[128];
	uint8_t txid[STUN_TXID_SIZE];

	/* The status of the PSA backend is lost in -EIO; the log has it. */
	zassert_ok(stun_parse(kRequest, sizeof(kRequest), &m));
	log_capture_clear();
	zassert_equal(
		stun_verify_integrity(kRequest, sizeof(kRequest), &m, huge_key, sizeof(huge_key)),
		-EIO);
	zassert_equal(log_capture_count(), ERR_ON ? 1U : 0U);
	zassert_equal(log_capture_count_level(LOG_LEVEL_ERR), log_capture_count());
	if (ERR_ON) {
		zassert_true(log_capture_contains("PSA"));
	}
	zassert_false(log_capture_contains(KEY_FRAGMENT), "the key is in the log");

	/* The same when signing. */
	fill_txid(txid, 0x41);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	log_capture_clear();
	zassert_equal(stun_add_integrity(&b, huge_key, sizeof(huge_key)), -EIO);
	zassert_equal(log_capture_count(), ERR_ON ? 1U : 0U);
	zassert_equal(log_capture_count_level(LOG_LEVEL_ERR), log_capture_count());
	zassert_false(log_capture_contains(KEY_FRAGMENT), "the key is in the log");
}

ZTEST(stun_log, test_keys_and_attribute_text_stay_out_of_the_log)
{
	char mac_lower[13], mac_upper[13];
	struct stun_builder b;
	struct stun_msg m;
	const uint8_t *mac;
	uint8_t msg[160];
	uint8_t txid[STUN_TXID_SIZE];
	size_t username_at;
	int len;

	/* A request with text in it, signed with a key. */
	fill_txid(txid, 0x42);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, USERNAME));
	zassert_ok(stun_add_software(&b, SOFTWARE));
	zassert_ok(stun_add_priority(&b, 1));
	len = stun_finish(&b, (const uint8_t *)SECRET_KEY, strlen(SECRET_KEY));
	zassert_true(len > 0);
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_true(m.has_integrity);

	/* The first bytes of its MESSAGE-INTEGRITY value, as a log would print them. */
	mac = msg + m.integrity_off + 4U;
	snprintf(mac_lower, sizeof(mac_lower), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2],
		 mac[3], mac[4], mac[5]);
	snprintf(mac_upper, sizeof(mac_upper), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2],
		 mac[3], mac[4], mac[5]);

	log_capture_clear();

	/* Everything the codec may do with it, and everything that goes wrong. */
	zassert_ok(stun_verify_integrity(msg, (size_t)len, &m, (const uint8_t *)SECRET_KEY,
					 strlen(SECRET_KEY)));
	zassert_equal(stun_verify_integrity(msg, (size_t)len, &m, (const uint8_t *)WRONG_KEY,
					    strlen(WRONG_KEY)),
		      -EBADMSG);
	zassert_equal(stun_verify_integrity(msg, (size_t)len, &m, huge_key, sizeof(huge_key)),
		      -EIO);

	/* The datagram cut short, and a USERNAME that says it is longer than the
	 * message, twice over: the text is in what the parser refuses.
	 */
	zassert_equal(stun_parse(msg, (size_t)len - 4U, &m), -EBADMSG);
	username_at = STUN_HEADER_SIZE;
	zassert_equal(sys_get_be16(msg + username_at), STUN_ATTR_USERNAME);
	put16(msg + username_at + 2, 0x7FFC);
	zassert_equal(stun_parse(msg, (size_t)len, &m), -EBADMSG);
	put16(msg + username_at + 2, 800);
	zassert_equal(stun_parse(msg, (size_t)len, &m), -EBADMSG);

	/* There are records to look at, or the absence below says nothing. */
	if (DBG_ON) {
		zassert_true(log_capture_count() >= 4U, "%u records",
			     (unsigned int)log_capture_count());
	}

	zassert_false(log_capture_contains("LOGSECRET"), "the key is in the log");
	zassert_false(log_capture_contains("WRONGSECRET"), "the key that was tried is in the log");
	zassert_false(log_capture_contains(KEY_FRAGMENT), "the key is in the log");
	zassert_false(log_capture_contains("LOGUSER"), "USERNAME is in the log");
	zassert_false(log_capture_contains("LOGSOFT"), "SOFTWARE is in the log");
	zassert_false(log_capture_contains(mac_lower), "the integrity value is in the log");
	zassert_false(log_capture_contains(mac_upper), "the integrity value is in the log");
}
