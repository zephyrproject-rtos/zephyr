/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/stun.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <psa/crypto.h>

/* Transaction id first, first + 1, ...: twelve different bytes. */
static void fill_txid(uint8_t txid[STUN_TXID_SIZE], uint8_t first)
{
	for (size_t i = 0; i < STUN_TXID_SIZE; i++) {
		txid[i] = (uint8_t)(first + i);
	}
}

/* RFC 5769 section 2.1: the password the sample messages are signed with,
 * used as the MESSAGE-INTEGRITY key.
 */
static const char rfc5769_password[] = "VOkJxbRl1RmTxUk/WvJxBt";

/* clang-format off */
/* RFC 5769 section 2.1: sample Binding Request from a controlled ICE agent,
 * with SOFTWARE "STUN test client", USERNAME "evtj:h6vY", PRIORITY,
 * ICE-CONTROLLED, MESSAGE-INTEGRITY and FINGERPRINT. USERNAME is padded with
 * spaces (0x20); padding may take any value (RFC 5769 section 2), so a
 * receiver must not look at it.
 */
static const uint8_t rfc5769_request[] = {
	0x00, 0x01, 0x00, 0x58, 0x21, 0x12, 0xa4, 0x42,
	0xb7, 0xe7, 0xa7, 0x01, 0xbc, 0x34, 0xd6, 0x86,
	0xfa, 0x87, 0xdf, 0xae, 0x80, 0x22, 0x00, 0x10,
	0x53, 0x54, 0x55, 0x4e, 0x20, 0x74, 0x65, 0x73,
	0x74, 0x20, 0x63, 0x6c, 0x69, 0x65, 0x6e, 0x74,
	0x00, 0x24, 0x00, 0x04, 0x6e, 0x00, 0x01, 0xff,
	0x80, 0x29, 0x00, 0x08, 0x93, 0x2f, 0xf9, 0xb1,
	0x51, 0x26, 0x3b, 0x36, 0x00, 0x06, 0x00, 0x09,
	0x65, 0x76, 0x74, 0x6a, 0x3a, 0x68, 0x36, 0x76,
	0x59, 0x20, 0x20, 0x20, 0x00, 0x08, 0x00, 0x14,
	0x9a, 0xea, 0xa7, 0x0c, 0xbf, 0xd8, 0xcb, 0x56,
	0x78, 0x1e, 0xf2, 0xb5, 0xb2, 0xd3, 0xf2, 0x49,
	0xc1, 0xb5, 0x71, 0xa2, 0x80, 0x28, 0x00, 0x04,
	0xe5, 0x7a, 0x3b, 0xcf,
};

/* RFC 5769 section 2.2: sample IPv4 Binding Success Response, with SOFTWARE
 * "test vector", XOR-MAPPED-ADDRESS 192.0.2.1 port 32853, MESSAGE-INTEGRITY
 * and FINGERPRINT. The transaction id is that of rfc5769_request.
 */
static const uint8_t rfc5769_response[] = {
	0x01, 0x01, 0x00, 0x3c, 0x21, 0x12, 0xa4, 0x42,
	0xb7, 0xe7, 0xa7, 0x01, 0xbc, 0x34, 0xd6, 0x86,
	0xfa, 0x87, 0xdf, 0xae, 0x80, 0x22, 0x00, 0x0b,
	0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63,
	0x74, 0x6f, 0x72, 0x20, 0x00, 0x20, 0x00, 0x08,
	0x00, 0x01, 0xa1, 0x47, 0xe1, 0x12, 0xa6, 0x43,
	0x00, 0x08, 0x00, 0x14, 0x2b, 0x91, 0xf5, 0x99,
	0xfd, 0x9e, 0x90, 0xc3, 0x8c, 0x74, 0x89, 0xf9,
	0x2a, 0xf9, 0xba, 0x53, 0xf0, 0x6b, 0xe7, 0xd7,
	0x80, 0x28, 0x00, 0x04, 0xc0, 0x7d, 0x4c, 0x96,
};

/* RFC 5769 section 2.3: sample IPv6 Binding Success Response, with SOFTWARE
 * "test vector", XOR-MAPPED-ADDRESS 2001:db8:1234:5678:11:2233:4455:6677 port
 * 32853, MESSAGE-INTEGRITY and FINGERPRINT. The transaction id is that of
 * rfc5769_request.
 */
static const uint8_t rfc5769_response_ipv6[] = {
	0x01, 0x01, 0x00, 0x48, 0x21, 0x12, 0xa4, 0x42,
	0xb7, 0xe7, 0xa7, 0x01, 0xbc, 0x34, 0xd6, 0x86,
	0xfa, 0x87, 0xdf, 0xae, 0x80, 0x22, 0x00, 0x0b,
	0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63,
	0x74, 0x6f, 0x72, 0x20, 0x00, 0x20, 0x00, 0x14,
	0x00, 0x02, 0xa1, 0x47, 0x01, 0x13, 0xa9, 0xfa,
	0xa5, 0xd3, 0xf1, 0x79, 0xbc, 0x25, 0xf4, 0xb5,
	0xbe, 0xd2, 0xb9, 0xd9, 0x00, 0x08, 0x00, 0x14,
	0xa3, 0x82, 0x95, 0x4e, 0x4b, 0xe6, 0x7b, 0xf1,
	0x17, 0x84, 0xc9, 0x7c, 0x82, 0x92, 0xc2, 0x75,
	0xbf, 0xe3, 0xed, 0x41, 0x80, 0x28, 0x00, 0x04,
	0xc8, 0xfb, 0x0b, 0x4c,
};

/* RFC 5769 section 2.4: sample Binding Request with long-term credentials,
 * with USERNAME, NONCE "f//499k954d6OL34oL9FSTvy64sA", REALM "example.org"
 * and MESSAGE-INTEGRITY. Unlike sections 2.1 to 2.3, it is padded with zeros,
 * as RFC 8489 section 14 requires of a sender, so a builder can produce it
 * byte for byte.
 */
static const uint8_t rfc5769_long_term_request[] = {
	0x00, 0x01, 0x00, 0x60, 0x21, 0x12, 0xa4, 0x42,
	0x78, 0xad, 0x34, 0x33, 0xc6, 0xad, 0x72, 0xc0,
	0x29, 0xda, 0x41, 0x2e, 0x00, 0x06, 0x00, 0x12,
	0xe3, 0x83, 0x9e, 0xe3, 0x83, 0x88, 0xe3, 0x83,
	0xaa, 0xe3, 0x83, 0x83, 0xe3, 0x82, 0xaf, 0xe3,
	0x82, 0xb9, 0x00, 0x00, 0x00, 0x15, 0x00, 0x1c,
	0x66, 0x2f, 0x2f, 0x34, 0x39, 0x39, 0x6b, 0x39,
	0x35, 0x34, 0x64, 0x36, 0x4f, 0x4c, 0x33, 0x34,
	0x6f, 0x4c, 0x39, 0x46, 0x53, 0x54, 0x76, 0x79,
	0x36, 0x34, 0x73, 0x41, 0x00, 0x14, 0x00, 0x0b,
	0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e,
	0x6f, 0x72, 0x67, 0x00, 0x00, 0x08, 0x00, 0x14,
	0xf6, 0x70, 0x24, 0x65, 0x6d, 0xd6, 0x4a, 0x3e,
	0x02, 0xb8, 0xe0, 0x71, 0x2e, 0x85, 0xc9, 0xa2,
	0x8c, 0xa8, 0x96, 0x66,
};

/* RFC 8489 Appendix B.1: sample Binding Request with long-term credentials and
 * MESSAGE-INTEGRITY-SHA256, as corrected by verified erratum 6268 (the printed
 * vector counts the header in its Length and lacks PASSWORD-ALGORITHM): USERHASH,
 * NONCE, REALM "example.org", PASSWORD-ALGORITHM SHA-256 and
 * MESSAGE-INTEGRITY-SHA256 of 32 bytes.
 */
static const uint8_t rfc8489_request[] = {
	0x00, 0x01, 0x00, 0x90, 0x21, 0x12, 0xa4, 0x42,
	0x78, 0xad, 0x34, 0x33, 0xc6, 0xad, 0x72, 0xc0,
	0x29, 0xda, 0x41, 0x2e, 0x00, 0x1e, 0x00, 0x20,
	0x4a, 0x3c, 0xf3, 0x8f, 0xef, 0x69, 0x92, 0xbd,
	0xa9, 0x52, 0xc6, 0x78, 0x04, 0x17, 0xda, 0x0f,
	0x24, 0x81, 0x94, 0x15, 0x56, 0x9e, 0x60, 0xb2,
	0x05, 0xc4, 0x6e, 0x41, 0x40, 0x7f, 0x17, 0x04,
	0x00, 0x15, 0x00, 0x29, 0x6f, 0x62, 0x4d, 0x61,
	0x74, 0x4a, 0x6f, 0x73, 0x32, 0x41, 0x41, 0x41,
	0x43, 0x66, 0x2f, 0x2f, 0x34, 0x39, 0x39, 0x6b,
	0x39, 0x35, 0x34, 0x64, 0x36, 0x4f, 0x4c, 0x33,
	0x34, 0x6f, 0x4c, 0x39, 0x46, 0x53, 0x54, 0x76,
	0x79, 0x36, 0x34, 0x73, 0x41, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x00, 0x0b, 0x65, 0x78, 0x61, 0x6d,
	0x70, 0x6c, 0x65, 0x2e, 0x6f, 0x72, 0x67, 0x00,
	0x00, 0x1d, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00,
	0x00, 0x1c, 0x00, 0x20, 0xb5, 0xc7, 0xbf, 0x00,
	0x5b, 0x6c, 0x52, 0xa2, 0x1c, 0x51, 0xc5, 0xe8,
	0x92, 0xf8, 0x19, 0x24, 0x13, 0x62, 0x96, 0xcb,
	0x92, 0x7c, 0x43, 0x14, 0x93, 0x09, 0x27, 0x8c,
	0xc6, 0x51, 0x8e, 0x65,
};
/* clang-format on */

/* The long-term key of rfc8489_request. With the SHA-256 password algorithm it
 * is SHA-256(username ":" realm ":" password) (RFC 8489 section 18.5.1.2); the
 * codec does not derive long-term keys, so the tests do, with PSA.
 */
static void rfc8489_long_term_key(uint8_t key[PSA_HASH_LENGTH(PSA_ALG_SHA_256)])
{
	/* The six katakana characters of the RFC in UTF-8, the realm, and the
	 * password, joined by ':'.
	 */
	static const uint8_t key_input[] = "\xe3\x83\x9e\xe3\x83\x88\xe3\x83\xaa\xe3\x83\x83"
					   "\xe3\x82\xaf\xe3\x82\xb9:example.org:TheMatrIX";
	size_t key_len = 0U;

	zassert_equal(psa_hash_compute(PSA_ALG_SHA_256, key_input, sizeof(key_input) - 1U, key,
				       PSA_HASH_LENGTH(PSA_ALG_SHA_256), &key_len), PSA_SUCCESS);
	zassert_equal(key_len, PSA_HASH_LENGTH(PSA_ALG_SHA_256));
}

/* Set the address to IPv4 a.b.c.d with this port. */
static void addr_set_ipv4(struct net_sockaddr *addr, uint8_t a, uint8_t b, uint8_t c,
				 uint8_t d, uint16_t port)
{
	struct net_sockaddr_in *sin4 = net_sin(addr);

	memset(addr, 0, sizeof(*addr));
	sin4->sin_family = NET_AF_INET;
	sin4->sin_port = net_htons(port);
	sin4->sin_addr.s4_addr[0] = a;
	sin4->sin_addr.s4_addr[1] = b;
	sin4->sin_addr.s4_addr[2] = c;
	sin4->sin_addr.s4_addr[3] = d;
}

/* Is the address IPv4 a.b.c.d with this port? */
static bool addr_is_ipv4(const struct net_sockaddr *addr, uint8_t a, uint8_t b, uint8_t c,
				uint8_t d, uint16_t port)
{
	const struct net_sockaddr_in *sin4 = net_sin(addr);
	const uint8_t want[NET_IPV4_ADDR_SIZE] = {a, b, c, d};

	if (sin4->sin_family != NET_AF_INET) {
		return false;
	}
	if (sin4->sin_port != net_htons(port)) {
		return false;
	}
	if (memcmp(sin4->sin_addr.s4_addr, want, sizeof(want)) != 0) {
		return false;
	}

	return true;
}

/* Set the address to IPv6 ip with this port. */
static void addr_set_ipv6(struct net_sockaddr *addr, const uint8_t ip[NET_IPV6_ADDR_SIZE],
				 uint16_t port)
{
	struct net_sockaddr_in6 *sin6 = net_sin6(addr);

	memset(addr, 0, sizeof(*addr));
	sin6->sin6_family = NET_AF_INET6;
	sin6->sin6_port = net_htons(port);
	memcpy(sin6->sin6_addr.s6_addr, ip, NET_IPV6_ADDR_SIZE);
}

/* Is the address IPv6 ip with this port? */
static bool addr_is_ipv6(const struct net_sockaddr *addr,
				const uint8_t ip[NET_IPV6_ADDR_SIZE], uint16_t port)
{
	const struct net_sockaddr_in6 *sin6 = net_sin6(addr);

	if (sin6->sin6_family != NET_AF_INET6) {
		return false;
	}
	if (sin6->sin6_port != net_htons(port)) {
		return false;
	}
	if (memcmp(sin6->sin6_addr.s6_addr, ip, NET_IPV6_ADDR_SIZE) != 0) {
		return false;
	}

	return true;
}

/* Raw assembler: messages written byte by byte, without the library, for the
 * shapes its builder does not produce.
 */

/* Fields of the message header (RFC 8489 section 5): the type at 0, then the
 * Length, the magic cookie and the transaction id.
 */
#define RAW_LENGTH_OFFSET 2U
#define RAW_COOKIE_OFFSET 4U
#define RAW_TXID_OFFSET   8U

/* Fields of an attribute (RFC 8489 section 14): the type, the Length of the
 * value, then the value, padded to a multiple of 4 bytes.
 */
#define RAW_ATTR_LENGTH_OFFSET 2U
#define RAW_ATTR_HEADER_SIZE   4U
/* Attributes are padded to 32-bit words (RFC 8489 section 14). */
#define RAW_WORD_SIZE          4U

/* Attributes on the wire: the 4-byte header and the value. */
#define RAW_PRIORITY_SIZE    (RAW_ATTR_HEADER_SIZE + sizeof(uint32_t))
#define RAW_INTEGRITY_SIZE   (RAW_ATTR_HEADER_SIZE + STUN_INTEGRITY_LEN)
#define RAW_FINGERPRINT_SIZE (RAW_ATTR_HEADER_SIZE + sizeof(uint32_t))

/* The value of ERROR-CODE: the code in 4 bytes, then the reason phrase
 * (RFC 8489 section 14.8).
 */
#define RAW_ERROR_REASON_OFFSET 4U

struct raw_msg {
	uint8_t buf[1200];
	/* Bytes written. */
	size_t len;
};

/* Start a message: the header with an empty attribute area, and every byte of
 * the transaction id set to txid_fill.
 */
static void raw_init(struct raw_msg *r, uint16_t type, uint8_t txid_fill)
{
	memset(r, 0, sizeof(*r));
	sys_put_be16(type, r->buf);
	sys_put_be16(0U, r->buf + RAW_LENGTH_OFFSET);
	sys_put_be32((uint32_t)STUN_MAGIC_COOKIE, r->buf + RAW_COOKIE_OFFSET);
	memset(r->buf + RAW_TXID_OFFSET, txid_fill, STUN_TXID_SIZE);
	r->len = STUN_HEADER_SIZE;
}

/* Append one attribute. attr_len goes in its Length field and may differ from
 * value_len on purpose. The value area is attr_len rounded up to 4 bytes, zero
 * past value_len, and the header Length counts everything appended so far.
 */
static void raw_attr(struct raw_msg *r, uint16_t attr_type, uint16_t attr_len,
			    const void *value, size_t value_len)
{
	size_t padded = ROUND_UP(attr_len, RAW_WORD_SIZE);

	zassert_true(r->len + RAW_ATTR_HEADER_SIZE + padded <= sizeof(r->buf), "raw_msg overflow");
	zassert_true(value_len <= padded, "the value does not fit its attribute");
	sys_put_be16(attr_type, r->buf + r->len);
	sys_put_be16(attr_len, r->buf + r->len + RAW_ATTR_LENGTH_OFFSET);
	if (value_len != 0U) {
		memcpy(r->buf + r->len + RAW_ATTR_HEADER_SIZE, value, value_len);
	}
	r->len += RAW_ATTR_HEADER_SIZE + padded;
	sys_put_be16((uint16_t)(r->len - STUN_HEADER_SIZE), r->buf + RAW_LENGTH_OFFSET);
}

/* Append an attribute whose value is a 32-bit number in network byte order. */
static void raw_u32(struct raw_msg *r, uint16_t attr_type, uint16_t attr_len,
			   uint32_t value)
{
	uint8_t bytes[sizeof(uint32_t)] = {0};

	sys_put_be32(value, bytes);
	raw_attr(r, attr_type, attr_len, bytes, sizeof(bytes));
}

/* Append an attribute whose value is a 64-bit number in network byte order. */
static void raw_u64(struct raw_msg *r, uint16_t attr_type, uint16_t attr_len,
			   uint64_t value)
{
	uint8_t bytes[sizeof(uint64_t)] = {0};

	sys_put_be64(value, bytes);
	raw_attr(r, attr_type, attr_len, bytes, sizeof(bytes));
}

/* FINGERPRINT is the CRC-32 of the message ahead of it XORed with this value,
 * "STUN" in ASCII (RFC 8489 section 14.7).
 */
#define RAW_FINGERPRINT_XOR 0x5354554eU

/* Append a correct FINGERPRINT: the CRC-32 of the message up to it, with the
 * header Length already counting it. Computed with Zephyr's CRC, not with the
 * library under test.
 */
static void raw_fingerprint(struct raw_msg *r)
{
	size_t attr_offset = r->len;

	raw_u32(r, STUN_ATTR_FINGERPRINT, sizeof(uint32_t), 0U);
	sys_put_be32(crc32_ieee(r->buf, attr_offset) ^ RAW_FINGERPRINT_XOR,
		     r->buf + attr_offset + RAW_ATTR_HEADER_SIZE);
}

/* Room for any message a test builds; the text limit test needs more. */
#define TEST_MSG_SIZE      256U
#define TEST_LONG_MSG_SIZE 1024U

/* Transaction id of the messages the tests write: the tests do not look at it.
 * TEST_TXID_FILL is every byte of it, TEST_TXID_FIRST the first of twelve bytes
 * that count up.
 */
#define TEST_TXID_FILL  0x5aU
#define TEST_TXID_FIRST 0x10U

/* A PRIORITY for the tests that do not look at its value. */
#define TEST_PRIORITY 1U

/* 400 Bad Request, and 487 Role Conflict, which an ICE agent sends when both
 * sides claim the same role (RFC 8445 section 7.3.1.1).
 */
#define TEST_ERROR_BAD_REQUEST   400U
#define TEST_ERROR_ROLE_CONFLICT 487U

/* ERROR-CODE values on the wire: two reserved bytes, the class and the number.
 * 400 is class 4, number 0; 487 is class 4, number 87 (0x57).
 */
static const uint8_t error_code_400[] = {0x00U, 0x00U, 0x04U, 0x00U};
static const uint8_t error_code_487[] = {0x00U, 0x00U, 0x04U, 0x57U};

/* The mapped address of the RFC 5769 samples: port 32853, and in section 2.3
 * 2001:db8:1234:5678:11:2233:4455:6677.
 */
#define RFC5769_MAPPED_PORT 32853U
static const uint8_t rfc5769_mapped_ipv6[NET_IPV6_ADDR_SIZE] = {
	0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0x56, 0x78,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
};

/* The default STUN port (RFC 8489 section 8.1). */
#define TEST_STUN_PORT 3478U

/* XOR masks that change one bit of a byte: the lowest, and the highest. */
#define TEST_LOW_BIT 0x01U
#define TEST_TOP_BIT 0x80U

/* One byte longer than the largest key the PSA implementation takes
 * (PSA_MAX_KEY_BITS), so importing it fails.
 */
static const uint8_t oversized_key[PSA_BITS_TO_BYTES(PSA_MAX_KEY_BITS) + 1U] = {0};

/* Attribute types the codec does not define: long-term credentials (RFC 8489
 * section 18.3), TURN (RFC 8656 section 18), NAT behavior discovery (RFC 5780
 * section 7), and a comprehension-required type nobody assigns.
 */
#define TEST_ATTR_PASSWORD_ALGORITHM  0x001dU
#define TEST_ATTR_USERHASH            0x001eU
#define TEST_ATTR_XOR_PEER_ADDRESS    0x0012U
#define TEST_ATTR_XOR_RELAYED_ADDRESS 0x0016U
#define TEST_ATTR_RESPONSE_ORIGIN     0x802bU
#define TEST_ATTR_OTHER_ADDRESS       0x802cU
#define TEST_ATTR_UNASSIGNED          0x7f10U

/* Key and USERNAME of the messages the builder tests sign. */
static const char baseline_password[] = "baselinepassword0123456789";
static const char check_username[] = "CAMufrag:PANufrag";

/* Key and tie-breaker of the connectivity checks. */
static const char check_password[] = "remotepassword0123456789";
static const uint64_t controlling_tiebreaker = 0x0123456789abcdefULL;
static const uint64_t controlled_tiebreaker = 0xfedcba9876543210ULL;
/* PRIORITY of the connectivity checks. */
static const uint32_t check_priority = 0x6e0001ffU;

/* clang-format off */
/* Binding request with SOFTWARE "zephyr-stun-test", USERNAME check_username,
 * PRIORITY 0x7e0000ff, USE-CANDIDATE, XOR-MAPPED-ADDRESS 192.168.88.20 port
 * 50000, MESSAGE-INTEGRITY over baseline_password and FINGERPRINT. The
 * transaction id runs from 0xa0 to 0xab.
 *
 * RFC 5769 has no sample with these attributes that is padded with zeros, as
 * a sender has to pad (RFC 8489 section 14), so this one was written by the
 * author of the codec with a separate encoder following RFC 8489, not recorded
 * from the library. Any HMAC-SHA1 and CRC-32, such as Python's hmac and
 * zlib.crc32, rebuild it from these values.
 */
static const uint8_t baseline_request[] = {
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

/* Connectivity check as the controlling ICE agent sends it: USERNAME
 * check_username, PRIORITY 0x6e0001ff, ICE-CONTROLLING with the tie-breaker
 * controlling_tiebreaker, USE-CANDIDATE, MESSAGE-INTEGRITY over check_password
 * and FINGERPRINT. The transaction id runs from 0xc0 to 0xcb. Written and
 * re-derived like baseline_request: RFC 5769 has no sample with ICE-CONTROLLING
 * that is padded with zeros.
 */
static const uint8_t controlling_request[] = {
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

/* The same check as the controlled agent sends it: ICE-CONTROLLED with the
 * tie-breaker controlled_tiebreaker in place of ICE-CONTROLLING, and no
 * USE-CANDIDATE, which only the controlling agent sends. Written and
 * re-derived like baseline_request.
 */
static const uint8_t controlled_request[] = {
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

/* A 420 Binding error response, the answer to a request with unknown
 * comprehension-required attributes (RFC 8489 section 6.3.1): ERROR-CODE 420
 * "Unknown Attribute", UNKNOWN-ATTRIBUTES listing 0x7f00, 0x7f01 and 0x7f02,
 * and FINGERPRINT, without MESSAGE-INTEGRITY. The transaction id runs from
 * 0xb0 to 0xbb. Written and rebuilt like baseline_request.
 */
static const uint8_t unknown_attributes_error[] = {
	0x01, 0x11, 0x00, 0x30, 0x21, 0x12, 0xa4, 0x42,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0x00, 0x09, 0x00, 0x15,
	0x00, 0x00, 0x04, 0x14, 0x55, 0x6e, 0x6b, 0x6e,
	0x6f, 0x77, 0x6e, 0x20, 0x41, 0x74, 0x74, 0x72,
	0x69, 0x62, 0x75, 0x74, 0x65, 0x00, 0x00, 0x00,
	0x00, 0x0a, 0x00, 0x06, 0x7f, 0x00, 0x7f, 0x01,
	0x7f, 0x02, 0x00, 0x00, 0x80, 0x28, 0x00, 0x04,
	0x5c, 0x80, 0xfd, 0xdc,
};

/* A connectivity check, USERNAME check_username and PRIORITY check_priority,
 * signed with check_password: with MESSAGE-INTEGRITY-SHA256 truncated to 16
 * bytes, the shortest a STUN Usage may allow (RFC 8489 section 14.6), and no
 * FINGERPRINT; and with MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 of 32 bytes
 * and FINGERPRINT, as RFC 8489 section 9.1.2 asks of a request when the agents
 * do not know which algorithm they share. The transaction id runs from 0xc0 to
 * 0xcb. RFC 5769 has no sample with MESSAGE-INTEGRITY-SHA256, and the one of
 * RFC 8489 Appendix B.1 (rfc8489_request) carries only that attribute, under
 * long-term credentials; so these two were written and rebuilt like
 * baseline_request.
 */
static const uint8_t sha256_truncated_request[] = {
	0x00, 0x01, 0x00, 0x34, 0x21, 0x12, 0xa4, 0x42,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0x00, 0x06, 0x00, 0x11,
	0x43, 0x41, 0x4d, 0x75, 0x66, 0x72, 0x61, 0x67,
	0x3a, 0x50, 0x41, 0x4e, 0x75, 0x66, 0x72, 0x61,
	0x67, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x04,
	0x6e, 0x00, 0x01, 0xff, 0x00, 0x1c, 0x00, 0x10,
	0xdd, 0xf3, 0x6b, 0xfe, 0xb8, 0xc6, 0x19, 0x3f,
	0x8b, 0x06, 0xa3, 0x1b, 0xfc, 0x0c, 0x57, 0xf1,
};

static const uint8_t sha256_both_request[] = {
	0x00, 0x01, 0x00, 0x64, 0x21, 0x12, 0xa4, 0x42,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0x00, 0x06, 0x00, 0x11,
	0x43, 0x41, 0x4d, 0x75, 0x66, 0x72, 0x61, 0x67,
	0x3a, 0x50, 0x41, 0x4e, 0x75, 0x66, 0x72, 0x61,
	0x67, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x04,
	0x6e, 0x00, 0x01, 0xff, 0x00, 0x08, 0x00, 0x14,
	0xd3, 0x8a, 0xcd, 0x39, 0xca, 0x37, 0xc7, 0x2e,
	0xe9, 0x6d, 0x73, 0x0f, 0xd1, 0xba, 0xcd, 0x90,
	0x91, 0xe5, 0x62, 0x67, 0x00, 0x1c, 0x00, 0x20,
	0xb9, 0xd9, 0x1c, 0x31, 0xc9, 0x9f, 0xc2, 0x7d,
	0x75, 0xcd, 0xc1, 0xe2, 0x84, 0xfb, 0xce, 0x69,
	0x61, 0xc0, 0x91, 0x0f, 0xd0, 0x6b, 0xca, 0xb2,
	0x23, 0xcb, 0x62, 0xca, 0xe1, 0x63, 0x40, 0x22,
	0x80, 0x28, 0x00, 0x04, 0x66, 0x0f, 0xbb, 0x55,
};
/* clang-format on */

ZTEST_SUITE(stun, NULL, NULL, NULL, NULL, NULL);

/*------------------------------------ TEST RFC 5769 SAMPLES ------------------------------------ */

/* The sample request of RFC 5769 section 2.1 (rfc5769_request) parses, and
 * every attribute the codec understands is found with its value.
 */
ZTEST(stun, test_parse_request_vector)
{
	/* What RFC 5769 section 2.1 lists in the request: the transaction id,
	 * the header Length (88 bytes of attributes), USERNAME, the ICE priority
	 * and the tie-breaker of ICE-CONTROLLED.
	 */
	static const uint8_t txid[STUN_TXID_SIZE] = {
		0xb7, 0xe7, 0xa7, 0x01, 0xbc, 0x34, 0xd6, 0x86, 0xfa, 0x87, 0xdf, 0xae,
	};
	static const uint16_t length = 0x58U;
	static const char username[] = "evtj:h6vY";
	static const uint32_t priority = 0x6e0001ffU;
	static const uint64_t tiebreaker = 0x932ff9b151263b36ULL;
	struct stun_msg m = {0};

	zassert_ok(stun_parse(rfc5769_request, sizeof(rfc5769_request), &m));
	zassert_equal(m.type, STUN_BINDING_REQUEST);
	zassert_equal(m.length, length);
	zassert_mem_equal(m.txid, txid, STUN_TXID_SIZE);
	zassert_equal(m.username_len, strlen(username));
	zassert_mem_equal(m.username, username, strlen(username));
	zassert_true(m.has_priority);
	zassert_equal(m.priority, priority);
	/* The sender is the controlled agent. */
	zassert_true(m.has_ice_controlled);
	zassert_false(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, tiebreaker);
	zassert_false(m.has_use_candidate);
	zassert_false(m.has_error_code);
	zassert_true(m.has_integrity);
	zassert_false(m.has_integrity_sha256);
	zassert_true(m.has_fingerprint);
}

/* The sample request of RFC 5769 section 2.1 (rfc5769_request) passes both
 * checks: FINGERPRINT, and MESSAGE-INTEGRITY with the password of the RFC.
 * Another key does not.
 */
ZTEST(stun, test_verify_request_integrity_and_fingerprint)
{
	static const char wrong_password[] = "VOkJxbRl1RmTxUk/WvJxBu";
	struct stun_msg m = {0};

	zassert_ok(stun_parse(rfc5769_request, sizeof(rfc5769_request), &m));
	zassert_ok(stun_verify_fingerprint(rfc5769_request, sizeof(rfc5769_request), &m));
	zassert_ok(stun_verify_integrity(rfc5769_request, sizeof(rfc5769_request), &m,
					 (const uint8_t *)rfc5769_password,
					 strlen(rfc5769_password)));
	zassert_equal(stun_verify_integrity(rfc5769_request, sizeof(rfc5769_request), &m,
					    (const uint8_t *)wrong_password,
					    strlen(wrong_password)), -EBADMSG);
}

/* The sample IPv4 response of RFC 5769 section 2.2 (rfc5769_response) passes
 * both checks, and its XOR-MAPPED-ADDRESS decodes to 192.0.2.1 port 32853.
 */
ZTEST(stun, test_verify_response_and_xor_mapped_address)
{
	struct net_sockaddr addr = {0};
	struct stun_msg m = {0};

	zassert_ok(stun_parse(rfc5769_response, sizeof(rfc5769_response), &m));
	zassert_equal(m.type, STUN_BINDING_SUCCESS_RESPONSE);
	zassert_ok(stun_verify_fingerprint(rfc5769_response, sizeof(rfc5769_response), &m));
	zassert_ok(stun_verify_integrity(rfc5769_response, sizeof(rfc5769_response), &m,
					 (const uint8_t *)rfc5769_password,
					 strlen(rfc5769_password)));

	zassert_ok(stun_get_xor_mapped_address(rfc5769_response, sizeof(rfc5769_response), &m,
					       &addr));
	zassert_true(addr_is_ipv4(&addr, 192, 0, 2, 1, RFC5769_MAPPED_PORT),
		     "expected 192.0.2.1 port 32853");
}

/*----------------------------------------- TEST PARSE ------------------------------------------ */

/* Damaged copies of the sample request of RFC 5769 section 2.1
 * (rfc5769_request) are caught: a wrong magic cookie and a datagram shorter
 * than the header are not STUN, a changed MESSAGE-INTEGRITY value fails the
 * HMAC, and a changed SOFTWARE byte fails both MESSAGE-INTEGRITY, which signs
 * it, and FINGERPRINT.
 */
ZTEST(stun, test_damaged_request_vector_is_caught)
{
	/* Offsets in rfc5769_request: the magic cookie follows the type and the
	 * Length; SOFTWARE is the first attribute, its value follows its 4-byte
	 * header; any attribute value follows a 4-byte header.
	 */
	static const size_t cookie_offset = 4U;
	static const size_t attr_header_size = 4U;
	static const size_t software_value_offset = STUN_HEADER_SIZE + 4U;
	/* XOR mask: every bit of a byte. */
	static const uint8_t all_bits = 0xffU;
	uint8_t tampered[sizeof(rfc5769_request)] = {0};
	struct stun_msg m = {0};

	/* A wrong magic cookie: not STUN. */
	memcpy(tampered, rfc5769_request, sizeof(rfc5769_request));
	tampered[cookie_offset] ^= all_bits;
	zassert_equal(stun_parse(tampered, sizeof(tampered), &m), -EINVAL);

	/* The offsets of the attributes, from the intact request. */
	zassert_ok(stun_parse(rfc5769_request, sizeof(rfc5769_request), &m));

	/* A changed bit in the MESSAGE-INTEGRITY value: the HMAC does not match. */
	memcpy(tampered, rfc5769_request, sizeof(rfc5769_request));
	tampered[m.integrity_off + attr_header_size] ^= TEST_LOW_BIT;
	zassert_equal(stun_verify_integrity(tampered, sizeof(tampered), &m,
		(const uint8_t *)rfc5769_password, strlen(rfc5769_password)), -EBADMSG);

	/* A changed bit in SOFTWARE, ahead of MESSAGE-INTEGRITY: neither the HMAC
	 * nor the CRC of FINGERPRINT matches.
	 */
	memcpy(tampered, rfc5769_request, sizeof(rfc5769_request));
	tampered[software_value_offset] ^= TEST_LOW_BIT;
	zassert_equal(stun_verify_integrity(tampered, sizeof(tampered), &m,
		(const uint8_t *)rfc5769_password, strlen(rfc5769_password)), -EBADMSG);
	zassert_equal(stun_verify_fingerprint(tampered, sizeof(tampered), &m), -EBADMSG);

	/* One byte short of a header: not STUN. */
	zassert_equal(stun_parse(rfc5769_request, STUN_HEADER_SIZE - 1U, &m), -EINVAL);
}

/* A Binding request with no attributes is valid. The parser reports every
 * attribute as absent, and the functions that look for one return -ENOENT,
 * not a failed check.
 */
ZTEST(stun, test_absent_attributes)
{
	struct raw_msg r = {0};
	struct net_sockaddr addr = {0};
	struct stun_msg m = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;

	/* The message under test is a valid 20-byte STUN header and nothing else:
	 * raw_init() writes the header, and no attribute is added after it.
	 */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);

	/* Parse: valid, with nothing found. */
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(m.length, 0U);
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

	/* Look for attributes: absent, not wrong. */
	zassert_equal(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_SOFTWARE, &value, &value_len),
		      -ENOENT);
	zassert_equal(stun_verify_integrity(r.buf, r.len, &m, (const uint8_t *)rfc5769_password,
					    strlen(rfc5769_password)), -ENOENT);
	zassert_equal(stun_verify_integrity_sha256(r.buf, r.len, &m,
						   (const uint8_t *)rfc5769_password,
						   strlen(rfc5769_password),
						   STUN_INTEGRITY_SHA256_MAX_LEN), -ENOENT);
	zassert_equal(stun_verify_fingerprint(r.buf, r.len, &m), -ENOENT);
	zassert_equal(stun_get_xor_mapped_address(r.buf, r.len, &m, &addr), -ENOENT);
}

/* Datagrams written byte by byte, each with one defect the parser has to
 * refuse with the right code: an attribute that runs past the end, a header
 * Length that does not match the datagram (-EBADMSG), a set top bit in the
 * type and NULL arguments (-EINVAL). Attributes of the wrong length are in
 * test_known_attribute_of_wrong_length_is_malformed.
 */
ZTEST(stun, test_parse_refusal_codes)
{
	/* One of the two top bits of the message type, which are always 0. */
	static const uint16_t type_top_bit = BIT(14);
	/* Half of a 32-bit word. */
	static const uint16_t half_word = RAW_WORD_SIZE / 2U;
	struct raw_msg r = {0};
	struct stun_msg m = {0};
	size_t attrs_len = 0U;

	/* An attribute that runs past the end: the header Length matches the
	 * datagram, but PRIORITY claims 8 bytes of value where 4 follow.
	 */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(uint32_t), TEST_PRIORITY);
	sys_put_be16(sizeof(uint32_t) + RAW_WORD_SIZE,
		     r.buf + STUN_HEADER_SIZE + RAW_ATTR_LENGTH_OFFSET);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* The header Length against the datagram, which holds one PRIORITY. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(uint32_t), TEST_PRIORITY);
	attrs_len = r.len - STUN_HEADER_SIZE;

	/* Shorter than the attributes by one 32-bit word. */
	sys_put_be16((uint16_t)(attrs_len - RAW_WORD_SIZE), r.buf + RAW_LENGTH_OFFSET);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Not a multiple of 4. */
	sys_put_be16((uint16_t)(attrs_len - half_word), r.buf + RAW_LENGTH_OFFSET);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Longer than the datagram by one 32-bit word. */
	sys_put_be16((uint16_t)(attrs_len + RAW_WORD_SIZE), r.buf + RAW_LENGTH_OFFSET);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* A set top bit in the type: not STUN. */
	raw_init(&r, STUN_BINDING_REQUEST | type_top_bit, TEST_TXID_FILL);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EINVAL);

	/* NULL arguments. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	zassert_equal(stun_parse(NULL, r.len, &m), -EINVAL);
	zassert_equal(stun_parse(r.buf, r.len, NULL), -EINVAL);
}

/* stun_has_valid_header() and stun_parse() have to agree on every input:
 * nothing parses that has no valid header, and every message that parses has
 * one. want is what stun_has_valid_header() must answer.
 */
static void expect_valid_header(const uint8_t *buf, size_t len, bool want, const char *what)
{
	struct stun_msg m = {0};
	bool valid = stun_has_valid_header(buf, len);
	int rc = stun_parse(buf, len, &m);

	zassert_equal(valid, want, "stun_has_valid_header: %s", what);
	if (rc == 0) {
		zassert_true(valid, "parsed without a valid header: %s", what);
	}
	if (!valid) {
		zassert_not_equal(rc, 0, "no valid header, but parsed: %s", what);
	}
}

/* What a STUN message is (RFC 8489 section 6.3): the three samples of RFC 5769
 * and a bare header are; a datagram with a set top bit in the type, a wrong
 * magic cookie, too short, with a Length that is not a multiple of 4 or does
 * not match the datagram is not; and neither are the other protocols that
 * share a port with STUN.
 */
ZTEST(stun, test_has_valid_header)
{
	/* The two top bits of the type, alone and together; both are always 0. */
	static const uint8_t type_top_bits[] = {0x40U, 0x80U, 0xc0U};
	/* The last byte of the magic cookie. */
	static const size_t cookie_last_offset = RAW_COOKIE_OFFSET + 3U;
	/* Starts of the protocols that share a port with STUN, padded with zeros
	 * to a STUN header and a word: a DTLS 1.2 handshake record (content type
	 * 22, version 0xfefd), an RTP packet (version 2, payload type 96) and TURN
	 * ChannelData (channel 0x4000, 20 bytes of data).
	 */
	static const uint8_t dtls[24] = {22U, 0xfeU, 0xfdU, 0x00U};
	static const uint8_t rtp[24] = {0x80U, 96U, 0x00U, 0x01U};
	static const uint8_t channel_data[24] = {0x40U, 0x00U, 0x00U, 0x14U};
	/* The sample request with room for one word after it. */
	uint8_t buf[sizeof(rfc5769_request) + 4U] = {0};
	struct raw_msg r = {0};

	/* STUN messages. */
	expect_valid_header(rfc5769_request, sizeof(rfc5769_request), true, "RFC 5769 request");
	expect_valid_header(rfc5769_response, sizeof(rfc5769_response), true,
		       "RFC 5769 IPv4 response");
	expect_valid_header(rfc5769_response_ipv6, sizeof(rfc5769_response_ipv6), true,
		       "RFC 5769 IPv6 response");
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	expect_valid_header(r.buf, r.len, true, "header without attributes");

	/* A set top bit in the type. */
	for (size_t i = 0; i < ARRAY_SIZE(type_top_bits); i++) {
		memcpy(buf, rfc5769_request, sizeof(rfc5769_request));
		buf[0] |= type_top_bits[i];
		expect_valid_header(buf, sizeof(rfc5769_request), false,
				    "top bits of the type set");
	}

	/* One changed bit in the magic cookie. */
	memcpy(buf, rfc5769_request, sizeof(rfc5769_request));
	buf[cookie_last_offset] ^= TEST_LOW_BIT;
	expect_valid_header(buf, sizeof(rfc5769_request), false, "one bit of the magic cookie");

	/* Too short, empty, or no datagram at all. */
	expect_valid_header(rfc5769_request, STUN_HEADER_SIZE - 1U, false, "shorter than a header");
	expect_valid_header(rfc5769_request, 0U, false, "empty");
	expect_valid_header(NULL, sizeof(rfc5769_request), false, "NULL");

	/* A Length of 1 to 3 bytes, which is not a multiple of 4, with as many
	 * bytes after the header.
	 */
	for (uint16_t length = 1U; length < RAW_WORD_SIZE; length++) {
		raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
		sys_put_be16(length, r.buf + RAW_LENGTH_OFFSET);
		expect_valid_header(r.buf, STUN_HEADER_SIZE + length, false,
			       "Length not a multiple of 4");
	}

	/* A Length that does not match the datagram: a word more or a word less. */
	memcpy(buf, rfc5769_request, sizeof(rfc5769_request));
	memset(buf + sizeof(rfc5769_request), 0, RAW_WORD_SIZE);
	expect_valid_header(buf, sizeof(rfc5769_request) + RAW_WORD_SIZE, false,
			    "bytes after the message");
	expect_valid_header(rfc5769_request, sizeof(rfc5769_request) - RAW_WORD_SIZE, false,
			    "shorter than its Length");

	/* Other protocols on the same port. */
	expect_valid_header(dtls, sizeof(dtls), false, "DTLS record");
	expect_valid_header(rtp, sizeof(rtp), false, "RTP packet");
	expect_valid_header(channel_data, sizeof(channel_data), false, "TURN ChannelData");
}

/* A datagram of a header and the one attribute given; attr_len goes in the
 * attribute's Length field. The parser has to refuse it as malformed.
 */
static void expect_malformed(uint16_t attr_type, uint16_t attr_len, const void *value,
			     size_t value_len, const char *what)
{
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, attr_type, attr_len, value, value_len);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG, "%s", what);
}

/* The same, for an attribute the parser has to accept. */
static void expect_well_formed(uint16_t attr_type, uint16_t attr_len, const void *value,
			       size_t value_len, const char *what)
{
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, attr_type, attr_len, value, value_len);
	zassert_ok(stun_parse(r.buf, r.len, &m), "%s", what);
}

/* An attribute the codec understands has to have the length RFC 8489 and
 * RFC 8445 fix for it; with any other the whole message is malformed. An
 * address of a family other than IPv4 and IPv6 is not malformed but ignored
 * (RFC 8489 section 6.3.3), and an address too short to hold a family is.
 */
ZTEST(stun, test_known_attribute_of_wrong_length_is_malformed)
{
	/* Lengths of the values: PRIORITY, a role attribute, FINGERPRINT, one
	 * attribute type in UNKNOWN-ATTRIBUTES, and an IPv4 and an IPv6 address
	 * (the reserved byte, the family, the port and the address).
	 */
	static const uint16_t priority_len = sizeof(uint32_t);
	static const uint16_t role_len = sizeof(uint64_t);
	static const uint16_t fingerprint_len = sizeof(uint32_t);
	static const uint16_t attr_type_len = sizeof(uint16_t);
	static const uint16_t ipv4_len = 4U + 4U;
	static const uint16_t ipv6_len = 4U + 16U;
	/* The bytes before the address: the reserved byte, the family and the
	 * port. An address value shorter than that is malformed whatever family
	 * it names.
	 */
	static const uint16_t before_address_len = 4U;
	/* Zero bytes, enough for any value here. */
	static const uint8_t zeros[40] = {0};
	/* Address values with the family in their second byte: IPv4 (1), IPv6 (2),
	 * and 3, which no RFC assigns.
	 */
	static const uint8_t family_ipv4[20] = {0x00U, 0x01U};
	static const uint8_t family_ipv6[20] = {0x00U, 0x02U};
	static const uint8_t family_3[8] = {0x00U, 0x03U};
	/* The XOR-MAPPED-ADDRESS of RFC 5769 section 2.2, 192.0.2.1 port 32853,
	 * and the same address as MAPPED-ADDRESS, without the XOR.
	 */
	static const uint8_t xor_address[8] = {0x00U, 0x01U, 0xa1U, 0x47U,
					       0xe1U, 0x12U, 0xa6U, 0x43U};
	static const uint8_t mapped_address[8] = {0x00U, 0x01U, 0x80U, 0x55U,
						  0xc0U, 0x00U, 0x02U, 0x01U};

	/* Fixed lengths, one byte or one word off. */
	expect_malformed(STUN_ATTR_PRIORITY, priority_len - 1U, zeros, priority_len - 1U,
			 "PRIORITY one byte short");
	expect_malformed(STUN_ATTR_PRIORITY, 2U * priority_len, zeros, 2U * priority_len,
			 "PRIORITY of two words");
	expect_malformed(STUN_ATTR_USE_CANDIDATE, RAW_WORD_SIZE, zeros, RAW_WORD_SIZE,
			 "USE-CANDIDATE with a value");
	expect_malformed(STUN_ATTR_ICE_CONTROLLING, role_len - RAW_WORD_SIZE, zeros,
			 role_len - RAW_WORD_SIZE, "ICE-CONTROLLING one word short");
	expect_malformed(STUN_ATTR_ICE_CONTROLLING, role_len + RAW_WORD_SIZE, zeros,
			 role_len + RAW_WORD_SIZE, "ICE-CONTROLLING one word long");
	expect_malformed(STUN_ATTR_ICE_CONTROLLED, role_len - RAW_WORD_SIZE, zeros,
			 role_len - RAW_WORD_SIZE, "ICE-CONTROLLED one word short");
	expect_malformed(STUN_ATTR_MESSAGE_INTEGRITY, STUN_INTEGRITY_LEN - 1U, zeros,
			 STUN_INTEGRITY_LEN - 1U, "MESSAGE-INTEGRITY one byte short");
	expect_malformed(STUN_ATTR_FINGERPRINT, 2U * fingerprint_len, zeros,
			 2U * fingerprint_len, "FINGERPRINT of two words");
	expect_malformed(STUN_ATTR_UNKNOWN_ATTRIBUTES, attr_type_len + 1U, zeros,
			 attr_type_len + 1U, "UNKNOWN-ATTRIBUTES of an odd length");

	/* Addresses: a length that does not fit the family, or no family. */
	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, ipv6_len, family_ipv4, ipv6_len,
			 "IPv4 in the length of IPv6");
	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, ipv4_len, family_ipv6, ipv4_len,
			 "IPv6 in the length of IPv4");
	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, ipv4_len - 1U, xor_address, ipv4_len - 1U,
			 "IPv4 one byte short");
	expect_malformed(STUN_ATTR_MAPPED_ADDRESS, ipv4_len - 3U, mapped_address, ipv4_len - 3U,
			 "MAPPED-ADDRESS three bytes short");
	expect_malformed(STUN_ATTR_XOR_MAPPED_ADDRESS, before_address_len - 1U, family_3,
			 before_address_len - 1U, "shorter than the bytes before the address");

	/* The right lengths, and a family nobody assigns, are accepted. */
	expect_well_formed(STUN_ATTR_XOR_MAPPED_ADDRESS, ipv4_len, xor_address, ipv4_len,
			   "IPv4 XOR-MAPPED-ADDRESS");
	expect_well_formed(STUN_ATTR_MAPPED_ADDRESS, ipv4_len, mapped_address, ipv4_len,
			   "IPv4 MAPPED-ADDRESS");
	expect_well_formed(STUN_ATTR_XOR_MAPPED_ADDRESS, ipv4_len, family_3, ipv4_len,
			   "XOR-MAPPED-ADDRESS of family 3");
	expect_well_formed(STUN_ATTR_MAPPED_ADDRESS, ipv4_len, family_3, ipv4_len,
			   "MAPPED-ADDRESS of family 3");
	expect_well_formed(STUN_ATTR_UNKNOWN_ATTRIBUTES, attr_type_len, zeros, attr_type_len,
			   "UNKNOWN-ATTRIBUTES with one type");
	expect_well_formed(STUN_ATTR_UNKNOWN_ATTRIBUTES, 0U, NULL, 0U,
			   "UNKNOWN-ATTRIBUTES with no type");
}

/* MESSAGE-INTEGRITY-SHA256 may be truncated, but only to 16 to 32 bytes in
 * steps of 4 (RFC 8489 section 14.6): every such length is accepted, and an
 * empty value, one word short of the shortest, one not a multiple of 4 and one
 * word over the longest are malformed. They are malformed after
 * MESSAGE-INTEGRITY too, where the parser reads nothing else but FINGERPRINT.
 */
ZTEST(stun, test_integrity_sha256_length)
{
	/* Lengths that are not allowed: none at all, one word short of the
	 * shortest, two bytes past it, and one word past the longest.
	 */
	const uint16_t bad_lens[] = {
		0U,
		STUN_INTEGRITY_SHA256_MIN_LEN - RAW_WORD_SIZE,
		STUN_INTEGRITY_SHA256_MIN_LEN + 2U,
		STUN_INTEGRITY_SHA256_MAX_LEN + RAW_WORD_SIZE,
	};
	/* Zero bytes, enough for any value here. */
	static const uint8_t zeros[40] = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	/* Lengths outside the rule. */
	for (size_t i = 0; i < ARRAY_SIZE(bad_lens); i++) {
		expect_malformed(STUN_ATTR_MESSAGE_INTEGRITY_SHA256, bad_lens[i], zeros,
				 bad_lens[i], "MESSAGE-INTEGRITY-SHA256 length");
	}

	/* Every length the rule allows. */
	for (uint16_t len = STUN_INTEGRITY_SHA256_MIN_LEN; len <= STUN_INTEGRITY_SHA256_MAX_LEN;
	     len += RAW_WORD_SIZE) {
		expect_well_formed(STUN_ATTR_MESSAGE_INTEGRITY_SHA256, len, zeros, len,
				   "MESSAGE-INTEGRITY-SHA256 length");
	}

	/* The lengths outside the rule, after MESSAGE-INTEGRITY. */
	for (size_t i = 0; i < ARRAY_SIZE(bad_lens); i++) {
		raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
		raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY, STUN_INTEGRITY_LEN, zeros,
			 STUN_INTEGRITY_LEN);
		raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, bad_lens[i], zeros, bad_lens[i]);
		zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG,
			      "%u bytes after MESSAGE-INTEGRITY", bad_lens[i]);
	}
}

/* The value of ERROR-CODE (RFC 8489 section 14.8): a class other than 3 to 6,
 * a number over 99, or a value too short to hold a code is malformed. The
 * reserved bits in front of the class are ignored, whatever they hold.
 */
ZTEST(stun, test_error_code_values)
{
	/* ERROR-CODE values: 21 reserved bits, the 3-bit class and the 8-bit
	 * number, in 4 bytes. The class sits in the low 3 bits of the third byte,
	 * the number in the fourth.
	 */
	/* One byte short of a code. */
	static const uint8_t too_short[] = {0x00U, 0x00U, 0x04U};
	/* Class 2 and class 7, just outside 3 to 6. */
	static const uint8_t class_2[] = {0x00U, 0x00U, 0x02U, 0x00U};
	static const uint8_t class_7[] = {0x00U, 0x00U, 0x07U, 0x00U};
	/* Class 4, number 100 (0x64), just past 99. */
	static const uint8_t number_100[] = {0x00U, 0x00U, 0x04U, 0x64U};
	/* 487: class 4, number 87 (0x57), with all 21 reserved bits set: the
	 * first two bytes, and the top 5 bits of the third (0xfc is 11111 100).
	 */
	static const uint8_t reserved_set[] = {0xffU, 0xffU, 0xfcU, 0x57U};
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	/* Malformed values. */
	expect_malformed(STUN_ATTR_ERROR_CODE, sizeof(too_short), too_short, sizeof(too_short),
			 "too short to hold a code");
	expect_malformed(STUN_ATTR_ERROR_CODE, sizeof(class_2), class_2, sizeof(class_2),
			 "class 2");
	expect_malformed(STUN_ATTR_ERROR_CODE, sizeof(class_7), class_7, sizeof(class_7),
			 "class 7");
	expect_malformed(STUN_ATTR_ERROR_CODE, sizeof(number_100), number_100,
			 sizeof(number_100), "number 100");

	/* Set reserved bits: ignored, the code is read as it is. */
	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, STUN_ATTR_ERROR_CODE, sizeof(reserved_set), reserved_set,
		 sizeof(reserved_set));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_true(m.has_error_code);
	zassert_equal(m.error_code, TEST_ERROR_ROLE_CONFLICT);
}

/* Text attributes up to STUN_TEXT_MAX_LEN (763) bytes are accepted: USERNAME,
 * SOFTWARE, REALM, NONCE and the reason phrase of ERROR-CODE. RFC 8489
 * sections 14.3 and 14.8 require a receiver to take that many; a byte more is
 * refused, which is this codec's own limit.
 */
ZTEST(stun, test_text_attribute_limits)
{
	static const uint16_t text_types[] = {
		STUN_ATTR_USERNAME,
		STUN_ATTR_SOFTWARE,
		STUN_ATTR_REALM,
		STUN_ATTR_NONCE,
	};
	/* Any printable character fills the text: the test looks at lengths. */
	static const uint8_t fill = 'a';
	/* A code and the longest text, plus one byte. */
	uint8_t text[4U + STUN_TEXT_MAX_LEN + 1U] = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	memset(text, fill, sizeof(text));

	for (size_t i = 0; i < ARRAY_SIZE(text_types); i++) {
		/* The longest is accepted, and USERNAME is reported whole. */
		raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
		raw_attr(&r, text_types[i], STUN_TEXT_MAX_LEN, text, STUN_TEXT_MAX_LEN);
		zassert_ok(stun_parse(r.buf, r.len, &m), "type 0x%04x of 763 bytes",
			   text_types[i]);
		if (text_types[i] == STUN_ATTR_USERNAME) {
			zassert_equal(m.username_len, STUN_TEXT_MAX_LEN);
		}

		/* One byte more is refused. */
		expect_malformed(text_types[i], STUN_TEXT_MAX_LEN + 1U, text,
				 STUN_TEXT_MAX_LEN + 1U, "text attribute of 764 bytes");
	}

	/* The reason phrase of ERROR-CODE, after the code. */
	memcpy(text, error_code_487, sizeof(error_code_487));
	expect_well_formed(STUN_ATTR_ERROR_CODE, RAW_ERROR_REASON_OFFSET + STUN_TEXT_MAX_LEN,
			   text, RAW_ERROR_REASON_OFFSET + STUN_TEXT_MAX_LEN,
			   "reason phrase of 763 bytes");
	expect_malformed(STUN_ATTR_ERROR_CODE, RAW_ERROR_REASON_OFFSET + STUN_TEXT_MAX_LEN + 1U,
			 text, RAW_ERROR_REASON_OFFSET + STUN_TEXT_MAX_LEN + 1U,
			 "reason phrase of 764 bytes");
}

/* Of each attribute type only the first occurrence counts (RFC 8489 section
 * 14): a second USERNAME, PRIORITY, XOR-MAPPED-ADDRESS, ERROR-CODE or role
 * attribute of the same type changes nothing, and is not even looked at, so a
 * malformed second one does no harm. The same holds for SOFTWARE, REALM,
 * NONCE, USE-CANDIDATE and UNKNOWN-ATTRIBUTES.
 */
ZTEST(stun, test_first_occurrence_wins)
{
	static const char first_username[] = "first";
	static const char second_username[] = "second";
	/* Two priorities, and a second one too short to be a PRIORITY. */
	static const uint32_t first_priority = 1U;
	static const uint32_t second_priority = 2U;
	static const uint16_t short_priority_len = sizeof(uint32_t) - 1U;
	/* XOR-MAPPED-ADDRESS values: 192.0.2.1 port 32853, as in RFC 5769
	 * section 2.2, and 192.0.2.2 with the same port. The last byte differs:
	 * 0x43 and 0x40 are 1 and 2 XORed with the cookie byte 0x42.
	 */
	static const uint8_t first_address[] = {0x00U, 0x01U, 0xa1U, 0x47U,
						0xe1U, 0x12U, 0xa6U, 0x43U};
	static const uint8_t second_address[] = {0x00U, 0x01U, 0xa1U, 0x47U,
						 0xe1U, 0x12U, 0xa6U, 0x40U};
	/* The text attributes other than USERNAME, a first value for each, and
	 * a length one byte past their limit for the second.
	 */
	const uint16_t text_types[] = {
		STUN_ATTR_SOFTWARE,
		STUN_ATTR_REALM,
		STUN_ATTR_NONCE,
	};
	static const char first_text[] = "first";
	static const uint16_t long_text_len = STUN_TEXT_MAX_LEN + 1U;
	/* A USE-CANDIDATE with a 4-byte value, which it never has. */
	static const uint16_t use_candidate_bad_len = sizeof(uint32_t);
	/* An UNKNOWN-ATTRIBUTES list that ends in the middle of a second type. */
	static const uint16_t odd_list_len = sizeof(uint16_t) + 1U;
	/* The one type the first UNKNOWN-ATTRIBUTES lists. Any type works. */
	uint8_t listed_type[sizeof(uint16_t)] = {0};
	struct net_sockaddr addr = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;

	/* Two of each: the first values are reported. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, STUN_ATTR_USERNAME, strlen(first_username), first_username,
		 strlen(first_username));
	raw_attr(&r, STUN_ATTR_USERNAME, strlen(second_username), second_username,
		 strlen(second_username));
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(first_priority), first_priority);
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(second_priority), second_priority);
	raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, sizeof(first_address), first_address,
		 sizeof(first_address));
	raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, sizeof(second_address), second_address,
		 sizeof(second_address));
	raw_attr(&r, STUN_ATTR_ERROR_CODE, sizeof(error_code_400), error_code_400,
		 sizeof(error_code_400));
	raw_attr(&r, STUN_ATTR_ERROR_CODE, sizeof(error_code_487), error_code_487,
		 sizeof(error_code_487));
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, sizeof(uint64_t), controlling_tiebreaker);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, sizeof(uint64_t), controlled_tiebreaker);

	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(m.username_len, strlen(first_username));
	zassert_mem_equal(m.username, first_username, strlen(first_username));
	zassert_equal(m.priority, first_priority);
	zassert_ok(stun_get_xor_mapped_address(r.buf, r.len, &m, &addr));
	zassert_true(addr_is_ipv4(&addr, 192, 0, 2, 1, RFC5769_MAPPED_PORT),
		     "192.0.2.1, not the second 192.0.2.2");
	zassert_equal(m.error_code, TEST_ERROR_BAD_REQUEST);
	zassert_true(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, controlling_tiebreaker);

	/* The same role twice, as the controlled agent. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, sizeof(uint64_t), controlled_tiebreaker);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, sizeof(uint64_t), controlling_tiebreaker);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_true(m.has_ice_controlled);
	zassert_equal(m.ice_tiebreaker, controlled_tiebreaker);

	/* A malformed second PRIORITY is skipped, not refused. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(first_priority), first_priority);
	raw_u32(&r, STUN_ATTR_PRIORITY, short_priority_len, second_priority);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(m.priority, first_priority);

	/* A second SOFTWARE, REALM or NONCE over the text limit is skipped, and
	 * the first is the one found. The long value is the zeros of raw_init().
	 */
	for (size_t i = 0; i < ARRAY_SIZE(text_types); i++) {
		raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
		raw_attr(&r, text_types[i], strlen(first_text), first_text, strlen(first_text));
		raw_attr(&r, text_types[i], long_text_len, NULL, 0U);
		zassert_ok(stun_parse(r.buf, r.len, &m), "type 0x%04x", text_types[i]);
		zassert_ok(stun_find_attr(r.buf, r.len, &m, text_types[i], &value, &value_len),
			   "type 0x%04x", text_types[i]);
		zassert_equal(value_len, strlen(first_text), "type 0x%04x", text_types[i]);
	}

	/* A second USE-CANDIDATE that has a value is skipped. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_attr(&r, STUN_ATTR_USE_CANDIDATE, 0U, NULL, 0U);
	raw_attr(&r, STUN_ATTR_USE_CANDIDATE, use_candidate_bad_len, NULL, 0U);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_true(m.has_use_candidate);

	/* A second UNKNOWN-ATTRIBUTES of an odd length is skipped, and the
	 * first is the one found.
	 */
	sys_put_be16(STUN_ATTR_PRIORITY, listed_type);
	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, STUN_ATTR_UNKNOWN_ATTRIBUTES, sizeof(listed_type), listed_type,
		 sizeof(listed_type));
	raw_attr(&r, STUN_ATTR_UNKNOWN_ATTRIBUTES, odd_list_len, listed_type, sizeof(listed_type));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_ok(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_UNKNOWN_ATTRIBUTES, &value,
				  &value_len));
	zassert_equal(value_len, sizeof(listed_type));
}

/* ICE-CONTROLLING and ICE-CONTROLLED in one message are malformed, in either
 * order: an agent sends the attribute of its one role (RFC 8445 section
 * 7.1.3), never both.
 */
ZTEST(stun, test_both_ice_roles_are_malformed)
{
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	/* Controlling first. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, sizeof(uint64_t), controlling_tiebreaker);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, sizeof(uint64_t), controlled_tiebreaker);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Controlled first. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLED, sizeof(uint64_t), controlled_tiebreaker);
	raw_u64(&r, STUN_ATTR_ICE_CONTROLLING, sizeof(uint64_t), controlling_tiebreaker);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);
}

/* FINGERPRINT is the last attribute (RFC 8489 section 14.7): a message that
 * ends with it parses and passes the check, and any attribute after it, a
 * second FINGERPRINT included, makes the message malformed.
 */
ZTEST(stun, test_fingerprint_must_be_last)
{
	static const char late_software[] = "late";
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	/* PRIORITY, then FINGERPRINT: valid. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(uint32_t), TEST_PRIORITY);
	raw_fingerprint(&r);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_ok(stun_verify_fingerprint(r.buf, r.len, &m));

	/* The same with SOFTWARE after FINGERPRINT. */
	raw_attr(&r, STUN_ATTR_SOFTWARE, strlen(late_software), late_software,
		 strlen(late_software));
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG);

	/* Two FINGERPRINTs. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_fingerprint(&r);
	raw_fingerprint(&r);
	zassert_equal(stun_parse(r.buf, r.len, &m), -EBADMSG, "two FINGERPRINTs");
}

/* A connectivity check signed by the library's own builder with
 * check_password: USERNAME, PRIORITY, ICE-CONTROLLED, MESSAGE-INTEGRITY and
 * FINGERPRINT. Returns its length.
 */
static size_t signed_check(uint8_t *out, size_t buf_size)
{
	struct stun_builder b = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);
	stun_builder_init(&b, out, buf_size, STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, check_username));
	zassert_ok(stun_add_priority(&b, check_priority));
	zassert_ok(stun_add_ice_controlled(&b, controlled_tiebreaker));
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	return (size_t)len;
}

/* The same for a success response that carries no address: SOFTWARE,
 * MESSAGE-INTEGRITY and FINGERPRINT.
 */
static size_t signed_response(uint8_t *out, size_t buf_size)
{
	struct stun_builder b = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);
	stun_builder_init(&b, out, buf_size, STUN_BINDING_SUCCESS_RESPONSE, txid);
	zassert_ok(stun_add_software(&b, "zephyr"));
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	return (size_t)len;
}

/* What anyone on the path can do without the key: insert an attribute between
 * MESSAGE-INTEGRITY and FINGERPRINT and compute FINGERPRINT again, which has no
 * key. msg ends with the FINGERPRINT the builder wrote. Returns the new length.
 * An attribute appended after FINGERPRINT instead would be refused by another
 * rule and prove nothing about the integrity boundary.
 */
static size_t splice_after_integrity(uint8_t *msg, size_t len, size_t buf_size,
				     uint16_t attr_type, uint16_t attr_len, const void *value,
				     size_t value_len)
{
	size_t padded = ROUND_UP(attr_len, RAW_WORD_SIZE);
	/* The new attribute takes the place of the old FINGERPRINT. */
	size_t attr_offset = len - RAW_FINGERPRINT_SIZE;
	size_t fingerprint_offset = attr_offset + RAW_ATTR_HEADER_SIZE + padded;

	zassert_true(fingerprint_offset + RAW_FINGERPRINT_SIZE <= buf_size, "splice overflow");
	sys_put_be16(attr_type, msg + attr_offset);
	sys_put_be16(attr_len, msg + attr_offset + RAW_ATTR_LENGTH_OFFSET);
	memset(msg + attr_offset + RAW_ATTR_HEADER_SIZE, 0, padded);
	if (value_len != 0U) {
		memcpy(msg + attr_offset + RAW_ATTR_HEADER_SIZE, value, value_len);
	}

	/* The header Length counts the new attribute and FINGERPRINT. */
	sys_put_be16((uint16_t)(fingerprint_offset + RAW_FINGERPRINT_SIZE - STUN_HEADER_SIZE),
		     msg + RAW_LENGTH_OFFSET);
	sys_put_be16(STUN_ATTR_FINGERPRINT, msg + fingerprint_offset);
	sys_put_be16(sizeof(uint32_t), msg + fingerprint_offset + RAW_ATTR_LENGTH_OFFSET);
	sys_put_be32(crc32_ieee(msg, fingerprint_offset) ^ RAW_FINGERPRINT_XOR,
		     msg + fingerprint_offset + RAW_ATTR_HEADER_SIZE);
	return fingerprint_offset + RAW_FINGERPRINT_SIZE;
}

/* The forgery is only a forgery if both checks still pass on it. */
static void expect_still_authentic(const uint8_t *msg, size_t len, const struct stun_msg *m)
{
	zassert_ok(stun_verify_integrity(msg, len, m, (const uint8_t *)check_password,
					 strlen(check_password)),
		   "the forged message must still pass MESSAGE-INTEGRITY");
	zassert_ok(stun_verify_fingerprint(msg, len, m),
		   "the forged message must still pass FINGERPRINT");
}

/* Attributes after MESSAGE-INTEGRITY are outside its HMAC, so anyone on the
 * path can add them; the parser ignores them (RFC 8489 section 9). An
 * attribute inserted after MESSAGE-INTEGRITY, with FINGERPRINT computed again,
 * leaves a message that still passes both checks, and none of what it says is
 * reported: not USE-CANDIDATE, not a different PRIORITY, USERNAME or ICE role,
 * not an address. A malformed attribute there is not even looked at.
 */
ZTEST(stun, test_attributes_after_integrity_are_ignored)
{
	/* What an attacker would insert: a PRIORITY, a USERNAME, a tie-breaker
	 * for the other role, a PRIORITY one byte short, and the
	 * XOR-MAPPED-ADDRESS of RFC 5769 section 2.2 (192.0.2.1 port 32853).
	 */
	static const uint8_t forged_priority[] = {0x01U, 0x02U, 0x03U, 0x04U};
	static const char forged_username[] = "evil";
	static const uint8_t forged_tiebreaker[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
	static const uint8_t short_priority[] = {0x01U, 0x02U, 0x03U};
	static const uint8_t forged_address[] = {0x00U, 0x01U, 0xa1U, 0x47U,
						 0xe1U, 0x12U, 0xa6U, 0x43U};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	struct net_sockaddr addr = {0};
	struct stun_msg m = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;
	size_t len = 0U;

	/* USE-CANDIDATE: it would nominate a pair the peer never nominated. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_USE_CANDIDATE, 0U, NULL, 0U);
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_false(m.has_use_candidate);
	zassert_equal(stun_find_attr(msg, len, &m, STUN_ATTR_USE_CANDIDATE, &value, &value_len),
		      -ENOENT);

	/* PRIORITY: the signed value stands. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_PRIORITY,
				     sizeof(forged_priority), forged_priority,
				     sizeof(forged_priority));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_true(m.has_priority);
	zassert_equal(m.priority, check_priority);

	/* USERNAME: the signed value stands. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_USERNAME,
				     strlen(forged_username), forged_username,
				     strlen(forged_username));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_equal(m.username_len, strlen(check_username));
	zassert_mem_equal(m.username, check_username, strlen(check_username));

	/* The other ICE role: neither a change of role nor a malformed message. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_ICE_CONTROLLING,
				     sizeof(forged_tiebreaker), forged_tiebreaker,
				     sizeof(forged_tiebreaker));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_true(m.has_ice_controlled);
	zassert_false(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, controlled_tiebreaker);

	/* A malformed attribute there is not looked at. */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_PRIORITY,
				     sizeof(short_priority), short_priority,
				     sizeof(short_priority));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_equal(m.priority, check_priority);

	/* XOR-MAPPED-ADDRESS in a response that has none: neither the getter nor
	 * the generic lookup sees it.
	 */
	len = signed_response(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_XOR_MAPPED_ADDRESS,
				     sizeof(forged_address), forged_address,
				     sizeof(forged_address));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);
	zassert_equal(stun_get_xor_mapped_address(msg, len, &m, &addr), -ENOENT);
	zassert_equal(stun_find_attr(msg, len, &m, STUN_ATTR_XOR_MAPPED_ADDRESS, &value,
				     &value_len), -ENOENT);
}

/* The two exceptions to the integrity boundary (RFC 8489 section 9): a
 * MESSAGE-INTEGRITY-SHA256 after MESSAGE-INTEGRITY, and FINGERPRINT, are found
 * and reported where they are, while the boundary stays at MESSAGE-INTEGRITY.
 */
ZTEST(stun, test_sha256_and_fingerprint_after_integrity_are_found)
{
	/* Any byte fills the inserted MESSAGE-INTEGRITY-SHA256 value: the parser
	 * does not check it, the test only finds it again.
	 */
	static const uint8_t sha256_fill = 0xabU;
	uint8_t sha256[STUN_INTEGRITY_SHA256_MAX_LEN] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	struct stun_msg m = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;
	size_t integrity_offset = 0U;
	size_t len = 0U;

	/* A signed check, with MESSAGE-INTEGRITY-SHA256 inserted after
	 * MESSAGE-INTEGRITY: the message still passes both checks.
	 */
	memset(sha256, sha256_fill, sizeof(sha256));
	len = signed_check(msg, sizeof(msg));
	integrity_offset = len - RAW_FINGERPRINT_SIZE - RAW_INTEGRITY_SIZE;
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_MESSAGE_INTEGRITY_SHA256,
				     sizeof(sha256), sha256, sizeof(sha256));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);

	/* Where the parser records the three attributes and the boundary. */
	zassert_true(m.has_integrity);
	zassert_equal(m.integrity_off, integrity_offset);
	zassert_equal(m.attrs_end, integrity_offset);
	zassert_true(m.has_integrity_sha256);
	zassert_equal(m.integrity_sha256_off, integrity_offset + RAW_INTEGRITY_SIZE);
	zassert_equal(m.integrity_sha256_len, sizeof(sha256));
	zassert_true(m.has_fingerprint);
	zassert_equal(m.fingerprint_off, len - RAW_FINGERPRINT_SIZE);

	/* stun_find_attr() finds all three, with their values. */
	zassert_ok(stun_find_attr(msg, len, &m, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, &value,
				  &value_len));
	zassert_equal(value_len, sizeof(sha256));
	zassert_mem_equal(value, sha256, sizeof(sha256));
	zassert_ok(stun_find_attr(msg, len, &m, STUN_ATTR_MESSAGE_INTEGRITY, &value, &value_len));
	zassert_equal(value_len, STUN_INTEGRITY_LEN);
	zassert_ok(stun_find_attr(msg, len, &m, STUN_ATTR_FINGERPRINT, &value, &value_len));
	zassert_equal(value_len, sizeof(uint32_t));
}

/* Without MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 is the integrity
 * boundary (RFC 8489 section 9): what comes before it is reported, what
 * follows is ignored, except FINGERPRINT.
 */
ZTEST(stun, test_sha256_is_the_boundary_without_integrity)
{
	static const char username[] = "user";
	/* The value is not checked by the parser: zeros do. */
	uint8_t sha256[STUN_INTEGRITY_SHA256_MAX_LEN] = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;
	size_t sha256_offset = 0U;

	/* USERNAME, MESSAGE-INTEGRITY-SHA256, PRIORITY, FINGERPRINT. */
	raw_init(&r, STUN_BINDING_REQUEST, TEST_TXID_FILL);
	raw_attr(&r, STUN_ATTR_USERNAME, strlen(username), username, strlen(username));
	sha256_offset = r.len;
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, sizeof(sha256), sha256, sizeof(sha256));
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(uint32_t), TEST_PRIORITY);
	raw_fingerprint(&r);

	/* The boundary is MESSAGE-INTEGRITY-SHA256. */
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_ok(stun_verify_fingerprint(r.buf, r.len, &m));
	zassert_false(m.has_integrity);
	zassert_true(m.has_integrity_sha256);
	zassert_equal(m.integrity_sha256_off, sha256_offset);
	zassert_equal(m.attrs_end, sha256_offset);

	/* USERNAME before it is reported, PRIORITY after it is not, and
	 * FINGERPRINT is found.
	 */
	zassert_equal(m.username_len, strlen(username));
	zassert_false(m.has_priority, "PRIORITY follows MESSAGE-INTEGRITY-SHA256");
	zassert_equal(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_PRIORITY, &value, &value_len),
		      -ENOENT);
	zassert_true(m.has_fingerprint);
}

/* A second MESSAGE-INTEGRITY, inserted after the first, lies past the
 * integrity boundary and is ignored (RFC 8489 section 9): the parser keeps the
 * first, and the message still passes both checks with the right key.
 */
ZTEST(stun, test_second_integrity_is_ignored)
{
	/* Any byte fills the second MESSAGE-INTEGRITY: it is never checked. */
	static const uint8_t junk_fill = 0x5aU;
	uint8_t junk[STUN_INTEGRITY_LEN] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	struct stun_msg m = {0};
	size_t integrity_offset = 0U;
	size_t len = 0U;

	/* A signed check with a second MESSAGE-INTEGRITY inserted after it. */
	memset(junk, junk_fill, sizeof(junk));
	len = signed_check(msg, sizeof(msg));
	integrity_offset = len - RAW_FINGERPRINT_SIZE - RAW_INTEGRITY_SIZE;
	len = splice_after_integrity(msg, len, sizeof(msg), STUN_ATTR_MESSAGE_INTEGRITY,
				     sizeof(junk), junk, sizeof(junk));

	/* The first one is recorded and checked. */
	zassert_ok(stun_parse(msg, len, &m));
	zassert_equal(m.integrity_off, integrity_offset, "the first MESSAGE-INTEGRITY is the one");
	expect_still_authentic(msg, len, &m);
}

/* stun_find_attr() finds any attribute ahead of the integrity boundary, also
 * one the codec does not know: the first of two, as a pointer into the
 * caller's buffer, and one without a value with length 0. An absent attribute
 * is -ENOENT, a NULL argument or a stun_msg parsed from another buffer length
 * is -EINVAL.
 */
ZTEST(stun, test_find_attr)
{
	/* Attribute types the codec does not know: XOR-RELAYED-ADDRESS and
	 * REQUESTED-ADDRESS-FAMILY of TURN (RFC 8656 section 18), and 0x8050,
	 * comprehension-optional and assigned to nobody.
	 */
	static const uint16_t requested_address_family = 0x0017U;
	static const uint16_t unassigned_optional = 0x8050U;
	/* Two values in the XOR-MAPPED-ADDRESS format: 192.0.2.1 port 32853, as
	 * in RFC 5769 section 2.2, and 192.0.2.2 with the same port.
	 */
	static const uint8_t first_address[] = {0x00U, 0x01U, 0xa1U, 0x47U,
						0xe1U, 0x12U, 0xa6U, 0x43U};
	static const uint8_t second_address[] = {0x00U, 0x01U, 0xa1U, 0x47U,
						 0xe1U, 0x12U, 0xa6U, 0x40U};
	struct raw_msg r = {0};
	struct stun_msg m = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;

	/* Two XOR-RELAYED-ADDRESS, then an attribute without a value. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, TEST_ATTR_XOR_RELAYED_ADDRESS, sizeof(first_address), first_address,
		 sizeof(first_address));
	raw_attr(&r, TEST_ATTR_XOR_RELAYED_ADDRESS, sizeof(second_address), second_address,
		 sizeof(second_address));
	raw_attr(&r, unassigned_optional, 0U, NULL, 0U);
	zassert_ok(stun_parse(r.buf, r.len, &m));

	/* The first of two, as a pointer into the caller's buffer. */
	zassert_ok(stun_find_attr(r.buf, r.len, &m, TEST_ATTR_XOR_RELAYED_ADDRESS, &value,
				  &value_len));
	zassert_equal(value_len, sizeof(first_address));
	zassert_equal_ptr(value, r.buf + STUN_HEADER_SIZE + RAW_ATTR_HEADER_SIZE);
	zassert_mem_equal(value, first_address, sizeof(first_address));

	/* An attribute without a value is found, with length 0. */
	zassert_ok(stun_find_attr(r.buf, r.len, &m, unassigned_optional, &value, &value_len));
	zassert_equal(value_len, 0U);

	/* Absent attributes. */
	zassert_equal(stun_find_attr(r.buf, r.len, &m, requested_address_family, &value,
				     &value_len), -ENOENT);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_FINGERPRINT, &value, &value_len),
		      -ENOENT);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, STUN_ATTR_MESSAGE_INTEGRITY, &value,
				     &value_len), -ENOENT);

	/* NULL arguments, and a datagram length that is not the parsed one. */
	zassert_equal(stun_find_attr(NULL, r.len, &m, TEST_ATTR_XOR_RELAYED_ADDRESS, &value,
				     &value_len), -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len, NULL, TEST_ATTR_XOR_RELAYED_ADDRESS, &value,
				     &value_len), -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, TEST_ATTR_XOR_RELAYED_ADDRESS, NULL,
				     &value_len), -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len, &m, TEST_ATTR_XOR_RELAYED_ADDRESS, &value, NULL),
		      -EINVAL);
	zassert_equal(stun_find_attr(r.buf, r.len - RAW_WORD_SIZE, &m,
				     TEST_ATTR_XOR_RELAYED_ADDRESS, &value, &value_len),
		      -EINVAL, "msg was parsed from another buffer length");
}

/* The 14-bit message type interleaves the 12-bit method with the 2-bit class
 * (RFC 8489 section 5). stun_msg_type_compose() builds the four Binding types
 * the RFC gives, and stun_msg_type_get_method() and stun_msg_type_get_class()
 * take any type apart again into the method and class it was built from, with
 * the top two bits zero.
 */
ZTEST(stun, test_msg_type)
{
	/* The Binding types: request 0x0001 and success response 0x0101 as RFC
	 * 8489 section 5 spells them out, indication 0x0011 and error response
	 * 0x0111 by the same encoding.
	 */
	static const uint16_t binding_indication = 0x0011U;
	/* Methods to take apart: Binding, the TURN methods (RFC 8656 section 17:
	 * Allocate, Refresh, Send, Data, CreatePermission, ChannelBind), 0x080,
	 * whose one set bit lands above both class bits, and 0xfff, the highest
	 * 12-bit method.
	 */
	static const uint16_t methods[] = {
		STUN_METHOD_BINDING, 0x003U, 0x004U, 0x006U, 0x007U, 0x008U, 0x009U, 0x080U, 0xfffU,
	};
	static const enum stun_class classes[] = {
		STUN_CLASS_REQUEST,
		STUN_CLASS_INDICATION,
		STUN_CLASS_SUCCESS,
		STUN_CLASS_ERROR,
	};
	/* The two top bits of the type, which are always 0. */
	static const uint16_t type_top_bits = 0xc000U;
	uint16_t type = 0U;

	/* The Binding types. */
	zassert_equal(stun_msg_type_compose(STUN_METHOD_BINDING, STUN_CLASS_REQUEST),
		      STUN_BINDING_REQUEST);
	zassert_equal(stun_msg_type_compose(STUN_METHOD_BINDING, STUN_CLASS_INDICATION),
		      binding_indication);
	zassert_equal(stun_msg_type_compose(STUN_METHOD_BINDING, STUN_CLASS_SUCCESS),
		      STUN_BINDING_SUCCESS_RESPONSE);
	zassert_equal(stun_msg_type_compose(STUN_METHOD_BINDING, STUN_CLASS_ERROR),
		      STUN_BINDING_ERROR_RESPONSE);

	/* Every method with every class, built and taken apart again. */
	for (size_t i = 0; i < ARRAY_SIZE(methods); i++) {
		for (size_t k = 0; k < ARRAY_SIZE(classes); k++) {
			type = stun_msg_type_compose(methods[i], classes[k]);
			zassert_equal(type & type_top_bits, 0U, "top bits of 0x%04x", type);
			zassert_equal(stun_msg_type_get_method(type), methods[i],
				      "method of 0x%04x", type);
			zassert_equal(stun_msg_type_get_class(type), classes[k],
				      "class of 0x%04x", type);
		}
	}
}

/* stun_unknown_attrs() lists the comprehension-required attributes (below
 * 0x8000) that neither the codec nor the caller understands, which a request
 * answers with 420: each type once, in order of appearance, with the full
 * count however little room the list has. Comprehension-optional attributes,
 * the codec's own ones and the caller's are not listed; the types RFC 8489
 * only reserves are.
 */
ZTEST(stun, test_unknown_attrs)
{
	/* Comprehension-required types nobody assigns (the second half of the
	 * range, 0x4000 to 0x7fff, is given out by Expert Review: RFC 8489
	 * section 18.3).
	 */
	static const uint16_t unknown_required_a = 0x7f00U;
	static const uint16_t unknown_required_b = 0x7f01U;
	static const uint16_t unknown_required_c = 0x7f02U;
	/* Comprehension-optional types: 0x8000, the first of the range, 0xc057
	 * from its Expert Review half, and RESPONSE-ORIGIN and OTHER-ADDRESS that
	 * real servers send (RFC 5780 section 7).
	 */
	static const uint16_t optional_first = 0x8000U;
	static const uint16_t optional_expert = 0xc057U;
	/* Types the caller understands here: LIFETIME and XOR-RELAYED-ADDRESS of
	 * TURN (RFC 8656 section 18), and a LIFETIME of 600 seconds.
	 */
	static const uint16_t lifetime = 0x000dU;
	static const uint16_t known[] = {lifetime, TEST_ATTR_XOR_RELAYED_ADDRESS};
	static const uint32_t lifetime_seconds = 600U;
	/* Types RFC 8489 section 18.3.1 only reserves: RESPONSE-ADDRESS,
	 * SOURCE-ADDRESS and CHANGED-ADDRESS of RFC 3489.
	 */
	static const uint16_t response_address = 0x0002U;
	static const uint16_t source_address = 0x0004U;
	static const uint16_t changed_address = 0x0005U;
	/* Values for the attributes: an address (192.0.2.1 port 32853), four
	 * bytes, and a USERNAME.
	 */
	static const uint8_t address[] = {0x00U, 0x01U, 0x80U, 0x55U, 0xc0U, 0x00U, 0x02U, 0x01U};
	static const uint8_t four_bytes[] = {0x00U, 0x00U, 0x00U, 0x01U};
	static const char username[] = "user";
	uint16_t out[4] = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	/* Nothing to list: a known attribute and comprehension-optional ones. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, STUN_ATTR_USERNAME, strlen(username), username, strlen(username));
	raw_attr(&r, optional_first, 0U, NULL, 0U);
	raw_attr(&r, optional_expert, sizeof(four_bytes), four_bytes, sizeof(four_bytes));
	raw_attr(&r, TEST_ATTR_RESPONSE_ORIGIN, sizeof(address), address, sizeof(address));
	raw_attr(&r, TEST_ATTR_OTHER_ADDRESS, sizeof(address), address, sizeof(address));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, ARRAY_SIZE(out)), 0);

	/* One unknown. */
	raw_attr(&r, unknown_required_a, sizeof(four_bytes), four_bytes, sizeof(four_bytes));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	memset(out, 0, sizeof(out));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, ARRAY_SIZE(out)), 1);
	zassert_equal(out[0], unknown_required_a);

	/* Each type once, in order of appearance. */
	raw_attr(&r, unknown_required_b, 0U, NULL, 0U);
	raw_attr(&r, unknown_required_a, 0U, NULL, 0U);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	memset(out, 0, sizeof(out));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, ARRAY_SIZE(out)), 2);
	zassert_equal(out[0], unknown_required_a);
	zassert_equal(out[1], unknown_required_b);
	zassert_equal(out[2], 0U, "nothing written past the count");

	/* The full count, however little room the list has. */
	raw_attr(&r, unknown_required_c, 0U, NULL, 0U);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	memset(out, 0, sizeof(out));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, 1U), 3);
	zassert_equal(out[0], unknown_required_a);
	zassert_equal(out[1], 0U, "nothing written past the room");
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, NULL, 0U), 3);

	/* What the caller understands is not unknown. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_u32(&r, lifetime, sizeof(lifetime_seconds), lifetime_seconds);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, ARRAY_SIZE(out)), 1);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, known, ARRAY_SIZE(known), out,
					 ARRAY_SIZE(out)), 0);

	/* The types RFC 8489 only reserves are unknown. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, response_address, sizeof(address), address, sizeof(address));
	raw_attr(&r, source_address, sizeof(address), address, sizeof(address));
	raw_attr(&r, changed_address, sizeof(address), address, sizeof(address));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, ARRAY_SIZE(out)), 3);

	/* NULL arguments, and a datagram length that is not the parsed one. */
	zassert_equal(stun_unknown_attrs(NULL, r.len, &m, NULL, 0U, out, 1U), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, NULL, NULL, 0U, out, 1U), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 1U, out, 1U), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, NULL, 1U), -EINVAL);
	zassert_equal(stun_unknown_attrs(r.buf, r.len - RAW_WORD_SIZE, &m, NULL, 0U, out, 1U),
		      -EINVAL, "msg was parsed from another buffer length");
}

/* Every comprehension-required attribute the codec itself understands is left
 * out of the list stun_unknown_attrs() makes. MESSAGE-INTEGRITY and
 * MESSAGE-INTEGRITY-SHA256 close the message: the first of them is the
 * integrity boundary, where stun_unknown_attrs() stops, so they are never
 * looked at and never listed either.
 */
ZTEST(stun, test_unknown_attrs_knows_the_codec_set)
{
	/* Values for the attributes: MAPPED-ADDRESS 192.0.2.1 port 32853, its
	 * XOR-MAPPED-ADDRESS form from RFC 5769 section 2.2, and text.
	 */
	static const uint8_t mapped_address[] = {0x00U, 0x01U, 0x80U, 0x55U,
						 0xc0U, 0x00U, 0x02U, 0x01U};
	static const uint8_t xor_address[] = {0x00U, 0x01U, 0xa1U, 0x47U,
					      0xe1U, 0x12U, 0xa6U, 0x43U};
	static const char username[] = "user";
	static const char realm[] = "realm";
	static const char nonce[] = "nonce";
	/* Zero bytes for the values that are not looked at: one attribute type
	 * in UNKNOWN-ATTRIBUTES, and the two integrity values.
	 */
	static const uint8_t zeros[STUN_INTEGRITY_SHA256_MAX_LEN] = {0};
	uint16_t out[4] = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	/* One of each, the two integrity attributes last. */
	raw_init(&r, STUN_BINDING_ERROR_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, STUN_ATTR_MAPPED_ADDRESS, sizeof(mapped_address), mapped_address,
		 sizeof(mapped_address));
	raw_attr(&r, STUN_ATTR_USERNAME, strlen(username), username, strlen(username));
	raw_attr(&r, STUN_ATTR_ERROR_CODE, sizeof(error_code_487), error_code_487,
		 sizeof(error_code_487));
	raw_attr(&r, STUN_ATTR_UNKNOWN_ATTRIBUTES, sizeof(uint16_t), zeros, sizeof(uint16_t));
	raw_attr(&r, STUN_ATTR_REALM, strlen(realm), realm, strlen(realm));
	raw_attr(&r, STUN_ATTR_NONCE, strlen(nonce), nonce, strlen(nonce));
	raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, sizeof(xor_address), xor_address,
		 sizeof(xor_address));
	raw_u32(&r, STUN_ATTR_PRIORITY, sizeof(uint32_t), TEST_PRIORITY);
	raw_attr(&r, STUN_ATTR_USE_CANDIDATE, 0U, NULL, 0U);
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY, STUN_INTEGRITY_LEN, zeros, STUN_INTEGRITY_LEN);
	raw_attr(&r, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, STUN_INTEGRITY_SHA256_MAX_LEN, zeros,
		 STUN_INTEGRITY_SHA256_MAX_LEN);

	/* Nothing is unknown. */
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, ARRAY_SIZE(out)), 0);
}

/* stun_unknown_attrs() stops at the integrity boundary: an unknown
 * comprehension-required attribute inserted after MESSAGE-INTEGRITY, where
 * anyone on the path can put one without the key, is not listed, and so
 * cannot make a signed request draw a 420.
 */
ZTEST(stun, test_unknown_attrs_stops_at_integrity)
{
	/* A comprehension-required type nobody assigns, with any 4-byte value. */
	static const uint16_t unknown_required = 0x7f00U;
	static const uint8_t four_bytes[] = {0x00U, 0x00U, 0x00U, 0x01U};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	uint16_t out[4] = {0};
	struct stun_msg m = {0};
	size_t len = 0U;

	/* A signed check with the unknown attribute inserted after
	 * MESSAGE-INTEGRITY: it still passes both checks.
	 */
	len = signed_check(msg, sizeof(msg));
	len = splice_after_integrity(msg, len, sizeof(msg), unknown_required, sizeof(four_bytes),
				     four_bytes, sizeof(four_bytes));
	zassert_ok(stun_parse(msg, len, &m));
	expect_still_authentic(msg, len, &m);

	/* Nothing is listed. */
	zassert_equal(stun_unknown_attrs(msg, len, &m, NULL, 0U, out, ARRAY_SIZE(out)), 0,
		      "an attribute nobody signed is not there at all");
}

/* Sets of attributes public STUN servers return to a Binding request, with
 * their address replaced by 192.0.2.1 port 32853: each has a valid header,
 * parses, has no unknown comprehension-required attribute, and gives the
 * mapped address. None carries anything a strict parser refuses.
 */
ZTEST(stun, test_success_response_shapes)
{
	/* Which attributes follow XOR-MAPPED-ADDRESS in each set. */
	struct response_shape {
		bool mapped_address;
		bool response_origin;
		bool other_address;
		bool software;
		bool fingerprint;
	};
	static const struct response_shape shapes[] = {
		{false, false, false, false, false},
		{true, false, false, true, false},
		{true, true, true, false, false},
		{true, true, true, true, false},
		{true, true, false, true, true},
	};
	/* 192.0.2.1 port 32853: as MAPPED-ADDRESS, and as XOR-MAPPED-ADDRESS from
	 * RFC 5769 section 2.2.
	 */
	static const uint8_t mapped_address[] = {0x00U, 0x01U, 0x80U, 0x55U,
						 0xc0U, 0x00U, 0x02U, 0x01U};
	static const uint8_t xor_address[] = {0x00U, 0x01U, 0xa1U, 0x47U,
					      0xe1U, 0x12U, 0xa6U, 0x43U};
	static const char software[] = "example STUN server";
	struct net_sockaddr addr = {0};
	uint16_t out[4] = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	for (size_t i = 0; i < ARRAY_SIZE(shapes); i++) {
		/* Build the set. */
		raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
		raw_attr(&r, STUN_ATTR_XOR_MAPPED_ADDRESS, sizeof(xor_address), xor_address,
			 sizeof(xor_address));
		if (shapes[i].mapped_address) {
			raw_attr(&r, STUN_ATTR_MAPPED_ADDRESS, sizeof(mapped_address),
				 mapped_address, sizeof(mapped_address));
		}
		if (shapes[i].response_origin) {
			raw_attr(&r, TEST_ATTR_RESPONSE_ORIGIN, sizeof(mapped_address),
				 mapped_address, sizeof(mapped_address));
		}
		if (shapes[i].other_address) {
			raw_attr(&r, TEST_ATTR_OTHER_ADDRESS, sizeof(mapped_address),
				 mapped_address, sizeof(mapped_address));
		}
		if (shapes[i].software) {
			raw_attr(&r, STUN_ATTR_SOFTWARE, strlen(software), software,
				 strlen(software));
		}
		if (shapes[i].fingerprint) {
			raw_fingerprint(&r);
		}

		/* Accepted, with nothing unknown and the mapped address. */
		zassert_true(stun_has_valid_header(r.buf, r.len), "set %zu", i);
		zassert_ok(stun_parse(r.buf, r.len, &m), "set %zu", i);
		zassert_equal(stun_unknown_attrs(r.buf, r.len, &m, NULL, 0U, out, ARRAY_SIZE(out)),
			      0, "set %zu", i);
		memset(&addr, 0, sizeof(addr));
		zassert_ok(stun_get_xor_mapped_address(r.buf, r.len, &m, &addr), "set %zu", i);
		zassert_true(addr_is_ipv4(&addr, 192, 0, 2, 1, RFC5769_MAPPED_PORT), "set %zu", i);
		if (shapes[i].fingerprint) {
			zassert_ok(stun_verify_fingerprint(r.buf, r.len, &m), "set %zu", i);
		}
	}
}

/*---------------------------------------- TEST BUILDER ----------------------------------------- */

/* The builder writes the sample request of RFC 5769 section 2.4
 * (rfc5769_long_term_request) byte for byte: USERNAME, NONCE and REALM, then
 * MESSAGE-INTEGRITY under the long-term key. The sample, which has no
 * FINGERPRINT, parses back and passes MESSAGE-INTEGRITY with that key.
 */
ZTEST(stun, test_rfc5769_long_term_request_byte_exact)
{
	static const uint8_t txid[STUN_TXID_SIZE] = {
		0x78, 0xad, 0x34, 0x33, 0xc6, 0xad, 0x72, 0xc0, 0x29, 0xda, 0x41, 0x2e,
	};
	/* The six katakana characters of the RFC, in UTF-8. */
	static const char username[] = "\xe3\x83\x9e\xe3\x83\x88\xe3\x83\xaa"
				       "\xe3\x83\x83\xe3\x82\xaf\xe3\x82\xb9";
	static const char nonce[] = "f//499k954d6OL34oL9FSTvy64sA";
	static const char realm[] = "example.org";
	/* MD5(username ":" realm ":" password) with the password "TheMatrIX"
	 * (RFC 8489 section 9.2.2). The codec does not derive long-term keys, so
	 * the key is given as computed.
	 */
	static const uint8_t key[] = {
		0xe8, 0xca, 0x7a, 0xd5, 0x9d, 0x5e, 0xb0, 0x51,
		0x8e, 0x31, 0x29, 0x11, 0xd2, 0xda, 0xb2, 0xa9,
	};
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t out[TEST_MSG_SIZE] = {0};
	int len = 0;

	/* Build the message the RFC describes. */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, username));
	zassert_ok(stun_add_attr(&b, STUN_ATTR_NONCE, (const uint8_t *)nonce, strlen(nonce)));
	zassert_ok(stun_add_attr(&b, STUN_ATTR_REALM, (const uint8_t *)realm, strlen(realm)));
	zassert_ok(stun_add_integrity(&b, key, sizeof(key)));
	len = stun_builder_len(&b);

	/* Compare: the same length and the same bytes. */
	zassert_equal(len, (int)sizeof(rfc5769_long_term_request), "length %d", len);
	zassert_mem_equal(out, rfc5769_long_term_request, sizeof(rfc5769_long_term_request),
			  "the encoding differs from RFC 5769 section 2.4");

	/* The sample parses back and passes MESSAGE-INTEGRITY. Without
	 * FINGERPRINT, that check has nothing to check.
	 */
	zassert_ok(stun_parse(rfc5769_long_term_request, sizeof(rfc5769_long_term_request), &m));
	zassert_true(m.has_integrity);
	zassert_false(m.has_fingerprint);
	zassert_ok(stun_verify_integrity(rfc5769_long_term_request,
					 sizeof(rfc5769_long_term_request), &m, key, sizeof(key)));
	zassert_equal(stun_verify_fingerprint(rfc5769_long_term_request,
					      sizeof(rfc5769_long_term_request), &m), -ENOENT);
}

/* The builder writes baseline_request byte for byte: SOFTWARE, USERNAME,
 * PRIORITY, USE-CANDIDATE and XOR-MAPPED-ADDRESS in call order, closed with
 * stun_finish().
 */
ZTEST(stun, test_baseline_request_byte_exact)
{
	/* The transaction id of baseline_request runs from this byte up. */
	static const uint8_t txid_first = 0xa0U;
	/* The values baseline_request holds: PRIORITY, and XOR-MAPPED-ADDRESS
	 * 192.168.88.20 port 50000.
	 */
	static const uint32_t priority = 0x7e0000ffU;
	static const uint16_t mapped_port = 50000U;
	struct net_sockaddr addr = {0};
	struct stun_builder b = {0};
	uint8_t out[TEST_MSG_SIZE] = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	int len = 0;

	/* Build the same message as the separate encoder. */
	fill_txid(txid, txid_first);
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_software(&b, "zephyr-stun-test"));
	zassert_ok(stun_add_username(&b, check_username));
	zassert_ok(stun_add_priority(&b, priority));
	zassert_ok(stun_add_use_candidate(&b));
	addr_set_ipv4(&addr, 192, 168, 88, 20, mapped_port);
	zassert_ok(stun_add_xor_mapped_address(&b, &addr));
	len = stun_finish(&b, (const uint8_t *)baseline_password, strlen(baseline_password));

	/* Compare: the same length and the same bytes. */
	zassert_equal(len, (int)sizeof(baseline_request), "length %d", len);
	zassert_mem_equal(out, baseline_request, sizeof(baseline_request),
			  "the encoding differs from baseline_request");
}

/* A buffer too small for the message: the builder refuses with -EMSGSIZE,
 * keeps refusing once it has, and writes the message once the buffer has
 * room for every byte of it, not before. The adders of text, ERROR-CODE,
 * UNKNOWN-ATTRIBUTES and an address refuse the same way. The 16-bit header
 * Length is a limit of its own: with room left in the buffer, attributes that
 * would take it past 65535 are refused.
 */
ZTEST(stun, test_builder_overflow)
{
	/* The header, PRIORITY, MESSAGE-INTEGRITY and FINGERPRINT: 60 bytes. */
	const size_t needed = STUN_HEADER_SIZE + RAW_PRIORITY_SIZE + RAW_INTEGRITY_SIZE +
			      RAW_FINGERPRINT_SIZE;
	/* One byte short of MESSAGE-INTEGRITY, one byte short of FINGERPRINT,
	 * and just enough.
	 */
	const size_t buf_sizes[] = {
		STUN_HEADER_SIZE + RAW_PRIORITY_SIZE + RAW_INTEGRITY_SIZE - 1U,
		needed - 1U,
		needed,
	};
	/* Values for the adders: only the refusal matters. A text, one type for
	 * UNKNOWN-ATTRIBUTES, and 192.0.2.1 and 2001:db8::1 at the default STUN
	 * port (RFC 8489 section 8.1).
	 */
	static const char text[] = "text";
	const uint16_t unknown_types[] = {STUN_ATTR_PRIORITY};
	static const uint8_t ipv6[NET_IPV6_ADDR_SIZE] = {
		0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	};
	/* The attributes of a message take at most the largest multiple of 4
	 * the 16-bit header Length holds: 65532 bytes.
	 */
	static const size_t max_attrs_size = ROUND_DOWN(UINT16_MAX, RAW_WORD_SIZE);
	/* USE-CANDIDATE has no value: its 4-byte header is all of it. */
	static const size_t use_candidate_size = RAW_ATTR_HEADER_SIZE;
	/* Room for the largest message and one USE-CANDIDATE more, so that the
	 * buffer is not what refuses it.
	 */
	static uint8_t large[STUN_HEADER_SIZE + ROUND_DOWN(UINT16_MAX, RAW_WORD_SIZE) +
			     RAW_ATTR_HEADER_SIZE];
	struct net_sockaddr addr = {0};
	struct stun_builder b = {0};
	uint8_t out[TEST_MSG_SIZE] = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	int rc = 0;

	fill_txid(txid, TEST_TXID_FIRST);

	/* Too small for the header: init fails, and so does every later call. */
	stun_builder_init(&b, out, STUN_HEADER_SIZE - 1U, STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	zassert_equal(stun_add_priority(&b, TEST_PRIORITY), -EMSGSIZE);

	/* The header fits, the first attribute does not. The error stays, and
	 * stun_finish() reports it instead of writing anything.
	 */
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
	zassert_equal(stun_add_priority(&b, TEST_PRIORITY), -EMSGSIZE);
	zassert_equal(stun_add_use_candidate(&b), -EMSGSIZE);
	zassert_equal(stun_finish(&b, (const uint8_t *)baseline_password,
				  strlen(baseline_password)), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);

	/* PRIORITY, then stun_finish(): the message is written only when the
	 * buffer holds all of it.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(buf_sizes); i++) {
		stun_builder_init(&b, out, buf_sizes[i], STUN_BINDING_REQUEST, txid);
		zassert_ok(stun_add_priority(&b, TEST_PRIORITY));
		rc = stun_finish(&b, (const uint8_t *)baseline_password, strlen(baseline_password));
		if (buf_sizes[i] < needed) {
			zassert_equal(rc, -EMSGSIZE, "buffer of %zu bytes", buf_sizes[i]);
		} else {
			zassert_equal(rc, (int)needed, "buffer of %zu bytes: %d", buf_sizes[i], rc);
		}
	}

	/* The other adders: with no room past the header each refuses, and the
	 * builder remembers it.
	 */
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_username(&b, text), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_software(&b, text), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_attr(&b, STUN_ATTR_NONCE, (const uint8_t *)text, strlen(text)),
		      -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_equal(stun_add_error_code(&b, TEST_ERROR_BAD_REQUEST, NULL), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_equal(stun_add_unknown_attributes(&b, unknown_types, ARRAY_SIZE(unknown_types)),
		      -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	addr_set_ipv4(&addr, 192, 0, 2, 1, TEST_STUN_PORT);
	stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_SUCCESS_RESPONSE, txid);
	zassert_equal(stun_add_xor_mapped_address(&b, &addr), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		addr_set_ipv6(&addr, ipv6, TEST_STUN_PORT);
		stun_builder_init(&b, out, STUN_HEADER_SIZE, STUN_BINDING_SUCCESS_RESPONSE, txid);
		zassert_equal(stun_add_xor_mapped_address(&b, &addr), -EMSGSIZE);
		zassert_equal(stun_builder_len(&b), -EMSGSIZE);
	}

	/* The header Length: USE-CANDIDATE after USE-CANDIDATE fills the
	 * message to the most it holds, and one more is refused although the
	 * buffer has room for it.
	 */
	stun_builder_init(&b, large, sizeof(large), STUN_BINDING_REQUEST, txid);
	for (size_t i = 0; i < max_attrs_size / use_candidate_size; i++) {
		zassert_ok(stun_add_use_candidate(&b), "USE-CANDIDATE %zu", i);
	}
	zassert_equal(stun_builder_len(&b), (int)(STUN_HEADER_SIZE + max_attrs_size));
	zassert_equal(sys_get_be16(large + RAW_LENGTH_OFFSET), max_attrs_size);
	zassert_equal(stun_add_use_candidate(&b), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
}

/* The builder writes controlling_request byte for byte: ICE-CONTROLLING with
 * its 8-byte tie-breaker in network byte order, and the attributes after it
 * where they belong. The message then parses back with the role, and both
 * checks pass.
 */
ZTEST(stun, test_ice_controlling_builder_byte_exact)
{
	/* The transaction id of controlling_request runs from this byte up. */
	static const uint8_t txid_first = 0xc0U;
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t out[TEST_MSG_SIZE] = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	int len = 0;

	/* Build the check the controlling agent sends. */
	fill_txid(txid, txid_first);
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, check_username));
	zassert_ok(stun_add_priority(&b, check_priority));
	zassert_ok(stun_add_ice_controlling(&b, controlling_tiebreaker));
	zassert_ok(stun_add_use_candidate(&b));
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));

	/* Compare: the same length and the same bytes. */
	zassert_equal(len, (int)sizeof(controlling_request), "length %d", len);
	zassert_mem_equal(out, controlling_request, sizeof(controlling_request),
			  "the encoding differs from controlling_request");

	/* Parse it back, as the peer would. */
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_true(m.has_ice_controlling);
	zassert_false(m.has_ice_controlled);
	zassert_equal(m.ice_tiebreaker, controlling_tiebreaker);
	zassert_true(m.has_use_candidate);
	zassert_true(m.has_priority);
	zassert_equal(m.priority, check_priority);
	zassert_ok(stun_verify_fingerprint(out, (size_t)len, &m));
	zassert_ok(stun_verify_integrity(out, (size_t)len, &m, (const uint8_t *)check_password,
					 strlen(check_password)));
}

/* The builder writes controlled_request byte for byte: ICE-CONTROLLED with its
 * 8-byte tie-breaker in network byte order. The message then parses back with
 * the role, and both checks pass.
 */
ZTEST(stun, test_ice_controlled_builder_byte_exact)
{
	/* The transaction id of controlled_request runs from this byte up. */
	static const uint8_t txid_first = 0xc0U;
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t out[TEST_MSG_SIZE] = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	int len = 0;

	/* Build the check the controlled agent sends. */
	fill_txid(txid, txid_first);
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, check_username));
	zassert_ok(stun_add_priority(&b, check_priority));
	zassert_ok(stun_add_ice_controlled(&b, controlled_tiebreaker));
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));

	/* Compare: the same length and the same bytes. */
	zassert_equal(len, (int)sizeof(controlled_request), "length %d", len);
	zassert_mem_equal(out, controlled_request, sizeof(controlled_request),
			  "the encoding differs from controlled_request");

	/* Parse it back, as the peer would. */
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_true(m.has_ice_controlled);
	zassert_false(m.has_ice_controlling);
	zassert_equal(m.ice_tiebreaker, controlled_tiebreaker);
	zassert_false(m.has_use_candidate);
	zassert_true(m.has_priority);
	zassert_equal(m.priority, check_priority);
	zassert_ok(stun_verify_fingerprint(out, (size_t)len, &m));
	zassert_ok(stun_verify_integrity(out, (size_t)len, &m, (const uint8_t *)check_password,
					 strlen(check_password)));
}

/* Tie-breakers at the edges of 64 bits go through both role attributes
 * unchanged, and a role attribute needs room for its 12 bytes like any other
 * attribute with an 8-byte value.
 */
ZTEST(stun, test_role_tiebreaker_roundtrip)
{
	/* Both 32-bit halves have to keep their order: a swap would keep the
	 * symmetric values and change the asymmetric ones.
	 */
	static const uint64_t tiebreakers[] = {
		0ULL,
		1ULL,
		0x00000000ffffffffULL,
		0xffffffff00000000ULL,
		0x0000000100000000ULL,
		UINT64_MAX,
	};
	/* A role attribute on the wire: its 4-byte header and the 8-byte value. */
	static const size_t role_size = 4U + sizeof(uint64_t);
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t out[TEST_MSG_SIZE] = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);

	/* Each value as ICE-CONTROLLING, then as ICE-CONTROLLED. */
	for (size_t i = 0; i < ARRAY_SIZE(tiebreakers); i++) {
		stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
		zassert_ok(stun_add_ice_controlling(&b, tiebreakers[i]));
		len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
		zassert_true(len > 0, "stun_finish failed (%d)", len);
		zassert_ok(stun_parse(out, (size_t)len, &m));
		zassert_true(m.has_ice_controlling);
		zassert_false(m.has_ice_controlled);
		zassert_equal(m.ice_tiebreaker, tiebreakers[i], "controlling, value %zu", i);

		stun_builder_init(&b, out, sizeof(out), STUN_BINDING_REQUEST, txid);
		zassert_ok(stun_add_ice_controlled(&b, tiebreakers[i]));
		len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
		zassert_true(len > 0, "stun_finish failed (%d)", len);
		zassert_ok(stun_parse(out, (size_t)len, &m));
		zassert_true(m.has_ice_controlled);
		zassert_false(m.has_ice_controlling);
		zassert_equal(m.ice_tiebreaker, tiebreakers[i], "controlled, value %zu", i);
	}

	/* Room: one byte short of the attribute fails, exactly enough fits. */
	stun_builder_init(&b, out, STUN_HEADER_SIZE + role_size - 1U, STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_ice_controlled(&b, controlled_tiebreaker), -EMSGSIZE);
	stun_builder_init(&b, out, STUN_HEADER_SIZE + role_size, STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_ice_controlled(&b, controlled_tiebreaker));
	zassert_equal(stun_builder_len(&b), (int)(STUN_HEADER_SIZE + role_size));
}

/* ERROR-CODE: the codes 300 to 699 go through the split into class and number
 * and back, the reason phrase is sent whole or left out, a code outside the
 * range is refused without changing the message, and a response without
 * ERROR-CODE reports none.
 */
ZTEST(stun, test_error_code_builder_and_roundtrip)
{
	/* The lowest code, the highest number of a class, the first code of a
	 * class, the code of a role conflict, and the highest code.
	 */
	static const uint16_t codes[] = {300U, 399U, 400U, 487U, 500U, 699U};
	static const char reason[] = "Role Conflict";
	static const char long_reason[] = "a reason phrase that is longer than most, to show "
					  "that the builder sends it whole";
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t out[TEST_MSG_SIZE] = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);

	/* Each code comes back as it was sent, in a signed response. */
	for (size_t i = 0; i < ARRAY_SIZE(codes); i++) {
		stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
		zassert_ok(stun_add_error_code(&b, codes[i], reason), "code %u", codes[i]);
		len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
		zassert_true(len > 0, "stun_finish failed (%d)", len);
		zassert_ok(stun_parse(out, (size_t)len, &m));
		zassert_true(m.has_error_code, "code %u lost", codes[i]);
		zassert_equal(m.error_code, codes[i]);
		zassert_ok(stun_verify_integrity(out, (size_t)len, &m,
						 (const uint8_t *)check_password,
						 strlen(check_password)));
	}

	/* No reason phrase at all: the code alone is sent. */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_ok(stun_add_error_code(&b, TEST_ERROR_ROLE_CONFLICT, NULL));
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_equal(m.error_code, TEST_ERROR_ROLE_CONFLICT);
	zassert_ok(stun_find_attr(out, (size_t)len, &m, STUN_ATTR_ERROR_CODE, &value, &value_len));
	zassert_equal(value_len, RAW_ERROR_REASON_OFFSET);

	/* A long reason phrase is sent whole. */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_ok(stun_add_error_code(&b, TEST_ERROR_ROLE_CONFLICT, long_reason));
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_equal(m.error_code, TEST_ERROR_ROLE_CONFLICT);
	zassert_ok(stun_find_attr(out, (size_t)len, &m, STUN_ATTR_ERROR_CODE, &value, &value_len));
	zassert_equal(value_len, RAW_ERROR_REASON_OFFSET + strlen(long_reason));
	zassert_mem_equal(value + RAW_ERROR_REASON_OFFSET, long_reason, strlen(long_reason));

	/* A code outside 300 to 699 is refused. The message stays as it was and
	 * the builder goes on working.
	 */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_equal(stun_add_error_code(&b, 299U, reason), -EINVAL);
	zassert_equal(stun_add_error_code(&b, 700U, reason), -EINVAL);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE,
		      "a refused code must not stop the builder");
	zassert_ok(stun_add_error_code(&b, TEST_ERROR_ROLE_CONFLICT, reason));
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_equal(m.error_code, TEST_ERROR_ROLE_CONFLICT);

	/* An error response without ERROR-CODE: none is reported. */
	stun_builder_init(&b, out, sizeof(out), STUN_BINDING_ERROR_RESPONSE, txid);
	len = stun_finish(&b, (const uint8_t *)check_password, strlen(check_password));
	zassert_true(len > 0, "stun_finish failed (%d)", len);
	zassert_ok(stun_parse(out, (size_t)len, &m));
	zassert_false(m.has_error_code);
	zassert_equal(m.error_code, 0U);
}

/* stun_add_attr() writes any attribute the caller gives it: the type, the
 * length of the value without padding, the value and zero padding up to a
 * multiple of 4 bytes, and counts it in the header Length. It refuses a value
 * longer than 65535 bytes, a NULL value with a length, the closing
 * attributes, which have their own functions, and a NULL builder. A refused
 * call writes nothing and leaves the builder working.
 */
ZTEST(stun, test_add_attr)
{
	/* A value whose lengths 0 to 5 need every amount of padding, 0 to 3
	 * bytes.
	 */
	static const uint8_t value[5] = {0xa1U, 0xa2U, 0xa3U, 0xa4U, 0xa5U};
	/* Fills the buffer first, so that unwritten padding would show. */
	static const uint8_t stale_fill = 0xffU;
	/* The longest value a 16-bit Length can hold, plus one. */
	static const size_t too_long = UINT16_MAX + 1U;
	/* Where the one attribute of the message starts, and its value. */
	static const size_t attr_offset = STUN_HEADER_SIZE;
	static const size_t value_offset = STUN_HEADER_SIZE + RAW_ATTR_HEADER_SIZE;
	struct stun_builder b = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	size_t padded = 0U;

	fill_txid(txid, TEST_TXID_FIRST);

	/* Values of 0 to 5 bytes, each in a fresh message. */
	for (size_t len = 0; len <= sizeof(value); len++) {
		padded = ROUND_UP(len, RAW_WORD_SIZE);
		memset(msg, stale_fill, sizeof(msg));
		stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
		zassert_ok(stun_add_attr(&b, TEST_ATTR_UNASSIGNED, value, len), "length %zu", len);
		zassert_equal(stun_builder_len(&b), (int)(value_offset + padded));

		/* The type, the length without padding, the value, zero padding. */
		zassert_equal(sys_get_be16(msg + attr_offset), TEST_ATTR_UNASSIGNED);
		zassert_equal(sys_get_be16(msg + attr_offset + RAW_ATTR_LENGTH_OFFSET), len);
		zassert_mem_equal(msg + value_offset, value, len);
		for (size_t i = len; i < padded; i++) {
			zassert_equal(msg[value_offset + i], 0U, "padding byte %zu of length %zu",
				      i, len);
		}

		/* The header counts it, and the message is whole. */
		zassert_equal(sys_get_be16(msg + RAW_LENGTH_OFFSET), RAW_ATTR_HEADER_SIZE + padded);
		zassert_true(stun_has_valid_header(msg, value_offset + padded));
	}

	/* What does not fit an attribute, or is not for this function. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_attr(&b, TEST_ATTR_UNASSIGNED, value, too_long), -EINVAL);
	zassert_equal(stun_add_attr(&b, TEST_ATTR_UNASSIGNED, NULL, sizeof(value)), -EINVAL);
	zassert_equal(stun_add_attr(&b, STUN_ATTR_MESSAGE_INTEGRITY, value, sizeof(value)),
		      -EINVAL);
	zassert_equal(stun_add_attr(&b, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, value, sizeof(value)),
		      -EINVAL);
	zassert_equal(stun_add_attr(&b, STUN_ATTR_FINGERPRINT, value, sizeof(value)), -EINVAL);
	zassert_equal(stun_add_attr(NULL, TEST_ATTR_UNASSIGNED, value, sizeof(value)), -EINVAL);

	/* Nothing was written, and the builder still works. */
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE,
		      "a refused argument must not stop the builder or write anything");
	zassert_ok(stun_add_attr(&b, TEST_ATTR_UNASSIGNED, NULL, 0U));
}

/* stun_builder_len() gives the length of the message so far: the header
 * alone, each attribute as it is added, and the closed message, the same
 * however often it is asked. It gives -EINVAL for a NULL builder and the
 * error a builder has remembered, here -EMSGSIZE from an attribute that did
 * not fit.
 */
ZTEST(stun, test_builder_len)
{
	struct stun_builder b = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);

	/* The header, then PRIORITY, asked twice. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
	zassert_ok(stun_add_priority(&b, TEST_PRIORITY));
	zassert_equal(stun_builder_len(&b), (int)(STUN_HEADER_SIZE + RAW_PRIORITY_SIZE));
	zassert_equal(stun_builder_len(&b), (int)(STUN_HEADER_SIZE + RAW_PRIORITY_SIZE),
		      "asking twice changes nothing");

	/* The closed message. */
	len = stun_finish_plain(&b);
	zassert_equal(len, (int)(STUN_HEADER_SIZE + RAW_PRIORITY_SIZE + RAW_FINGERPRINT_SIZE));
	zassert_equal(stun_builder_len(&b), len);

	/* A NULL builder. */
	zassert_equal(stun_builder_len(NULL), -EINVAL);

	/* The error an attribute that did not fit left behind: the buffer has
	 * room for the header and half of PRIORITY.
	 */
	stun_builder_init(&b, msg, STUN_HEADER_SIZE + RAW_ATTR_HEADER_SIZE, STUN_BINDING_REQUEST,
			  txid);
	zassert_equal(stun_add_priority(&b, TEST_PRIORITY), -EMSGSIZE);
	zassert_equal(stun_builder_len(&b), -EMSGSIZE);
}

/* A call out of order: it returns -EPERM, and neither the message in buf nor
 * the builder changes.
 */
#define EXPECT_EPERM(builder, buf, call)                                                           \
	do {                                                                                       \
		uint8_t before_[sizeof(buf)];                                                      \
		int len_ = stun_builder_len(&(builder));                                           \
                                                                                                   \
		memcpy(before_, (buf), sizeof(buf));                                               \
		zassert_equal((call), -EPERM, #call);                                              \
		zassert_ok((builder).err, #call " must not stop the builder");                     \
		zassert_equal(stun_builder_len(&(builder)), len_,                                  \
			      #call " must not change the length");                                \
		zassert_mem_equal((buf), before_, sizeof(buf),                                     \
				  #call " must not touch the message");                            \
	} while (false)

/* The closing attributes come in this order and only in it (RFC 8489 sections
 * 9 and 14.7): MESSAGE-INTEGRITY, then MESSAGE-INTEGRITY-SHA256, then
 * FINGERPRINT. From the first of them on, every call that would add an
 * attribute out of order is refused with -EPERM and changes nothing, and the
 * message built is one the parser takes and both checks pass.
 */
ZTEST(stun, test_closing_attributes_order)
{
	/* Any values for the attributes that come too late. */
	static const uint32_t late_priority = 1U;
	static const char late_text[] = "late";
	static const uint64_t late_tiebreaker = 1U;
	const uint8_t *key = (const uint8_t *)check_password;
	const size_t key_len = strlen(check_password);
	struct net_sockaddr addr = {0};
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);
	addr_set_ipv4(&addr, 192, 0, 2, 1, TEST_STUN_PORT);

	/* After MESSAGE-INTEGRITY: no ordinary attribute, and not a second one. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, check_username));
	zassert_ok(stun_add_priority(&b, check_priority));
	zassert_ok(stun_add_integrity(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_add_priority(&b, late_priority));
	EXPECT_EPERM(b, msg, stun_add_use_candidate(&b));
	EXPECT_EPERM(b, msg, stun_add_username(&b, late_text));
	EXPECT_EPERM(b, msg, stun_add_software(&b, late_text));
	EXPECT_EPERM(b, msg, stun_add_error_code(&b, TEST_ERROR_BAD_REQUEST, NULL));
	EXPECT_EPERM(b, msg, stun_add_xor_mapped_address(&b, &addr));
	EXPECT_EPERM(b, msg, stun_add_ice_controlling(&b, late_tiebreaker));
	EXPECT_EPERM(b, msg, stun_add_ice_controlled(&b, late_tiebreaker));
	EXPECT_EPERM(b, msg, stun_add_unknown_attributes(&b, NULL, 0U));
	EXPECT_EPERM(b, msg, stun_add_attr(&b, TEST_ATTR_UNASSIGNED, NULL, 0U));
	EXPECT_EPERM(b, msg, stun_add_integrity(&b, key, key_len));

	/* After MESSAGE-INTEGRITY-SHA256: neither integrity attribute again. */
	zassert_ok(stun_add_integrity_sha256(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_add_integrity(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_add_integrity_sha256(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_add_priority(&b, late_priority));

	/* After FINGERPRINT: nothing at all, closing included. */
	zassert_ok(stun_add_fingerprint(&b));
	EXPECT_EPERM(b, msg, stun_add_fingerprint(&b));
	EXPECT_EPERM(b, msg, stun_add_integrity(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_add_integrity_sha256(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_add_priority(&b, late_priority));
	EXPECT_EPERM(b, msg, stun_finish(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_finish_plain(&b));

	/* What was built parses, and all three checks pass. */
	len = stun_builder_len(&b);
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_ok(stun_verify_integrity(msg, (size_t)len, &m, key, key_len));
	zassert_ok(stun_verify_integrity_sha256(msg, (size_t)len, &m, key, key_len,
						STUN_INTEGRITY_SHA256_MAX_LEN));
	zassert_ok(stun_verify_fingerprint(msg, (size_t)len, &m));

	/* MESSAGE-INTEGRITY cannot follow MESSAGE-INTEGRITY-SHA256. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_integrity_sha256(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_add_integrity(&b, key, key_len));

	/* A message closed with stun_finish() cannot be closed again. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_true(stun_finish(&b, key, key_len) > 0);
	EXPECT_EPERM(b, msg, stun_finish(&b, key, key_len));
	EXPECT_EPERM(b, msg, stun_finish_plain(&b));
}

/* n copies of the character utf8 (1 to 4 bytes) in text, ended with '\0'. */
static const char *repeat(char *text, size_t text_size, const char *utf8, size_t n)
{
	size_t char_len = strlen(utf8);

	zassert_true(n * char_len < text_size, "the text does not fit");
	for (size_t i = 0; i < n; i++) {
		memcpy(text + i * char_len, utf8, char_len);
	}
	text[n * char_len] = '\0';
	return text;
}

/* The builder keeps to the limits RFC 8489 puts on text it sends: USERNAME
 * shorter than 509 bytes (section 14.3), SOFTWARE and the reason phrase of
 * ERROR-CODE shorter than 128 characters whatever they take in bytes
 * (sections 14.14 and 14.8), and UTF-8 with no bad lead or continuation byte.
 * A string over the limit, broken or missing is refused with -EINVAL and
 * nothing is written; one at the limit is sent whole.
 */
ZTEST(stun, test_text_limits_on_sending)
{
	/* Characters to repeat: ASCII letters of one byte, and the euro sign
	 * U+20AC of three bytes.
	 */
	static const char one_byte[] = "a";
	static const char euro[] = "\xe2\x82\xac";
	/* Broken UTF-8: a two-byte lead followed by an ASCII byte instead of a
	 * continuation byte, and a continuation byte with no lead.
	 */
	static const char bad_continuation[] = "ok\xc3\x28";
	static const char lone_continuation[] = "ok\x80";
	/* Room for the longest strings the test builds: 128 characters of up to
	 * 4 bytes, and the ending '\0'.
	 */
	char text[4U * (STUN_TEXT_MAX_CHARS + 2U)] = {0};
	uint8_t msg[TEST_LONG_MSG_SIZE] = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	size_t euro_text_len = 0U;
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);
	euro_text_len = STUN_TEXT_MAX_CHARS * strlen(euro);

	/* USERNAME: one byte over the limit is refused, the limit is sent. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_username(&b, repeat(text, sizeof(text), one_byte,
						   STUN_USERNAME_MAX_LEN + 1U)), -EINVAL);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
	zassert_ok(stun_add_username(&b, repeat(text, sizeof(text), one_byte,
						STUN_USERNAME_MAX_LEN)));
	len = stun_builder_len(&b);
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_equal(m.username_len, STUN_USERNAME_MAX_LEN);

	/* SOFTWARE: one character over the limit is refused, in one-byte and in
	 * three-byte characters, and so is broken UTF-8. The limit is sent,
	 * however many bytes it takes.
	 */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_software(&b, repeat(text, sizeof(text), one_byte,
						   STUN_TEXT_MAX_CHARS + 1U)), -EINVAL);
	zassert_equal(stun_add_software(&b, repeat(text, sizeof(text), euro,
						   STUN_TEXT_MAX_CHARS + 1U)), -EINVAL);
	zassert_equal(stun_add_software(&b, bad_continuation), -EINVAL);
	zassert_equal(stun_add_software(&b, lone_continuation), -EINVAL);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
	zassert_ok(stun_add_software(&b, repeat(text, sizeof(text), euro, STUN_TEXT_MAX_CHARS)));
	zassert_equal(stun_builder_len(&b),
		      (int)(STUN_HEADER_SIZE + RAW_ATTR_HEADER_SIZE +
			    ROUND_UP(euro_text_len, RAW_WORD_SIZE)),
		      "127 three-byte characters, padded");
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_software(&b, repeat(text, sizeof(text), one_byte,
						STUN_TEXT_MAX_CHARS)));

	/* The reason phrase of ERROR-CODE: the same limits. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_equal(stun_add_error_code(&b, TEST_ERROR_BAD_REQUEST,
					  repeat(text, sizeof(text), one_byte,
						 STUN_TEXT_MAX_CHARS + 1U)), -EINVAL);
	zassert_equal(stun_add_error_code(&b, TEST_ERROR_BAD_REQUEST,
					  repeat(text, sizeof(text), euro,
						 STUN_TEXT_MAX_CHARS + 1U)), -EINVAL);
	zassert_equal(stun_add_error_code(&b, TEST_ERROR_BAD_REQUEST, bad_continuation), -EINVAL);
	zassert_equal(stun_add_error_code(&b, TEST_ERROR_BAD_REQUEST, lone_continuation), -EINVAL);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
	zassert_ok(stun_add_error_code(&b, TEST_ERROR_BAD_REQUEST,
				       repeat(text, sizeof(text), euro, STUN_TEXT_MAX_CHARS)));
	len = stun_builder_len(&b);
	zassert_equal(len,
		      (int)(STUN_HEADER_SIZE + RAW_ATTR_HEADER_SIZE +
			    ROUND_UP(RAW_ERROR_REASON_OFFSET + euro_text_len, RAW_WORD_SIZE)),
		      "the phrase is sent whole");
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_equal(m.error_code, TEST_ERROR_BAD_REQUEST);

	/* A string that is not there: NULL is allowed only for the reason
	 * phrase.
	 */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_username(&b, NULL), -EINVAL);
	zassert_equal(stun_add_software(&b, NULL), -EINVAL);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
}

/* The builder writes the 420 response unknown_attributes_error byte for byte,
 * closed with stun_finish_plain(), and it parses back. UNKNOWN-ATTRIBUTES
 * holds 2 bytes per type, padded to a multiple of 4; an empty list is allowed.
 * A NULL list with a count, or more types than a 16-bit Length can hold, is
 * refused with -EINVAL and nothing is written.
 */
ZTEST(stun, test_unknown_attributes_error_byte_exact)
{
	/* The types unknown_attributes_error lists, comprehension-required ones
	 * nobody assigns.
	 */
	static const uint16_t types[] = {0x7f00U, 0x7f01U, 0x7f02U};
	static const uint16_t unknown_attribute = 420U;
	static const char reason[] = "Unknown Attribute";
	/* One type more than a 16-bit Length can hold at 2 bytes each. */
	static const size_t too_many = UINT16_MAX / sizeof(uint16_t) + 1U;
	/* The transaction id of unknown_attributes_error runs from this byte up. */
	static const uint8_t txid_first = 0xb0U;
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;
	int len = 0;

	fill_txid(txid, txid_first);

	/* Build the 420 response and compare it byte for byte. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_ok(stun_add_error_code(&b, unknown_attribute, reason));
	zassert_ok(stun_add_unknown_attributes(&b, types, ARRAY_SIZE(types)));
	len = stun_finish_plain(&b);
	zassert_equal(len, (int)sizeof(unknown_attributes_error), "length %d", len);
	zassert_mem_equal(msg, unknown_attributes_error, sizeof(unknown_attributes_error),
			  "the encoding differs from unknown_attributes_error");

	/* It parses back with the code, the list and FINGERPRINT. */
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	zassert_equal(m.error_code, unknown_attribute);
	zassert_false(m.has_integrity);
	zassert_ok(stun_verify_fingerprint(msg, (size_t)len, &m));
	zassert_ok(stun_find_attr(msg, (size_t)len, &m, STUN_ATTR_UNKNOWN_ATTRIBUTES, &value,
				  &value_len));
	zassert_equal(value_len, sizeof(types));

	/* No, one and two types: 2 bytes each, padded to 4. */
	for (size_t n = 0; n < ARRAY_SIZE(types); n++) {
		stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_ERROR_RESPONSE, txid);
		zassert_ok(stun_add_unknown_attributes(&b, types, n));
		len = stun_builder_len(&b);
		zassert_equal(len, (int)(STUN_HEADER_SIZE + RAW_ATTR_HEADER_SIZE +
					 ROUND_UP(n * sizeof(uint16_t), RAW_WORD_SIZE)));
		zassert_ok(stun_parse(msg, (size_t)len, &m));
		zassert_ok(stun_find_attr(msg, (size_t)len, &m, STUN_ATTR_UNKNOWN_ATTRIBUTES,
					  &value, &value_len));
		zassert_equal(value_len, n * sizeof(uint16_t));
	}

	/* A NULL list with a count, and too many types: nothing is written. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_ERROR_RESPONSE, txid);
	zassert_equal(stun_add_unknown_attributes(&b, NULL, ARRAY_SIZE(types)), -EINVAL);
	zassert_equal(stun_add_unknown_attributes(&b, types, too_many), -EINVAL);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
}

/* The builder writes the integrity attributes byte for byte. MESSAGE-INTEGRITY
 * alone is checked against RFC 5769 section 2.4 by
 * test_rfc5769_long_term_request_byte_exact. MESSAGE-INTEGRITY-SHA256 alone is
 * checked here against RFC 8489 Appendix B.1 (rfc8489_request): its four
 * long-term attributes are taken from the RFC sample and added with
 * stun_add_attr(), then the builder signs with the long-term key. Both
 * attributes followed by FINGERPRINT are checked against sha256_both_request:
 * a request signed both ways, as RFC 8489 section 9.1.2 asks when the agents
 * do not know which algorithm they share, closed with stun_finish_plain().
 */
ZTEST(stun, test_integrity_attributes_byte_exact)
{
	/* The attributes of rfc8489_request ahead of MESSAGE-INTEGRITY-SHA256:
	 * USERHASH, NONCE, REALM and PASSWORD-ALGORITHM (RFC 8489 section 18.3).
	 */
	const uint16_t long_term_types[] = {
		TEST_ATTR_USERHASH,
		STUN_ATTR_NONCE,
		STUN_ATTR_REALM,
		TEST_ATTR_PASSWORD_ALGORITHM,
	};
	/* The transaction id of sha256_both_request runs from this byte up. */
	static const uint8_t txid_first = 0xc0U;
	const uint8_t *key = (const uint8_t *)check_password;
	const size_t key_len = strlen(check_password);
	uint8_t long_term_key[PSA_HASH_LENGTH(PSA_ALG_SHA_256)] = {0};
	struct stun_builder b = {0};
	struct stun_msg rfc = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;
	int len = 0;

	/* MESSAGE-INTEGRITY-SHA256 alone: rebuild the RFC 8489 sample from its
	 * attributes and its transaction id.
	 */
	rfc8489_long_term_key(long_term_key);
	zassert_ok(stun_parse(rfc8489_request, sizeof(rfc8489_request), &rfc));
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, rfc.txid);
	for (size_t i = 0; i < ARRAY_SIZE(long_term_types); i++) {
		zassert_ok(stun_find_attr(rfc8489_request, sizeof(rfc8489_request), &rfc,
					  long_term_types[i], &value, &value_len),
			   "type 0x%04x", long_term_types[i]);
		zassert_ok(stun_add_attr(&b, long_term_types[i], value, value_len),
			   "type 0x%04x", long_term_types[i]);
	}
	zassert_ok(stun_add_integrity_sha256(&b, long_term_key, sizeof(long_term_key)));
	len = stun_builder_len(&b);
	zassert_equal(len, (int)sizeof(rfc8489_request), "length %d", len);
	zassert_mem_equal(msg, rfc8489_request, sizeof(rfc8489_request),
			  "the encoding differs from RFC 8489 Appendix B.1");

	/* Both, then FINGERPRINT through stun_finish_plain(). */
	fill_txid(txid, txid_first);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_username(&b, check_username));
	zassert_ok(stun_add_priority(&b, check_priority));
	zassert_ok(stun_add_integrity(&b, key, key_len));
	zassert_ok(stun_add_integrity_sha256(&b, key, key_len));
	len = stun_finish_plain(&b);
	zassert_equal(len, (int)sizeof(sha256_both_request), "length %d", len);
	zassert_mem_equal(msg, sha256_both_request, sizeof(sha256_both_request),
			  "the encoding differs from sha256_both_request");
}

/* A failed signature leaves the message as it was. A missing or empty key is
 * a refused argument: -EINVAL, nothing remembered. A key the PSA backend
 * cannot take makes the HMAC fail: -EIO, remembered in the builder, which
 * then refuses every later call, while the message and its header Length stay
 * as they were before the call, a valid message without the signature.
 */
ZTEST(stun, test_integrity_failure_leaves_the_message_intact)
{
	/* Any priorities: only the lengths matter. */
	static const uint32_t priority = 1U;
	static const uint32_t later_priority = 2U;
	const uint8_t *key = (const uint8_t *)check_password;
	const size_t key_len = strlen(check_password);
	struct stun_builder b = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	/* The message as it was before the failed call. */
	uint8_t before[sizeof(msg)] = {0};

	fill_txid(txid, TEST_TXID_FIRST);

	/* A missing or empty key: refused, nothing remembered or written. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_ok(stun_add_priority(&b, priority));
	zassert_equal(stun_add_integrity(&b, NULL, key_len), -EINVAL);
	zassert_equal(stun_add_integrity(&b, key, 0U), -EINVAL);
	zassert_equal(stun_add_integrity_sha256(&b, NULL, key_len), -EINVAL);
	zassert_equal(stun_add_integrity_sha256(&b, key, 0U), -EINVAL);
	zassert_equal(stun_finish(&b, NULL, key_len), -EINVAL);
	zassert_equal(stun_builder_len(&b), (int)(STUN_HEADER_SIZE + RAW_PRIORITY_SIZE));

	/* MESSAGE-INTEGRITY with a key the backend refuses: -EIO is remembered
	 * and every later call refused, and the message stays whole without it.
	 */
	memcpy(before, msg, sizeof(msg));
	zassert_equal(stun_add_integrity(&b, oversized_key, sizeof(oversized_key)), -EIO);
	zassert_equal(stun_builder_len(&b), -EIO);
	zassert_equal(b.msg_len, STUN_HEADER_SIZE + RAW_PRIORITY_SIZE);
	zassert_mem_equal(msg, before, sizeof(msg));
	zassert_true(stun_has_valid_header(msg, b.msg_len));
	zassert_equal(stun_add_priority(&b, later_priority), -EIO);
	zassert_equal(stun_builder_len(&b), -EIO);
	zassert_equal(stun_finish(&b, key, key_len), -EIO);
	zassert_equal(stun_finish_plain(&b), -EIO);

	/* MESSAGE-INTEGRITY-SHA256 with such a key: the same. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	memcpy(before, msg, sizeof(msg));
	zassert_equal(stun_add_integrity_sha256(&b, oversized_key, sizeof(oversized_key)), -EIO);
	zassert_equal(b.msg_len, STUN_HEADER_SIZE);
	zassert_mem_equal(msg, before, sizeof(msg));

	/* stun_finish() with such a key: the same. */
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_finish(&b, oversized_key, sizeof(oversized_key)), -EIO);
	zassert_equal(stun_builder_len(&b), -EIO);
	zassert_equal(b.msg_len, STUN_HEADER_SIZE);
}

/* stun_builder_init() with a NULL buffer or transaction id, or a message type
 * with one of its two top bits set, remembers -EINVAL, and every later call
 * returns it; with a NULL builder it does nothing.
 */
ZTEST(stun, test_builder_init_bad_arguments)
{
	/* A Binding request with one of the two top bits set, which RFC 8489
	 * section 5 keeps zero: bit 14, then bit 15.
	 */
	const uint16_t top_bit_types[] = {
		(uint16_t)(STUN_BINDING_REQUEST | BIT(14)),
		(uint16_t)(STUN_BINDING_REQUEST | BIT(15)),
	};
	struct stun_builder b = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};

	fill_txid(txid, TEST_TXID_FIRST);

	/* No buffer. */
	stun_builder_init(&b, NULL, sizeof(msg), STUN_BINDING_REQUEST, txid);
	zassert_equal(stun_add_priority(&b, TEST_PRIORITY), -EINVAL);
	zassert_equal(stun_builder_len(&b), -EINVAL);

	/* No transaction id. */
	b = (struct stun_builder){0};
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_REQUEST, NULL);
	zassert_equal(stun_builder_len(&b), -EINVAL);
	zassert_equal(stun_add_priority(&b, TEST_PRIORITY), -EINVAL);
	zassert_equal(stun_finish_plain(&b), -EINVAL);

	/* A message type with a top bit set. */
	for (size_t i = 0; i < ARRAY_SIZE(top_bit_types); i++) {
		b = (struct stun_builder){0};
		stun_builder_init(&b, msg, sizeof(msg), top_bit_types[i], txid);
		zassert_equal(stun_add_priority(&b, TEST_PRIORITY), -EINVAL, "type 0x%04x",
			      top_bit_types[i]);
		zassert_equal(stun_builder_len(&b), -EINVAL, "type 0x%04x", top_bit_types[i]);
	}

	/* No builder: nothing happens, nothing crashes. */
	stun_builder_init(NULL, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
}

/*---------------------------------------- TEST ADDRESS ----------------------------------------- */

/* The XOR-MAPPED-ADDRESS of the sample IPv6 response of RFC 5769 section 2.3
 * (rfc5769_response_ipv6) decodes to 2001:db8:1234:5678:11:2233:4455:6677
 * port 32853. Without the IPv6 stack the message still parses, and the
 * address is -EAFNOSUPPORT. Either way it passes both checks: FINGERPRINT, and
 * MESSAGE-INTEGRITY with the password of the RFC.
 */
ZTEST(stun, test_xor_mapped_address_ipv6)
{
	struct net_sockaddr addr = {0};
	struct stun_msg m = {0};
	int rc = 0;

	/* A valid message, whatever the IP stack is built with, that passes both
	 * checks.
	 */
	zassert_ok(stun_parse(rfc5769_response_ipv6, sizeof(rfc5769_response_ipv6), &m));
	zassert_ok(stun_verify_fingerprint(rfc5769_response_ipv6, sizeof(rfc5769_response_ipv6),
					   &m));
	zassert_ok(stun_verify_integrity(rfc5769_response_ipv6, sizeof(rfc5769_response_ipv6), &m,
					 (const uint8_t *)rfc5769_password,
					 strlen(rfc5769_password)));
	rc = stun_get_xor_mapped_address(rfc5769_response_ipv6, sizeof(rfc5769_response_ipv6), &m,
					 &addr);

	/* The address, or -EAFNOSUPPORT without the IPv6 stack. */
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		zassert_ok(rc);
		zassert_true(addr_is_ipv6(&addr, rfc5769_mapped_ipv6, RFC5769_MAPPED_PORT),
			     "expected 2001:db8:1234:5678:11:2233:4455:6677 port 32853");
	} else {
		zassert_equal(rc, -EAFNOSUPPORT, "an IPv6 address without the IPv6 stack");
	}
}

/* The builder writes an IPv6 XOR-MAPPED-ADDRESS byte for byte as RFC 5769
 * section 2.3 has it (rfc5769_response_ipv6): with the same transaction id and
 * address, the value comes out the same. For IPv6 the address is XORed with
 * the magic cookie and the transaction id (RFC 8489 section 14.2), so this
 * also checks that the transaction id goes in. The rest of the RFC sample is
 * not compared: its SOFTWARE is padded with spaces, the builder pads with
 * zeros.
 */
ZTEST(stun, test_xor_mapped_address_ipv6_byte_exact)
{
	/* The transaction id of RFC 5769 section 2.3. */
	static const uint8_t txid[STUN_TXID_SIZE] = {
		0xb7, 0xe7, 0xa7, 0x01, 0xbc, 0x34, 0xd6, 0x86, 0xfa, 0x87, 0xdf, 0xae,
	};
	struct net_sockaddr_in6 *sin6 = NULL;
	struct net_sockaddr addr = {0};
	struct stun_builder b = {0};
	struct stun_msg rfc = {0};
	struct stun_msg built = {0};
	const uint8_t *rfc_value = NULL;
	const uint8_t *built_value = NULL;
	uint16_t rfc_len = 0U;
	uint16_t built_len = 0U;
	uint8_t msg[TEST_MSG_SIZE] = {0};
	int len = 0;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6);

	/* The value in the RFC sample. */
	zassert_ok(stun_parse(rfc5769_response_ipv6, sizeof(rfc5769_response_ipv6), &rfc));
	zassert_ok(stun_find_attr(rfc5769_response_ipv6, sizeof(rfc5769_response_ipv6), &rfc,
				  STUN_ATTR_XOR_MAPPED_ADDRESS, &rfc_value, &rfc_len));

	/* The same address written by the builder. */
	sin6 = net_sin6(&addr);
	sin6->sin6_family = NET_AF_INET6;
	sin6->sin6_port = net_htons(RFC5769_MAPPED_PORT);
	memcpy(sin6->sin6_addr.s6_addr, rfc5769_mapped_ipv6, sizeof(rfc5769_mapped_ipv6));
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);
	zassert_ok(stun_add_xor_mapped_address(&b, &addr));
	len = stun_finish_plain(&b);
	zassert_true(len > 0, "stun_finish_plain failed (%d)", len);
	zassert_ok(stun_parse(msg, (size_t)len, &built));
	zassert_ok(stun_find_attr(msg, (size_t)len, &built, STUN_ATTR_XOR_MAPPED_ADDRESS,
				  &built_value, &built_len));

	/* The same bytes. */
	zassert_equal(built_len, rfc_len);
	zassert_mem_equal(built_value, rfc_value, rfc_len,
			  "the value differs from RFC 5769 section 2.3");
}

/* Build a success response with one address attribute of attr_type, parse it
 * and read the address back into out.
 */
static void roundtrip_address(uint16_t attr_type, const struct net_sockaddr *in,
			      struct net_sockaddr *out)
{
	struct stun_builder b = {0};
	struct stun_msg m = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};
	int len = 0;

	fill_txid(txid, TEST_TXID_FIRST);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);
	zassert_ok(stun_add_xor_address(&b, attr_type, in));
	len = stun_builder_len(&b);
	zassert_true(len > 0, "stun_builder_len failed (%d)", len);
	zassert_ok(stun_parse(msg, (size_t)len, &m));
	memset(out, 0, sizeof(*out));
	zassert_ok(stun_get_xor_address(msg, (size_t)len, &m, attr_type, out));
}

/* An address written with stun_add_xor_address() reads back unchanged with
 * stun_get_xor_address(), for XOR-MAPPED-ADDRESS and for XOR-PEER-ADDRESS of
 * TURN, a type in the same format the codec does not otherwise know: IPv4 and,
 * with the IPv6 stack, IPv6 addresses, at the lowest and highest port and the
 * highest IPv4 address.
 */
ZTEST(stun, test_xor_address_roundtrip)
{
	static const uint16_t attr_types[] = {STUN_ATTR_XOR_MAPPED_ADDRESS,
					      TEST_ATTR_XOR_PEER_ADDRESS};
	/* The lowest and highest port, and one in between. */
	static const uint16_t lowest_port = 1U;
	static const uint16_t highest_port = UINT16_MAX;
	static const uint16_t middle_port = 50000U;
	/* 2001:db8::1:2. */
	static const uint8_t ipv6_short[NET_IPV6_ADDR_SIZE] = {
		0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02,
	};
	struct net_sockaddr in = {0};
	struct net_sockaddr out = {0};

	for (size_t i = 0; i < ARRAY_SIZE(attr_types); i++) {
		/* IPv4: 203.0.113.7 (a documentation address) at the lowest port,
		 * and 255.255.255.255 at the highest.
		 */
		addr_set_ipv4(&in, 203, 0, 113, 7, lowest_port);
		roundtrip_address(attr_types[i], &in, &out);
		zassert_true(addr_is_ipv4(&out, 203, 0, 113, 7, lowest_port), "type 0x%04x",
			     attr_types[i]);
		addr_set_ipv4(&in, 255, 255, 255, 255, highest_port);
		roundtrip_address(attr_types[i], &in, &out);
		zassert_true(addr_is_ipv4(&out, 255, 255, 255, 255, highest_port), "type 0x%04x",
			     attr_types[i]);

		/* IPv6, with the IPv6 stack only. */
		if (!IS_ENABLED(CONFIG_NET_IPV6)) {
			continue;
		}
		addr_set_ipv6(&in, ipv6_short, middle_port);
		roundtrip_address(attr_types[i], &in, &out);
		zassert_true(addr_is_ipv6(&out, ipv6_short, middle_port), "type 0x%04x",
			     attr_types[i]);
		addr_set_ipv6(&in, rfc5769_mapped_ipv6, lowest_port);
		roundtrip_address(attr_types[i], &in, &out);
		zassert_true(addr_is_ipv6(&out, rfc5769_mapped_ipv6, lowest_port), "type 0x%04x",
			     attr_types[i]);
	}
}

/* The address functions with nothing to find and with bad arguments: an
 * absent address attribute is -ENOENT, a NULL argument -EINVAL, and so is a
 * closing attribute type passed to stun_add_xor_address(). A refused call
 * writes nothing and leaves the builder working.
 */
ZTEST(stun, test_xor_address_absent_and_bad_arguments)
{
	struct net_sockaddr addr = {0};
	struct stun_builder b = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};

	/* A response without an address, read as an extension's type. Without
	 * XOR-MAPPED-ADDRESS it is checked in test_absent_attributes.
	 */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, TEST_ATTR_XOR_PEER_ADDRESS, &addr),
		      -ENOENT);

	/* NULL arguments to the getter. */
	zassert_equal(stun_get_xor_mapped_address(r.buf, r.len, &m, NULL), -EINVAL);
	zassert_equal(stun_get_xor_mapped_address(NULL, r.len, &m, &addr), -EINVAL);
	zassert_equal(stun_get_xor_mapped_address(r.buf, r.len, NULL, &addr), -EINVAL);

	/* NULL arguments, and FINGERPRINT, to the builder. */
	fill_txid(txid, TEST_TXID_FIRST);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);
	addr_set_ipv4(&addr, 192, 0, 2, 1, TEST_STUN_PORT);
	zassert_equal(stun_add_xor_mapped_address(&b, NULL), -EINVAL);
	zassert_equal(stun_add_xor_mapped_address(NULL, &addr), -EINVAL);
	zassert_equal(stun_add_xor_address(&b, STUN_ATTR_FINGERPRINT, &addr), -EINVAL,
		      "FINGERPRINT is not an address attribute");

	/* Nothing written, and the builder still works. */
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE,
		      "a refused argument must not stop the builder or write anything");
}

/* stun_add_xor_mapped_address() refuses an address of a family it cannot
 * write with -EAFNOSUPPORT: NET_AF_UNSPEC, and IPv6 when the IPv6 stack is not
 * built in. The refusal writes nothing and leaves the builder working.
 */
ZTEST(stun, test_xor_address_unsupported_family)
{
	/* An IPv4 XOR-MAPPED-ADDRESS on the wire: a 4-byte attribute header, then
	 * the reserved byte, the family, the port and the 4-byte address.
	 */
	static const size_t ipv4_attr_size = RAW_ATTR_HEADER_SIZE + 4U + 4U;
	/* Bigger than struct net_sockaddr when the IPv6 stack is not built in:
	 * only its family may be looked at.
	 */
	struct net_sockaddr_in6 sin6 = {.sin6_family = NET_AF_INET6};
	struct net_sockaddr addr = {0};
	struct stun_builder b = {0};
	uint8_t txid[STUN_TXID_SIZE] = {0};
	uint8_t msg[TEST_MSG_SIZE] = {0};

	fill_txid(txid, TEST_TXID_FIRST);
	stun_builder_init(&b, msg, sizeof(msg), STUN_BINDING_SUCCESS_RESPONSE, txid);

	/* No family. */
	addr_set_ipv4(&addr, 192, 0, 2, 1, TEST_STUN_PORT);
	net_sin(&addr)->sin_family = NET_AF_UNSPEC;
	zassert_equal(stun_add_xor_mapped_address(&b, &addr), -EAFNOSUPPORT);
	zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);

	/* IPv6 without the IPv6 stack. */
	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		zassert_equal(stun_add_xor_mapped_address(&b, (struct net_sockaddr *)&sin6),
			      -EAFNOSUPPORT, "IPv6 without the IPv6 stack");
		zassert_equal(stun_builder_len(&b), (int)STUN_HEADER_SIZE);
	}

	/* The builder still works. */
	addr_set_ipv4(&addr, 192, 0, 2, 1, TEST_STUN_PORT);
	zassert_ok(stun_add_xor_mapped_address(&b, &addr));
	zassert_equal(stun_builder_len(&b), (int)(STUN_HEADER_SIZE + ipv4_attr_size));
}

/* The parser does not judge an attribute it does not know, so a malformed
 * address in an extension's type, here XOR-PEER-ADDRESS of TURN, does not
 * make the message malformed. stun_get_xor_address() checks it when the caller
 * reads it: a length that does not fit the family is -EBADMSG, and a family
 * other than IPv4 and IPv6 is -EAFNOSUPPORT (RFC 8489 section 6.3.3).
 */
ZTEST(stun, test_xor_address_of_an_extension_is_checked_by_the_getter)
{
	/* Address values with the family in their second byte: 3, which no RFC
	 * assigns; IPv4 (1) in the 20 bytes of an IPv6 value; and IPv4 one byte
	 * short of its 8.
	 */
	static const uint8_t family_3[8] = {0x00U, 0x03U};
	static const uint8_t ipv4_in_ipv6_len[20] = {0x00U, 0x01U};
	static const uint8_t ipv4_short[7] = {0x00U, 0x01U};
	struct net_sockaddr addr = {0};
	struct raw_msg r = {0};
	struct stun_msg m = {0};

	/* Family 3: the message parses, the getter does not support it. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, TEST_ATTR_XOR_PEER_ADDRESS, sizeof(family_3), family_3, sizeof(family_3));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, TEST_ATTR_XOR_PEER_ADDRESS, &addr),
		      -EAFNOSUPPORT, "address family 3");

	/* IPv4 in the length of IPv6: the message parses, the value is
	 * malformed.
	 */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, TEST_ATTR_XOR_PEER_ADDRESS, sizeof(ipv4_in_ipv6_len), ipv4_in_ipv6_len,
		 sizeof(ipv4_in_ipv6_len));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, TEST_ATTR_XOR_PEER_ADDRESS, &addr),
		      -EBADMSG, "IPv4 in 20 bytes");

	/* IPv4 one byte short: the same. */
	raw_init(&r, STUN_BINDING_SUCCESS_RESPONSE, TEST_TXID_FILL);
	raw_attr(&r, TEST_ATTR_XOR_PEER_ADDRESS, sizeof(ipv4_short), ipv4_short,
		 sizeof(ipv4_short));
	zassert_ok(stun_parse(r.buf, r.len, &m));
	zassert_equal(stun_get_xor_address(r.buf, r.len, &m, TEST_ATTR_XOR_PEER_ADDRESS, &addr),
		      -EBADMSG, "address of 7 bytes");
}

/*--------------------------------------- TEST INTEGRITY ---------------------------------------- */

/* The three checks refuse bad arguments with -EINVAL: a NULL datagram, a NULL
 * result of stun_parse(), a datagram length that is not the parsed one and,
 * in the two checks that take a key, a NULL or empty key.
 */
ZTEST(stun, test_integrity_bad_arguments)
{
	const uint8_t *req = sha256_both_request;
	const size_t len = sizeof(sha256_both_request);
	const uint8_t *key = (const uint8_t *)check_password;
	const size_t key_len = strlen(check_password);
	struct stun_msg m = {0};

	zassert_ok(stun_parse(req, len, &m));

	/* MESSAGE-INTEGRITY. */
	zassert_equal(stun_verify_integrity(req, len, &m, NULL, key_len), -EINVAL);
	zassert_equal(stun_verify_integrity(req, len, &m, key, 0U), -EINVAL);
	zassert_equal(stun_verify_integrity(req, len, NULL, key, key_len), -EINVAL);
	zassert_equal(stun_verify_integrity(NULL, len, &m, key, key_len), -EINVAL);
	zassert_equal(stun_verify_integrity(req, len - RAW_WORD_SIZE, &m, key, key_len), -EINVAL,
		      "msg was parsed from another buffer length");

	/* MESSAGE-INTEGRITY-SHA256. */
	zassert_equal(stun_verify_integrity_sha256(req, len, &m, NULL, key_len,
						   STUN_INTEGRITY_SHA256_MAX_LEN), -EINVAL);
	zassert_equal(stun_verify_integrity_sha256(req, len, &m, key, 0U,
						   STUN_INTEGRITY_SHA256_MAX_LEN), -EINVAL);
	zassert_equal(stun_verify_integrity_sha256(req, len, NULL, key, key_len,
						   STUN_INTEGRITY_SHA256_MAX_LEN), -EINVAL);
	zassert_equal(stun_verify_integrity_sha256(NULL, len, &m, key, key_len,
						   STUN_INTEGRITY_SHA256_MAX_LEN), -EINVAL);
	zassert_equal(stun_verify_integrity_sha256(req, len - RAW_WORD_SIZE, &m, key, key_len,
						   STUN_INTEGRITY_SHA256_MAX_LEN),
		      -EINVAL, "msg was parsed from another buffer length");

	/* FINGERPRINT. */
	zassert_equal(stun_verify_fingerprint(NULL, len, &m), -EINVAL);
	zassert_equal(stun_verify_fingerprint(req, len, NULL), -EINVAL);
	zassert_equal(stun_verify_fingerprint(req, len - RAW_WORD_SIZE, &m), -EINVAL,
		      "msg was parsed from another buffer length");
}

/* A key the PSA backend cannot take makes the check -EIO, not -EBADMSG: a
 * caller has to be able to tell "forged" from "could not check".
 */
ZTEST(stun, test_integrity_backend_failure_is_not_a_mismatch)
{
	const uint8_t *req = sha256_both_request;
	const size_t len = sizeof(sha256_both_request);
	struct stun_msg m = {0};

	zassert_ok(stun_parse(req, len, &m));
	zassert_equal(stun_verify_integrity(req, len, &m, oversized_key, sizeof(oversized_key)),
		      -EIO);
	zassert_equal(stun_verify_integrity_sha256(req, len, &m, oversized_key,
						   sizeof(oversized_key),
						   STUN_INTEGRITY_SHA256_MAX_LEN), -EIO);
}

/* A truncated MESSAGE-INTEGRITY-SHA256 (sha256_truncated_request, 16 bytes)
 * passes only when the caller's STUN Usage allows that length (RFC 8489
 * section 14.6): with every min_len above its 16 bytes it is refused, with 16
 * it passes, and a change in its last bit is caught. A min_len outside 16 to
 * 32 in steps of 4 is -EINVAL.
 */
ZTEST(stun, test_integrity_sha256_truncated)
{
	/* min_len values outside the rule: none, a word short of the shortest,
	 * not a multiple of 4, and a word past the longest.
	 */
	const size_t bad_min_lens[] = {
		0U,
		STUN_INTEGRITY_SHA256_MIN_LEN - RAW_WORD_SIZE,
		STUN_INTEGRITY_SHA256_MIN_LEN + 1U,
		STUN_INTEGRITY_SHA256_MAX_LEN + RAW_WORD_SIZE,
	};
	const uint8_t *req = sha256_truncated_request;
	const size_t len = sizeof(sha256_truncated_request);
	const uint8_t *key = (const uint8_t *)check_password;
	const size_t key_len = strlen(check_password);
	uint8_t msg[sizeof(sha256_truncated_request)] = {0};
	struct stun_msg m = {0};

	zassert_ok(stun_parse(req, len, &m));
	zassert_equal(m.integrity_sha256_len, STUN_INTEGRITY_SHA256_MIN_LEN);

	/* Refused unless the caller allows the truncation. */
	for (size_t min_len = STUN_INTEGRITY_SHA256_MIN_LEN + RAW_WORD_SIZE;
	     min_len <= STUN_INTEGRITY_SHA256_MAX_LEN; min_len += RAW_WORD_SIZE) {
		zassert_equal(stun_verify_integrity_sha256(req, len, &m, key, key_len, min_len),
			      -EBADMSG, "truncated, and min_len %zu did not allow it", min_len);
	}
	zassert_ok(stun_verify_integrity_sha256(req, len, &m, key, key_len,
						STUN_INTEGRITY_SHA256_MIN_LEN));

	/* The last bit of the truncated value. */
	memcpy(msg, req, len);
	msg[m.integrity_sha256_off + RAW_ATTR_HEADER_SIZE + STUN_INTEGRITY_SHA256_MIN_LEN - 1U] ^=
		TEST_LOW_BIT;
	zassert_equal(stun_verify_integrity_sha256(msg, len, &m, key, key_len,
						   STUN_INTEGRITY_SHA256_MIN_LEN),
		      -EBADMSG, "the last bit of the truncated value");

	/* min_len outside the rule. */
	for (size_t i = 0; i < ARRAY_SIZE(bad_min_lens); i++) {
		zassert_equal(stun_verify_integrity_sha256(req, len, &m, key, key_len,
							   bad_min_lens[i]),
			      -EINVAL, "min_len %zu", bad_min_lens[i]);
	}
}

/* The sample request of RFC 8489 Appendix B.1 as corrected by erratum 6268
 * (rfc8489_request) passes MESSAGE-INTEGRITY-SHA256 with its long-term key,
 * which rfc8489_long_term_key() derives (RFC 8489 section 18.5.1.2). Another
 * key, a changed bit in the last byte of the 32-byte value and a changed bit
 * in REALM, which the HMAC covers, each fail it. An attribute appended after
 * it lies past the integrity boundary (RFC 8489 section 9): the message still
 * passes, and the attribute is not reported, with or without a FINGERPRINT
 * computed over the forged message. USERHASH and PASSWORD-ALGORITHM belong to
 * long-term credentials, which the codec leaves to its caller, so
 * stun_unknown_attrs() lists them.
 */
ZTEST(stun, test_integrity_sha256_rfc8489_vector)
{
	uint8_t key[PSA_HASH_LENGTH(PSA_ALG_SHA_256)] = {0};
	uint8_t msg[sizeof(rfc8489_request)] = {0};
	uint16_t unknown[4] = {0};
	struct raw_msg forged = {0};
	struct stun_msg m = {0};
	struct stun_msg forged_m = {0};
	const uint8_t *realm = NULL;
	uint16_t realm_len = 0U;
	size_t value_last_offset = 0U;

	rfc8489_long_term_key(key);

	/* The request passes MESSAGE-INTEGRITY-SHA256. */
	zassert_true(stun_has_valid_header(rfc8489_request, sizeof(rfc8489_request)));
	zassert_ok(stun_parse(rfc8489_request, sizeof(rfc8489_request), &m));
	zassert_true(m.has_integrity_sha256);
	zassert_false(m.has_integrity);
	zassert_ok(stun_verify_integrity_sha256(rfc8489_request, sizeof(rfc8489_request), &m, key,
						sizeof(key), STUN_INTEGRITY_SHA256_MAX_LEN));

	/* Another key: the one the builder tests sign with. */
	zassert_equal(stun_verify_integrity_sha256(rfc8489_request, sizeof(rfc8489_request), &m,
						   (const uint8_t *)check_password,
						   strlen(check_password),
						   STUN_INTEGRITY_SHA256_MAX_LEN),
		      -EBADMSG, "another key");

	/* A changed bit in the last byte of the value. */
	value_last_offset = m.integrity_sha256_off + RAW_ATTR_HEADER_SIZE +
			    STUN_INTEGRITY_SHA256_MAX_LEN - 1U;
	memcpy(msg, rfc8489_request, sizeof(msg));
	msg[value_last_offset] ^= TEST_TOP_BIT;
	zassert_equal(stun_verify_integrity_sha256(msg, sizeof(msg), &m, key, sizeof(key),
						   STUN_INTEGRITY_SHA256_MAX_LEN),
		      -EBADMSG, "a bit of the value");

	/* A changed bit in REALM, which the HMAC covers. */
	zassert_ok(stun_find_attr(rfc8489_request, sizeof(rfc8489_request), &m, STUN_ATTR_REALM,
				  &realm, &realm_len));
	memcpy(msg, rfc8489_request, sizeof(msg));
	msg[realm - rfc8489_request] ^= TEST_LOW_BIT;
	zassert_equal(stun_verify_integrity_sha256(msg, sizeof(msg), &m, key, sizeof(key),
						   STUN_INTEGRITY_SHA256_MAX_LEN),
		      -EBADMSG, "a bit of REALM");

	/* PRIORITY appended after MESSAGE-INTEGRITY-SHA256, with the header Length
	 * counting it, as anyone on the path can do without the key: the
	 * message still passes, and PRIORITY is not reported.
	 */
	forged.len = sizeof(rfc8489_request);
	memcpy(forged.buf, rfc8489_request, forged.len);
	raw_u32(&forged, STUN_ATTR_PRIORITY, sizeof(uint32_t), TEST_PRIORITY);
	zassert_ok(stun_parse(forged.buf, forged.len, &forged_m));
	zassert_ok(stun_verify_integrity_sha256(forged.buf, forged.len, &forged_m, key,
						sizeof(key), STUN_INTEGRITY_SHA256_MAX_LEN),
		   "the forged message must still pass MESSAGE-INTEGRITY-SHA256");
	zassert_false(forged_m.has_priority, "PRIORITY follows MESSAGE-INTEGRITY-SHA256");

	/* The same with FINGERPRINT computed over the forged message, which
	 * takes no key either: it passes both checks, and PRIORITY is still not
	 * reported.
	 */
	raw_fingerprint(&forged);
	zassert_ok(stun_parse(forged.buf, forged.len, &forged_m));
	zassert_ok(stun_verify_integrity_sha256(forged.buf, forged.len, &forged_m, key,
						sizeof(key), STUN_INTEGRITY_SHA256_MAX_LEN),
		   "the forged message must still pass MESSAGE-INTEGRITY-SHA256");
	zassert_ok(stun_verify_fingerprint(forged.buf, forged.len, &forged_m),
		   "the forged message must still pass FINGERPRINT");
	zassert_false(forged_m.has_priority, "PRIORITY follows MESSAGE-INTEGRITY-SHA256");

	/* The long-term attributes are the caller's. */
	zassert_equal(stun_unknown_attrs(rfc8489_request, sizeof(rfc8489_request), &m, NULL, 0U,
					 unknown, ARRAY_SIZE(unknown)), 2);
	zassert_equal(unknown[0], TEST_ATTR_USERHASH, "USERHASH");
	zassert_equal(unknown[1], TEST_ATTR_PASSWORD_ALGORITHM, "PASSWORD-ALGORITHM");
}

/* A stun_msg damaged after stun_parse(), with an offset that points inside the
 * 20-byte message header, is refused with -EINVAL by everything that reads at
 * it: stun_find_attr() and the three checks. No attribute starts inside the
 * header, so the offset cannot be right.
 */
ZTEST(stun, test_damaged_msg_offsets_are_refused)
{
	const uint8_t *req = sha256_both_request;
	const size_t len = sizeof(sha256_both_request);
	const uint8_t *key = (const uint8_t *)check_password;
	const size_t key_len = strlen(check_password);
	struct stun_msg good = {0};
	struct stun_msg m = {0};
	const uint8_t *value = NULL;
	uint16_t value_len = 0U;

	zassert_ok(stun_parse(req, len, &good));

	/* Every offset inside the header, for each of the three attributes. */
	for (size_t offset = 0U; offset < STUN_HEADER_SIZE; offset++) {
		m = good;
		m.integrity_off = offset;
		zassert_equal(stun_verify_integrity(req, len, &m, key, key_len), -EINVAL,
			      "integrity_off %zu", offset);
		zassert_equal(stun_find_attr(req, len, &m, STUN_ATTR_MESSAGE_INTEGRITY, &value,
					     &value_len), -EINVAL, "integrity_off %zu", offset);

		m = good;
		m.integrity_sha256_off = offset;
		zassert_equal(stun_verify_integrity_sha256(req, len, &m, key, key_len,
							   STUN_INTEGRITY_SHA256_MAX_LEN),
			      -EINVAL, "integrity_sha256_off %zu", offset);

		m = good;
		m.fingerprint_off = offset;
		zassert_equal(stun_verify_fingerprint(req, len, &m), -EINVAL,
			      "fingerprint_off %zu", offset);
	}
}
