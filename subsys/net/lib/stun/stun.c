/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * STUN message codec, RFC 8489.
 *
 * The integrity attributes are where the format has its subtleties:
 *  - MESSAGE-INTEGRITY and MESSAGE-INTEGRITY-SHA256 are an HMAC over the message
 *    up to the attribute itself, with the header Length standing at the end of
 *    that attribute — not at the end of the message. A message on the wire has
 *    a different Length whenever something follows, so both signing and
 *    verifying feed the HMAC a patched header.
 *  - FINGERPRINT is a CRC-32 over the message up to itself, with the header
 *    Length counting it. It is the last attribute, so that is the Length the
 *    message has anyway.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/stun.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include <psa/crypto.h>

/* The log gets what the return codes cannot carry. Why a message is refused is
 * a debug record and never more: -EBADMSG stands for every rule of the parser,
 * and traffic from the network must not be able to fill the log at the levels
 * a product runs with. An error record is for a fault of this system, such as
 * a PSA backend that fails. Neither keys nor integrity values nor the contents
 * of attributes are ever logged: types, lengths and offsets only.
 */
LOG_MODULE_REGISTER(net_stun, CONFIG_STUN_LOG_LEVEL);

#define FINGERPRINT_XOR 0x5354554eU
#define FINGERPRINT_LEN 4U

#define ATTR_HEADER_SIZE 4U

/* MAPPED-ADDRESS and XOR-MAPPED-ADDRESS (RFC 8489 §14.1, §14.2): a reserved
 * byte, the family, the port, then 4 or 16 bytes of address.
 */
#define ADDR_FAMILY_IPV4 0x01U
#define ADDR_FAMILY_IPV6 0x02U
#define ADDR_LEN_IPV4    8U
#define ADDR_LEN_IPV6    20U

/* Attribute types the parser checks once per message: a bit each, to tell the
 * first occurrence from the later ones (RFC 8489 §14).
 */
enum {
	SEEN_MAPPED_ADDRESS = BIT(0),
	SEEN_USERNAME = BIT(1),
	SEEN_ERROR_CODE = BIT(2),
	SEEN_UNKNOWN_ATTRIBUTES = BIT(3),
	SEEN_REALM = BIT(4),
	SEEN_NONCE = BIT(5),
	SEEN_XOR_MAPPED_ADDRESS = BIT(6),
	SEEN_PRIORITY = BIT(7),
	SEEN_USE_CANDIDATE = BIT(8),
	SEEN_SOFTWARE = BIT(9),
	SEEN_ICE_CONTROLLED = BIT(10),
	SEEN_ICE_CONTROLLING = BIT(11),
};

/* How far a message under construction has got: the closing attributes come in
 * this order, and only in this order (RFC 8489 §9, §14.7).
 */
enum {
	STAGE_ATTRS = 0,
	STAGE_INTEGRITY,
	STAGE_INTEGRITY_SHA256,
	STAGE_FINGERPRINT,
};

static size_t padded_len(size_t vlen)
{
	return ROUND_UP(vlen, 4U);
}

/* Number of UTF-8 characters in a string: every byte but the continuation ones. */
static size_t utf8_chars(const char *s)
{
	size_t n = 0;

	for (; *s != '\0'; s++) {
		if (((uint8_t)*s & 0xC0U) != 0x80U) {
			n++;
		}
	}
	return n;
}

/* --- HMAC through the PSA Crypto API --- */

/* Import the key and feed the operation what RFC 8489 §14.5 and §14.6 define as
 * the HMAC input: the message ahead of offset `cutoff`, with `patched_len` in
 * place of the header Length. The caller's buffer is not modified: the type,
 * the patched Length and the rest go in as three pieces.
 */
static int mac_start(psa_mac_operation_t *op, psa_key_id_t *kid, psa_algorithm_t alg, bool verify,
		     const uint8_t *key, size_t key_len, const uint8_t *msg, size_t cutoff,
		     uint16_t patched_len)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t len_be[2];
	psa_status_t status;

	psa_set_key_usage_flags(&attr,
				verify ? PSA_KEY_USAGE_VERIFY_MESSAGE : PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(key_len));
	status = psa_import_key(&attr, key, key_len, kid);
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA does not take the key (%d)", (int)status);
		return -EIO;
	}

	sys_put_be16(patched_len, len_be);
	status = verify ? psa_mac_verify_setup(op, *kid, alg) : psa_mac_sign_setup(op, *kid, alg);
	if (status == PSA_SUCCESS) {
		status = psa_mac_update(op, msg, 2U);
	}
	if (status == PSA_SUCCESS) {
		status = psa_mac_update(op, len_be, sizeof(len_be));
	}
	if (status == PSA_SUCCESS) {
		status = psa_mac_update(op, msg + 4U, cutoff - 4U);
	}
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA cannot start the HMAC (%d)", (int)status);
		(void)psa_mac_abort(op);
		(void)psa_destroy_key(*kid);
		return -EIO;
	}
	return 0;
}

static int mac_sign(psa_algorithm_t alg, const uint8_t *key, size_t key_len, const uint8_t *msg,
		    size_t cutoff, uint16_t patched_len, uint8_t *mac, size_t mac_len)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_key_id_t kid = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t written = 0;
	int rc;

	rc = mac_start(&op, &kid, alg, false, key, key_len, msg, cutoff, patched_len);
	if (rc != 0) {
		return rc;
	}
	status = psa_mac_sign_finish(&op, mac, mac_len, &written);
	if (status != PSA_SUCCESS) {
		(void)psa_mac_abort(&op);
	}
	(void)psa_destroy_key(kid);
	if (status != PSA_SUCCESS || written != mac_len) {
		LOG_ERR("PSA cannot compute the HMAC (%d)", (int)status);
		return -EIO;
	}
	return 0;
}

/* The comparison is the backend's, in constant time. */
static int mac_verify(psa_algorithm_t alg, const uint8_t *key, size_t key_len, const uint8_t *msg,
		      size_t cutoff, uint16_t patched_len, const uint8_t *mac, size_t mac_len)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_key_id_t kid = PSA_KEY_ID_NULL;
	psa_status_t status;
	int rc;

	rc = mac_start(&op, &kid, alg, true, key, key_len, msg, cutoff, patched_len);
	if (rc != 0) {
		return rc;
	}
	status = psa_mac_verify_finish(&op, mac, mac_len);
	if (status != PSA_SUCCESS) {
		(void)psa_mac_abort(&op);
	}
	(void)psa_destroy_key(kid);
	if (status == PSA_SUCCESS) {
		return 0;
	}
	/* A value that does not match is the message's fault, not the backend's. */
	if (status == PSA_ERROR_INVALID_SIGNATURE) {
		return -EBADMSG;
	}
	LOG_ERR("PSA cannot check the HMAC (%d)", (int)status);
	return -EIO;
}

/* --- Parsing --- */

/* What every STUN message starts with, and nothing else on the same port does
 * (RFC 8489 §5): two zero bits and the magic cookie.
 */
static bool has_stun_marks(const uint8_t *buf, size_t len)
{
	return len >= STUN_HEADER_SIZE && (buf[0] & 0xC0U) == 0U &&
	       sys_get_be32(buf + 4) == STUN_MAGIC_COOKIE;
}

/* The message accounts for the whole datagram, in whole 32-bit words. */
static bool length_ok(const uint8_t *buf, size_t len)
{
	uint16_t length = sys_get_be16(buf + 2);

	return (length % 4U) == 0U && (size_t)length + STUN_HEADER_SIZE == len;
}

bool stun_is_message(const uint8_t *buf, size_t len)
{
	return buf != NULL && has_stun_marks(buf, len) && length_ok(buf, len);
}

static bool first_time(uint32_t *seen, uint32_t bit)
{
	if ((*seen & bit) != 0U) {
		return false;
	}
	*seen |= bit;
	return true;
}

static bool address_ok(const uint8_t *val, uint16_t alen)
{
	return (alen == ADDR_LEN_IPV4 && val[1] == ADDR_FAMILY_IPV4) ||
	       (alen == ADDR_LEN_IPV6 && val[1] == ADDR_FAMILY_IPV6);
}

static bool sha256_len_ok(size_t len)
{
	return len >= STUN_INTEGRITY_SHA256_MIN_LEN && len <= STUN_INTEGRITY_SHA256_MAX_LEN &&
	       (len % 4U) == 0U;
}

static void record_sha256(struct stun_msg *out, size_t off, uint16_t alen)
{
	out->has_integrity_sha256 = true;
	out->integrity_sha256_off = off;
	out->integrity_sha256_len = alen;
}

/* One attribute ahead of the integrity boundary. Of each type only the first
 * occurrence is looked at, and that one has to be well-formed.
 */
static int parse_attr(const uint8_t *buf, size_t off, uint16_t atype, uint16_t alen,
		      struct stun_msg *out, uint32_t *seen)
{
	const uint8_t *val = buf + off + ATTR_HEADER_SIZE;

	switch (atype) {
	case STUN_ATTR_MESSAGE_INTEGRITY:
		if (alen != STUN_INTEGRITY_LEN) {
			LOG_DBG("MESSAGE-INTEGRITY of %u bytes at offset %zu", alen, off);
			return -EBADMSG;
		}
		out->has_integrity = true;
		out->integrity_off = off;
		out->attrs_end = off;
		break;
	case STUN_ATTR_MESSAGE_INTEGRITY_SHA256:
		if (!sha256_len_ok(alen)) {
			LOG_DBG("MESSAGE-INTEGRITY-SHA256 of %u bytes at offset %zu", alen, off);
			return -EBADMSG;
		}
		record_sha256(out, off, alen);
		out->attrs_end = off;
		break;
	case STUN_ATTR_USERNAME:
		if (!first_time(seen, SEEN_USERNAME)) {
			break;
		}
		if (alen > STUN_TEXT_MAX_LEN) {
			LOG_DBG("USERNAME of %u bytes at offset %zu", alen, off);
			return -EBADMSG;
		}
		out->username = val;
		out->username_len = alen;
		break;
	case STUN_ATTR_SOFTWARE:
	case STUN_ATTR_REALM:
	case STUN_ATTR_NONCE: {
		uint32_t bit = (atype == STUN_ATTR_SOFTWARE) ? SEEN_SOFTWARE
			       : (atype == STUN_ATTR_REALM)  ? SEEN_REALM
							     : SEEN_NONCE;

		if (first_time(seen, bit) && alen > STUN_TEXT_MAX_LEN) {
			LOG_DBG("text attribute 0x%04x of %u bytes at offset %zu", atype, alen,
				off);
			return -EBADMSG;
		}
		break;
	}
	case STUN_ATTR_PRIORITY:
		if (!first_time(seen, SEEN_PRIORITY)) {
			break;
		}
		if (alen != 4U) {
			LOG_DBG("PRIORITY of %u bytes at offset %zu", alen, off);
			return -EBADMSG;
		}
		out->has_priority = true;
		out->priority = sys_get_be32(val);
		break;
	case STUN_ATTR_USE_CANDIDATE:
		if (!first_time(seen, SEEN_USE_CANDIDATE)) {
			break;
		}
		if (alen != 0U) {
			LOG_DBG("USE-CANDIDATE of %u bytes at offset %zu", alen, off);
			return -EBADMSG;
		}
		out->has_use_candidate = true;
		break;
	case STUN_ATTR_ERROR_CODE: {
		/* 2 reserved bytes (ignored), the class in the low 3 bits, the
		 * number, then the reason phrase (RFC 8489 §14.8).
		 */
		uint8_t cls, number;

		if (!first_time(seen, SEEN_ERROR_CODE)) {
			break;
		}
		if (alen < 4U || alen - 4U > STUN_TEXT_MAX_LEN) {
			LOG_DBG("ERROR-CODE of %u bytes at offset %zu", alen, off);
			return -EBADMSG;
		}
		cls = val[2] & 0x07U;
		number = val[3];
		if (cls < 3U || cls > 6U || number > 99U) {
			LOG_DBG("ERROR-CODE with class %u and number %u at offset %zu", cls, number,
				off);
			return -EBADMSG;
		}
		out->has_error_code = true;
		out->error_code = (uint16_t)(cls * 100U + number);
		break;
	}
	case STUN_ATTR_ICE_CONTROLLING:
	case STUN_ATTR_ICE_CONTROLLED: {
		bool controlling = (atype == STUN_ATTR_ICE_CONTROLLING);

		if (!first_time(seen, controlling ? SEEN_ICE_CONTROLLING : SEEN_ICE_CONTROLLED)) {
			break;
		}
		if (alen != 8U) {
			LOG_DBG("ICE role attribute 0x%04x of %u bytes at offset %zu", atype, alen,
				off);
			return -EBADMSG;
		}
		/* An agent has one role (RFC 8445 §7.1.3). */
		if (out->has_ice_controlling || out->has_ice_controlled) {
			LOG_DBG("both ICE roles, the second one at offset %zu", off);
			return -EBADMSG;
		}
		out->has_ice_controlling = controlling;
		out->has_ice_controlled = !controlling;
		out->ice_tiebreaker = sys_get_be64(val);
		break;
	}
	case STUN_ATTR_MAPPED_ADDRESS:
	case STUN_ATTR_XOR_MAPPED_ADDRESS: {
		uint32_t bit = (atype == STUN_ATTR_MAPPED_ADDRESS) ? SEEN_MAPPED_ADDRESS
								   : SEEN_XOR_MAPPED_ADDRESS;

		if (first_time(seen, bit) && !address_ok(val, alen)) {
			/* The family is read only if the attribute has one. */
			LOG_DBG("address 0x%04x at offset %zu: %u bytes, family %u", atype, off,
				alen, (alen >= 2U) ? val[1] : 0U);
			return -EBADMSG;
		}
		break;
	}
	case STUN_ATTR_UNKNOWN_ATTRIBUTES:
		/* A list of 16-bit attribute types (RFC 8489 §14.13). */
		if (first_time(seen, SEEN_UNKNOWN_ATTRIBUTES) && (alen % 2U) != 0U) {
			LOG_DBG("UNKNOWN-ATTRIBUTES of %u bytes at offset %zu", alen, off);
			return -EBADMSG;
		}
		break;
	default:
		break;
	}
	return 0;
}

int stun_parse(const uint8_t *buf, size_t len, struct stun_msg *out)
{
	size_t off = STUN_HEADER_SIZE;
	uint32_t seen = 0U;

	if (buf == NULL || out == NULL || !has_stun_marks(buf, len)) {
		return -EINVAL;
	}
	if (!length_ok(buf, len)) {
		LOG_DBG("length %u does not make the %zu bytes of the datagram",
			sys_get_be16(buf + 2), len);
		return -EBADMSG;
	}

	memset(out, 0, sizeof(*out));
	out->type = sys_get_be16(buf);
	out->length = sys_get_be16(buf + 2);
	memcpy(out->txid, buf + 8, STUN_TXID_SIZE);
	out->attrs_end = len;

	/* off and len are multiples of 4, so while off < len there is room for
	 * an attribute header.
	 */
	while (off < len) {
		uint16_t atype = sys_get_be16(buf + off);
		uint16_t alen = sys_get_be16(buf + off + 2);
		int rc = 0;

		if (padded_len(alen) > len - off - ATTR_HEADER_SIZE) {
			LOG_DBG("attribute 0x%04x at offset %zu: its %u bytes run past the end",
				atype, off, alen);
			return -EBADMSG;
		}
		/* FINGERPRINT is the last attribute (RFC 8489 §14.7). */
		if (out->has_fingerprint) {
			LOG_DBG("attribute 0x%04x at offset %zu follows FINGERPRINT", atype, off);
			return -EBADMSG;
		}

		if (atype == STUN_ATTR_FINGERPRINT) {
			if (alen != FINGERPRINT_LEN) {
				LOG_DBG("FINGERPRINT of %u bytes at offset %zu", alen, off);
				return -EBADMSG;
			}
			out->has_fingerprint = true;
			out->fingerprint_off = off;
		} else if (out->attrs_end == len) {
			rc = parse_attr(buf, off, atype, alen, out, &seen);
		} else if (atype == STUN_ATTR_MESSAGE_INTEGRITY_SHA256 && out->has_integrity &&
			   !out->has_integrity_sha256) {
			/* Besides FINGERPRINT, the one attribute that counts
			 * after MESSAGE-INTEGRITY (RFC 8489 §9).
			 */
			if (!sha256_len_ok(alen)) {
				LOG_DBG("MESSAGE-INTEGRITY-SHA256 of %u bytes at offset %zu", alen,
					off);
				return -EBADMSG;
			}
			record_sha256(out, off, alen);
		} else {
			/* Past the integrity boundary: ignored, unexamined. */
		}
		if (rc != 0) {
			return rc;
		}
		off += ATTR_HEADER_SIZE + padded_len(alen);
	}

	return 0;
}

uint16_t stun_msg_type(uint16_t method, enum stun_class cls)
{
	uint16_t c = (uint16_t)cls;

	/* The 14 bits are M11..M7 C1 M6..M4 C0 M3..M0 (RFC 8489 §5). */
	return (uint16_t)((method & 0x000FU) | ((method & 0x0070U) << 1) |
			  ((method & 0x0F80U) << 2) | ((c & 0x1U) << 4) | ((c & 0x2U) << 7));
}

uint16_t stun_msg_method(uint16_t type)
{
	return (uint16_t)((type & 0x000FU) | ((type & 0x00E0U) >> 1) | ((type & 0x3E00U) >> 2));
}

enum stun_class stun_msg_class(uint16_t type)
{
	return (enum stun_class)(((type >> 4) & 0x1U) | ((type >> 7) & 0x2U));
}

/* msg has to be what stun_parse() made of these very bytes; this is the part of
 * that which can be checked.
 */
static bool msg_matches(const uint8_t *buf, size_t len, const struct stun_msg *msg)
{
	return buf != NULL && msg != NULL && (msg->length % 4U) == 0U &&
	       len == STUN_HEADER_SIZE + (size_t)msg->length &&
	       msg->attrs_end >= STUN_HEADER_SIZE && msg->attrs_end <= len;
}

/* An attribute the parser recorded the offset of still lies inside the buffer. */
static bool attr_in_bounds(size_t len, size_t off, size_t vlen)
{
	return off <= len && len - off >= ATTR_HEADER_SIZE && len - off - ATTR_HEADER_SIZE >= vlen;
}

int stun_find_attr(const uint8_t *buf, size_t len, const struct stun_msg *msg, uint16_t type,
		   const uint8_t **val, uint16_t *vlen)
{
	size_t off;

	if (val == NULL || vlen == NULL || !msg_matches(buf, len, msg)) {
		return -EINVAL;
	}

	switch (type) {
	case STUN_ATTR_MESSAGE_INTEGRITY:
		if (!msg->has_integrity) {
			return -ENOENT;
		}
		off = msg->integrity_off;
		break;
	case STUN_ATTR_MESSAGE_INTEGRITY_SHA256:
		if (!msg->has_integrity_sha256) {
			return -ENOENT;
		}
		off = msg->integrity_sha256_off;
		break;
	case STUN_ATTR_FINGERPRINT:
		if (!msg->has_fingerprint) {
			return -ENOENT;
		}
		off = msg->fingerprint_off;
		break;
	default:
		off = STUN_HEADER_SIZE;
		while (off < msg->attrs_end && sys_get_be16(buf + off) != type) {
			off += ATTR_HEADER_SIZE + padded_len(sys_get_be16(buf + off + 2));
		}
		if (off >= msg->attrs_end) {
			return -ENOENT;
		}
		break;
	}

	if (!attr_in_bounds(len, off, 0U) ||
	    !attr_in_bounds(len, off, padded_len(sys_get_be16(buf + off + 2)))) {
		return -EINVAL;
	}
	*val = buf + off + ATTR_HEADER_SIZE;
	*vlen = sys_get_be16(buf + off + 2);
	return 0;
}

/* The comprehension-required attributes this codec knows the meaning of. */
static bool codec_understands(uint16_t type)
{
	switch (type) {
	case STUN_ATTR_MAPPED_ADDRESS:
	case STUN_ATTR_USERNAME:
	case STUN_ATTR_MESSAGE_INTEGRITY:
	case STUN_ATTR_ERROR_CODE:
	case STUN_ATTR_UNKNOWN_ATTRIBUTES:
	case STUN_ATTR_REALM:
	case STUN_ATTR_NONCE:
	case STUN_ATTR_MESSAGE_INTEGRITY_SHA256:
	case STUN_ATTR_XOR_MAPPED_ADDRESS:
	case STUN_ATTR_PRIORITY:
	case STUN_ATTR_USE_CANDIDATE:
		return true;
	default:
		return false;
	}
}

static bool type_in_list(const uint16_t *list, size_t n, uint16_t type)
{
	for (size_t i = 0; i < n; i++) {
		if (list[i] == type) {
			return true;
		}
	}
	return false;
}

/* Does `type` occur among the attributes ahead of offset `end`? */
static bool type_occurs_before(const uint8_t *buf, size_t end, uint16_t type)
{
	size_t off = STUN_HEADER_SIZE;

	while (off < end) {
		if (sys_get_be16(buf + off) == type) {
			return true;
		}
		off += ATTR_HEADER_SIZE + padded_len(sys_get_be16(buf + off + 2));
	}
	return false;
}

int stun_unknown_attrs(const uint8_t *buf, size_t len, const struct stun_msg *msg,
		       const uint16_t *known, size_t n_known, uint16_t *out, size_t cap)
{
	size_t off = STUN_HEADER_SIZE;
	size_t count = 0;

	if (!msg_matches(buf, len, msg) || (known == NULL && n_known != 0U) ||
	    (out == NULL && cap != 0U)) {
		return -EINVAL;
	}

	while (off < msg->attrs_end) {
		uint16_t atype = sys_get_be16(buf + off);

		/* 0x8000 and up are comprehension-optional (RFC 8489 §14). */
		if (atype < 0x8000U && !codec_understands(atype) &&
		    !type_in_list(known, n_known, atype) && !type_occurs_before(buf, off, atype)) {
			if (count < cap) {
				out[count] = atype;
			}
			count++;
		}
		off += ATTR_HEADER_SIZE + padded_len(sys_get_be16(buf + off + 2));
	}
	return (int)count;
}

int stun_verify_integrity(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			  const uint8_t *key, size_t key_len)
{
	size_t off;

	if (key == NULL || key_len == 0U || !msg_matches(buf, len, msg)) {
		return -EINVAL;
	}
	if (!msg->has_integrity) {
		return -ENOENT;
	}
	off = msg->integrity_off;
	if (!attr_in_bounds(len, off, STUN_INTEGRITY_LEN)) {
		return -EINVAL;
	}

	return mac_verify(
		PSA_ALG_HMAC(PSA_ALG_SHA_1), key, key_len, buf, off,
		(uint16_t)(off + ATTR_HEADER_SIZE + STUN_INTEGRITY_LEN - STUN_HEADER_SIZE),
		buf + off + ATTR_HEADER_SIZE, STUN_INTEGRITY_LEN);
}

int stun_verify_integrity_sha256(const uint8_t *buf, size_t len, const struct stun_msg *msg,
				 const uint8_t *key, size_t key_len, size_t min_len)
{
	psa_algorithm_t alg = PSA_ALG_HMAC(PSA_ALG_SHA_256);
	size_t off, mac_len;

	if (key == NULL || key_len == 0U || !sha256_len_ok(min_len) ||
	    !msg_matches(buf, len, msg)) {
		return -EINVAL;
	}
	if (!msg->has_integrity_sha256) {
		return -ENOENT;
	}
	off = msg->integrity_sha256_off;
	mac_len = msg->integrity_sha256_len;
	if (!sha256_len_ok(mac_len) || !attr_in_bounds(len, off, mac_len)) {
		return -EINVAL;
	}
	/* Truncated further than the caller's STUN Usage allows (RFC 8489 §14.6). */
	if (mac_len < min_len) {
		return -EBADMSG;
	}
	if (mac_len < STUN_INTEGRITY_SHA256_MAX_LEN) {
		alg = PSA_ALG_TRUNCATED_MAC(alg, mac_len);
	}

	return mac_verify(alg, key, key_len, buf, off,
			  (uint16_t)(off + ATTR_HEADER_SIZE + mac_len - STUN_HEADER_SIZE),
			  buf + off + ATTR_HEADER_SIZE, mac_len);
}

int stun_verify_fingerprint(const uint8_t *buf, size_t len, const struct stun_msg *msg)
{
	size_t off;

	if (!msg_matches(buf, len, msg)) {
		return -EINVAL;
	}
	if (!msg->has_fingerprint) {
		return -ENOENT;
	}
	off = msg->fingerprint_off;
	if (!attr_in_bounds(len, off, FINGERPRINT_LEN)) {
		return -EINVAL;
	}

	return ((crc32_ieee(buf, off) ^ FINGERPRINT_XOR) ==
		sys_get_be32(buf + off + ATTR_HEADER_SIZE))
		       ? 0
		       : -EBADMSG;
}

/* --- Addresses --- */

/* Both directions XOR with the same key (RFC 8489 §14.2): the port with the top
 * half of the magic cookie, an IPv4 address with the cookie, an IPv6 address
 * with the cookie followed by the transaction id — that is, with bytes 4 to 19
 * of the message header, which `hdr` points at the start of.
 */
static void xor_bytes(uint8_t *dst, const uint8_t *src, const uint8_t *hdr, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		dst[i] = src[i] ^ hdr[4U + i];
	}
}

static int xor_address_decode(const uint8_t *hdr, const uint8_t *val, uint16_t alen,
			      struct net_sockaddr *addr)
{
	uint16_t port;

	if (!address_ok(val, alen)) {
		return -EBADMSG;
	}
	port = sys_get_be16(val + 2) ^ (uint16_t)(STUN_MAGIC_COOKIE >> 16);

	if (val[1] == ADDR_FAMILY_IPV4) {
		struct net_sockaddr_in *sin4 = net_sin(addr);

		memset(addr, 0, sizeof(*addr));
		sin4->sin_family = NET_AF_INET;
		sin4->sin_port = net_htons(port);
		xor_bytes(sin4->sin_addr.s4_addr, val + 4, hdr, NET_IPV4_ADDR_SIZE);
		return 0;
	}

#if defined(CONFIG_NET_IPV6)
	struct net_sockaddr_in6 *sin6 = net_sin6(addr);

	memset(addr, 0, sizeof(*addr));
	sin6->sin6_family = NET_AF_INET6;
	sin6->sin6_port = net_htons(port);
	xor_bytes(sin6->sin6_addr.s6_addr, val + 4, hdr, NET_IPV6_ADDR_SIZE);
	return 0;
#else
	return -EAFNOSUPPORT;
#endif
}

int stun_get_xor_address(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			 uint16_t attr_type, struct net_sockaddr *addr)
{
	const uint8_t *val;
	uint16_t alen;
	int rc;

	if (addr == NULL) {
		return -EINVAL;
	}
	rc = stun_find_attr(buf, len, msg, attr_type, &val, &alen);
	if (rc != 0) {
		return rc;
	}
	return xor_address_decode(buf, val, alen, addr);
}

int stun_get_xor_mapped_address(const uint8_t *buf, size_t len, const struct stun_msg *msg,
				struct net_sockaddr *addr)
{
	return stun_get_xor_address(buf, len, msg, STUN_ATTR_XOR_MAPPED_ADDRESS, addr);
}

/* --- Builder --- */

void stun_builder_init(struct stun_builder *b, uint8_t *buf, size_t cap, uint16_t msg_type,
		       const uint8_t txid[STUN_TXID_SIZE])
{
	if (b == NULL) {
		return;
	}
	b->buf = buf;
	b->cap = cap;
	b->len = STUN_HEADER_SIZE;
	b->stage = STAGE_ATTRS;
	b->err = 0;

	if (buf == NULL || txid == NULL) {
		b->err = -EINVAL;
		return;
	}
	if (cap < STUN_HEADER_SIZE) {
		b->err = -EMSGSIZE;
		return;
	}
	sys_put_be16(msg_type, buf);
	sys_put_be16(0U, buf + 2);
	sys_put_be32((uint32_t)STUN_MAGIC_COOKIE, buf + 4);
	memcpy(buf + 8, txid, STUN_TXID_SIZE);
}

/* What every function that adds to a message starts with: a builder, no error
 * remembered, and not past the stage at which this attribute may still come.
 */
static int builder_ready(const struct stun_builder *b, uint8_t max_stage)
{
	if (b == NULL) {
		return -EINVAL;
	}
	if (b->err != 0) {
		return b->err;
	}
	if (b->stage > max_stage) {
		return -EPERM;
	}
	return 0;
}

static bool is_closing_attr(uint16_t type)
{
	return type == STUN_ATTR_MESSAGE_INTEGRITY || type == STUN_ATTR_MESSAGE_INTEGRITY_SHA256 ||
	       type == STUN_ATTR_FINGERPRINT;
}

/* Does an attribute with a value of vlen bytes still fit the buffer and the
 * 16-bit header Length?
 */
static bool attr_fits(const struct stun_builder *b, size_t vlen)
{
	size_t need = ATTR_HEADER_SIZE + padded_len(vlen);

	return b->cap - b->len >= need && b->len + need - STUN_HEADER_SIZE <= UINT16_MAX;
}

/* Make room for one attribute: write its header and zero its padding, count it
 * in the header Length, and return where the value goes. NULL, and -EMSGSIZE
 * remembered, when it does not fit.
 */
static uint8_t *attr_reserve(struct stun_builder *b, uint16_t type, size_t vlen)
{
	uint8_t *at = b->buf + b->len;

	if (!attr_fits(b, vlen)) {
		b->err = -EMSGSIZE;
		return NULL;
	}
	sys_put_be16(type, at);
	sys_put_be16((uint16_t)vlen, at + 2);
	memset(at + ATTR_HEADER_SIZE + vlen, 0, padded_len(vlen) - vlen);
	b->len += ATTR_HEADER_SIZE + padded_len(vlen);
	sys_put_be16((uint16_t)(b->len - STUN_HEADER_SIZE), b->buf + 2);
	return at + ATTR_HEADER_SIZE;
}

static int add_value(struct stun_builder *b, uint16_t type, const uint8_t *val, size_t vlen)
{
	uint8_t *at = attr_reserve(b, type, vlen);

	if (at == NULL) {
		return b->err;
	}
	if (vlen != 0U) {
		memcpy(at, val, vlen);
	}
	return 0;
}

int stun_add_attr(struct stun_builder *b, uint16_t type, const uint8_t *val, size_t vlen)
{
	int rc = builder_ready(b, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	if (vlen > UINT16_MAX || (val == NULL && vlen != 0U) || is_closing_attr(type)) {
		return -EINVAL;
	}
	return add_value(b, type, val, vlen);
}

int stun_add_software(struct stun_builder *b, const char *sw)
{
	int rc = builder_ready(b, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	if (sw == NULL || utf8_chars(sw) > STUN_TEXT_MAX_CHARS) {
		return -EINVAL;
	}
	return add_value(b, STUN_ATTR_SOFTWARE, (const uint8_t *)sw, strlen(sw));
}

int stun_add_username(struct stun_builder *b, const char *username)
{
	int rc = builder_ready(b, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	if (username == NULL || strlen(username) > STUN_USERNAME_MAX_LEN) {
		return -EINVAL;
	}
	return add_value(b, STUN_ATTR_USERNAME, (const uint8_t *)username, strlen(username));
}

int stun_add_xor_address(struct stun_builder *b, uint16_t attr_type,
			 const struct net_sockaddr *addr)
{
	int rc = builder_ready(b, STAGE_ATTRS);
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	if (addr == NULL || is_closing_attr(attr_type)) {
		return -EINVAL;
	}

	if (addr->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *sin4 = net_sin(addr);

		val = attr_reserve(b, attr_type, ADDR_LEN_IPV4);
		if (val == NULL) {
			return b->err;
		}
		val[0] = 0U;
		val[1] = ADDR_FAMILY_IPV4;
		sys_put_be16(net_ntohs(sin4->sin_port) ^ (uint16_t)(STUN_MAGIC_COOKIE >> 16),
			     val + 2);
		xor_bytes(val + 4, sin4->sin_addr.s4_addr, b->buf, NET_IPV4_ADDR_SIZE);
		return 0;
	}

#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *sin6 = net_sin6(addr);

		val = attr_reserve(b, attr_type, ADDR_LEN_IPV6);
		if (val == NULL) {
			return b->err;
		}
		val[0] = 0U;
		val[1] = ADDR_FAMILY_IPV6;
		sys_put_be16(net_ntohs(sin6->sin6_port) ^ (uint16_t)(STUN_MAGIC_COOKIE >> 16),
			     val + 2);
		xor_bytes(val + 4, sin6->sin6_addr.s6_addr, b->buf, NET_IPV6_ADDR_SIZE);
		return 0;
	}
#endif

	return -EAFNOSUPPORT;
}

int stun_add_xor_mapped_address(struct stun_builder *b, const struct net_sockaddr *addr)
{
	return stun_add_xor_address(b, STUN_ATTR_XOR_MAPPED_ADDRESS, addr);
}

int stun_add_priority(struct stun_builder *b, uint32_t priority)
{
	int rc = builder_ready(b, STAGE_ATTRS);
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	val = attr_reserve(b, STUN_ATTR_PRIORITY, sizeof(priority));
	if (val == NULL) {
		return b->err;
	}
	sys_put_be32(priority, val);
	return 0;
}

int stun_add_use_candidate(struct stun_builder *b)
{
	int rc = builder_ready(b, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	return add_value(b, STUN_ATTR_USE_CANDIDATE, NULL, 0U);
}

int stun_add_error_code(struct stun_builder *b, uint16_t code, const char *reason)
{
	int rc = builder_ready(b, STAGE_ATTRS);
	size_t rlen = (reason != NULL) ? strlen(reason) : 0U;
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	if (code < 300U || code > 699U ||
	    (reason != NULL && utf8_chars(reason) > STUN_TEXT_MAX_CHARS)) {
		return -EINVAL;
	}

	/* 2 reserved bytes, the class, the number, the phrase (RFC 8489 §14.8). */
	val = attr_reserve(b, STUN_ATTR_ERROR_CODE, 4U + rlen);
	if (val == NULL) {
		return b->err;
	}
	val[0] = 0U;
	val[1] = 0U;
	val[2] = (uint8_t)(code / 100U);
	val[3] = (uint8_t)(code % 100U);
	if (rlen != 0U) {
		memcpy(val + 4, reason, rlen);
	}
	return 0;
}

int stun_add_unknown_attributes(struct stun_builder *b, const uint16_t *types, size_t n)
{
	int rc = builder_ready(b, STAGE_ATTRS);
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	if ((types == NULL && n != 0U) || n > UINT16_MAX / sizeof(uint16_t)) {
		return -EINVAL;
	}

	val = attr_reserve(b, STUN_ATTR_UNKNOWN_ATTRIBUTES, n * sizeof(uint16_t));
	if (val == NULL) {
		return b->err;
	}
	for (size_t i = 0; i < n; i++) {
		sys_put_be16(types[i], val + i * sizeof(uint16_t));
	}
	return 0;
}

/* Both role attributes carry the 64-bit tie-breaker (RFC 8445 §7.1.3). */
static int add_role(struct stun_builder *b, uint16_t type, uint64_t tiebreaker)
{
	int rc = builder_ready(b, STAGE_ATTRS);
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	val = attr_reserve(b, type, sizeof(tiebreaker));
	if (val == NULL) {
		return b->err;
	}
	sys_put_be64(tiebreaker, val);
	return 0;
}

int stun_add_ice_controlling(struct stun_builder *b, uint64_t tiebreaker)
{
	return add_role(b, STUN_ATTR_ICE_CONTROLLING, tiebreaker);
}

int stun_add_ice_controlled(struct stun_builder *b, uint64_t tiebreaker)
{
	return add_role(b, STUN_ATTR_ICE_CONTROLLED, tiebreaker);
}

/* Sign the message as it stands and append the result as an attribute of
 * `type`. The HMAC is computed before anything is written, so a backend that
 * fails leaves the message as it was.
 */
static int add_mac(struct stun_builder *b, uint16_t type, psa_algorithm_t alg, size_t mac_len,
		   const uint8_t *key, size_t key_len, uint8_t stage)
{
	uint8_t mac[STUN_INTEGRITY_SHA256_MAX_LEN];
	size_t off = b->len;
	int rc;

	if (key == NULL || key_len == 0U) {
		return -EINVAL;
	}
	if (!attr_fits(b, mac_len)) {
		b->err = -EMSGSIZE;
		return b->err;
	}
	rc = mac_sign(alg, key, key_len, b->buf, off,
		      (uint16_t)(off + ATTR_HEADER_SIZE + mac_len - STUN_HEADER_SIZE), mac,
		      mac_len);
	if (rc != 0) {
		b->err = rc;
		return rc;
	}
	memcpy(attr_reserve(b, type, mac_len), mac, mac_len);
	b->stage = stage;
	return 0;
}

int stun_add_integrity(struct stun_builder *b, const uint8_t *key, size_t key_len)
{
	int rc = builder_ready(b, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	return add_mac(b, STUN_ATTR_MESSAGE_INTEGRITY, PSA_ALG_HMAC(PSA_ALG_SHA_1),
		       STUN_INTEGRITY_LEN, key, key_len, STAGE_INTEGRITY);
}

int stun_add_integrity_sha256(struct stun_builder *b, const uint8_t *key, size_t key_len)
{
	int rc = builder_ready(b, STAGE_INTEGRITY);

	if (rc != 0) {
		return rc;
	}
	return add_mac(b, STUN_ATTR_MESSAGE_INTEGRITY_SHA256, PSA_ALG_HMAC(PSA_ALG_SHA_256),
		       STUN_INTEGRITY_SHA256_MAX_LEN, key, key_len, STAGE_INTEGRITY_SHA256);
}

int stun_add_fingerprint(struct stun_builder *b)
{
	int rc = builder_ready(b, STAGE_INTEGRITY_SHA256);
	size_t off;
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	/* The CRC covers the header with a Length that already counts
	 * FINGERPRINT, which is what reserving the attribute leaves there.
	 */
	off = b->len;
	val = attr_reserve(b, STUN_ATTR_FINGERPRINT, FINGERPRINT_LEN);
	if (val == NULL) {
		return b->err;
	}
	sys_put_be32(crc32_ieee(b->buf, off) ^ FINGERPRINT_XOR, val);
	b->stage = STAGE_FINGERPRINT;
	return 0;
}

int stun_builder_len(const struct stun_builder *b)
{
	if (b == NULL) {
		return -EINVAL;
	}
	return (b->err != 0) ? b->err : (int)b->len;
}

int stun_finish(struct stun_builder *b, const uint8_t *key, size_t key_len)
{
	int rc = stun_add_integrity(b, key, key_len);

	if (rc == 0) {
		rc = stun_add_fingerprint(b);
	}
	return (rc == 0) ? stun_builder_len(b) : rc;
}

int stun_finish_plain(struct stun_builder *b)
{
	int rc = stun_add_fingerprint(b);

	return (rc == 0) ? stun_builder_len(b) : rc;
}
