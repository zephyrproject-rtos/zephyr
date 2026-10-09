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
 *    that attribute, not at the end of the message. A message on the wire has
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
#include <zephyr/sys/util_utf8.h>

#include <psa/crypto.h>

/* The log gets what the return codes cannot carry. Why a message is refused is
 * a debug record and never more: -EBADMSG stands for every rule of the parser,
 * and traffic from the network must not be able to fill the log at the levels
 * a product runs with. An error record is for a fault of this system, such as
 * a PSA backend that fails. Neither keys nor integrity values nor the text of
 * attributes (USERNAME, SOFTWARE, REALM, NONCE, a reason phrase) are ever
 * logged. A debug record, and only a debug record, may carry the numbers that
 * broke a rule, such as the class and number of a refused ERROR-CODE, besides
 * types, lengths and offsets.
 */
LOG_MODULE_REGISTER(net_stun, CONFIG_STUN_LOG_LEVEL);

#define FINGERPRINT_XOR 0x5354554eU
#define FINGERPRINT_LEN 4U

/* Fields of the message header (RFC 8489 section 5): the type comes first. */
#define HDR_LENGTH_OFFSET 2U
#define HDR_COOKIE_OFFSET 4U
#define HDR_TXID_OFFSET   8U

/* The two top bits of the message type, which RFC 8489 section 5 keeps zero. */
#define HDR_TYPE_TOP_BITS 0xC000U

/* An attribute is its type, its length, then the value (RFC 8489 section 14). */
#define ATTR_LENGTH_OFFSET 2U
#define ATTR_HEADER_SIZE   4U

/* Types below this are comprehension-required, from it up comprehension-optional
 * (RFC 8489 section 14).
 */
#define ATTR_COMPREHENSION_OPTIONAL_START 0x8000U

/* MAPPED-ADDRESS and XOR-MAPPED-ADDRESS (RFC 8489 sections 14.1, 14.2): a
 * reserved byte, the family, the port, then 4 or 16 bytes of address.
 */
#define ADDR_RESERVED_OFFSET   0U
#define ADDR_FAMILY_OFFSET     1U
#define ADDR_PORT_OFFSET       2U
#define ADDR_IP_OFFSET         4U
#define ADDR_FAMILY_VALUE_IPV4 0x01U
#define ADDR_FAMILY_VALUE_IPV6 0x02U
#define ADDR_LEN_IPV4          (ADDR_IP_OFFSET + NET_IPV4_ADDR_SIZE)
#define ADDR_LEN_IPV6          (ADDR_IP_OFFSET + NET_IPV6_ADDR_SIZE)

/* The port is XORed with the top half of the magic cookie (RFC 8489 section 14.2). */
#define PORT_XOR_KEY ((uint16_t)(STUN_MAGIC_COOKIE >> 16))

/* ERROR-CODE (RFC 8489 section 14.8): two reserved bytes, the class in the low
 * 3 bits of the third, the number in the fourth, then the reason phrase.
 */
#define ERROR_CLASS_OFFSET  2U
#define ERROR_CLASS_MASK    0x07U
#define ERROR_NUMBER_OFFSET 3U
#define ERROR_REASON_OFFSET 4U

/* The class is the hundreds digit of the code (RFC 8489 section 14.8). It runs
 * from 3xx, redirection, to 6xx, global failure; 4xx and 5xx lie in between.
 */
#define ERROR_CLASS_REDIRECTION    3U
#define ERROR_CLASS_GLOBAL_FAILURE 6U

/* The number is the code modulo 100 (RFC 8489 section 14.8). */
#define ERROR_NUMBER_MAX 99U

/* The code is class * 100 + number, so the codes run from 300 to 699. */
#define ERROR_CLASS_FACTOR 100U
#define ERROR_CODE_MIN     (ERROR_CLASS_REDIRECTION * ERROR_CLASS_FACTOR)
#define ERROR_CODE_MAX     (ERROR_CLASS_GLOBAL_FAILURE * ERROR_CLASS_FACTOR + ERROR_NUMBER_MAX)

/* The message type interleaves the 12-bit method with the 2-bit class
 * (RFC 8489 section 5). The method falls into three groups of bits; the two
 * above the lowest are shifted up past the class bits placed between them.
 */
#define METHOD_BITS_3_0        0x000FU
#define METHOD_BITS_6_4        0x0070U
#define METHOD_BITS_11_7       0x0F80U
#define METHOD_BITS_6_4_SHIFT  1U
#define METHOD_BITS_11_7_SHIFT 2U
#define CLASS_BIT_C0           0x1U
#define CLASS_BIT_C1           0x2U
#define CLASS_BIT_C0_SHIFT     4U
#define CLASS_BIT_C1_SHIFT     7U

/* Attribute types the parser checks once per message: a bit each, to tell the
 * first occurrence from the later ones (RFC 8489 section 14). SEEN_NONE is no
 * bit at all, an empty mask.
 */
enum {
	SEEN_NONE = 0U,
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
 * this order, and only in this order (RFC 8489 sections 9, 14.7).
 */
enum {
	STAGE_ATTRS = 0,
	STAGE_INTEGRITY,
	STAGE_INTEGRITY_SHA256,
	STAGE_FINGERPRINT,
};

/* Lengths on the wire are padded to a multiple of 4 bytes (RFC 8489 sections 5, 14). */
static size_t padded_len(size_t vlen)
{
	return ROUND_UP(vlen, 4U);
}

/* Is the caller's array usable: not NULL, or NULL with no elements? */
static bool null_only_if_empty(const void *array, size_t n)
{
	if (array == NULL) {
		if (n != 0U) {
			return false;
		}
	}

	return true;
}

/* Is the string UTF-8 of at most STUN_TEXT_MAX_CHARS characters (RFC 8489
 * sections 14.8, 14.14)?
 */
static bool utf8_within_char_limit(const char *s)
{
	int chars = utf8_count_chars(s);

	if (chars < 0) {
		return false;
	}
	if ((size_t)chars > STUN_TEXT_MAX_CHARS) {
		return false;
	}

	return true;
}

/* --- HMAC through the PSA Crypto API --- */

/**
 * @brief Start an HMAC over a message, for signing or for verifying
 *
 * Initialises the PSA Crypto API, imports the key and feeds the operation what
 * RFC 8489 sections 14.5 and 14.6 define as the HMAC input: the message ahead
 * of offset @p cutoff, with @p patched_len in place of the header Length. The
 * caller's buffer is not modified: the type, the patched Length and the rest go
 * in as three pieces.
 *
 * On success the operation and the key belong to the caller, who finishes or
 * aborts the operation and destroys the key. On failure both are released.
 *
 * @param op Operation, initialised with PSA_MAC_OPERATION_INIT.
 * @param kid Where the id of the imported key is stored.
 * @param alg HMAC algorithm, possibly truncated.
 * @param verify true to verify a value, false to compute one.
 * @param key Key.
 * @param key_len Its length, not 0.
 * @param msg Message; its first @p cutoff bytes are read.
 * @param cutoff Offset of the integrity attribute, at least the header size.
 * @param patched_len Header Length as if the message ended with that attribute.
 *
 * @retval 0 Success.
 * @retval -EIO The PSA backend did not initialise, or refused the key or the operation.
 */
static int hmac_begin(psa_mac_operation_t *op, psa_key_id_t *kid, psa_algorithm_t alg, bool verify,
		      const uint8_t *key, size_t key_len, const uint8_t *msg, size_t cutoff,
		      uint16_t patched_len)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t len_be[sizeof(uint16_t)];
	psa_status_t status;

	/* Repeatable: once the backend is up, the call just succeeds. */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA cannot initialise (%d)", (int)status);
		return -EIO;
	}

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
		status = psa_mac_update(op, msg, HDR_LENGTH_OFFSET);
	}
	if (status == PSA_SUCCESS) {
		status = psa_mac_update(op, len_be, sizeof(len_be));
	}
	if (status == PSA_SUCCESS) {
		status = psa_mac_update(op, msg + HDR_COOKIE_OFFSET, cutoff - HDR_COOKIE_OFFSET);
	}
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA cannot start the HMAC (%d)", (int)status);
		(void)psa_mac_abort(op);
		(void)psa_destroy_key(*kid);
		return -EIO;
	}
	return 0;
}

static int hmac_sign(psa_algorithm_t alg, const uint8_t *key, size_t key_len, const uint8_t *msg,
		     size_t cutoff, uint16_t patched_len, uint8_t *mac, size_t mac_len)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_key_id_t kid = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t written = 0;
	int rc;

	rc = hmac_begin(&op, &kid, alg, false, key, key_len, msg, cutoff, patched_len);
	if (rc != 0) {
		return rc;
	}
	status = psa_mac_sign_finish(&op, mac, mac_len, &written);
	if (status != PSA_SUCCESS) {
		(void)psa_mac_abort(&op);
	}
	(void)psa_destroy_key(kid);
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA cannot compute the HMAC (%d)", (int)status);
		return -EIO;
	}
	if (written != mac_len) {
		LOG_ERR("PSA wrote %zu bytes of HMAC, not %zu", written, mac_len);
		return -EIO;
	}
	return 0;
}

/* The comparison is the backend's, in constant time. */
static int hmac_verify(psa_algorithm_t alg, const uint8_t *key, size_t key_len, const uint8_t *msg,
		       size_t cutoff, uint16_t patched_len, const uint8_t *mac, size_t mac_len)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_key_id_t kid = PSA_KEY_ID_NULL;
	psa_status_t status;
	int rc;

	rc = hmac_begin(&op, &kid, alg, true, key, key_len, msg, cutoff, patched_len);
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
 * (RFC 8489 section 5): two zero bits and the magic cookie.
 */
static bool has_stun_marks(const uint8_t *buf, size_t len)
{
	if (len < STUN_HEADER_SIZE) {
		return false;
	}
	/* A STUN message starts with two zero bits, then the 14-bit message type
	 * (RFC 8489 section 5):
	 *
	 *  0                   1
	 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 * |0 0|     STUN Message Type     |
	 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *
	 * 0xC0 (0b11000000) masks those two bits in the first byte: a byte with either of them
	 * set does not start a STUN message.
	 */
	if ((buf[0] & 0xC0U) != 0U) {
		return false;
	}
	if (sys_get_be32(buf + HDR_COOKIE_OFFSET) != STUN_MAGIC_COOKIE) {
		return false;
	}

	return true;
}

/* Does the header Length account for the whole datagram, in whole 32-bit words
 * (RFC 8489 sections 5, 6.3)? Length is a multiple of 4 and the 20-byte header
 * plus Length is the length of the datagram. The caller has checked that `len`
 * is at least the header size, see has_stun_marks().
 */
static bool length_matches_datagram(const uint8_t *buf, size_t len)
{
	uint16_t length = sys_get_be16(buf + HDR_LENGTH_OFFSET);

	if (padded_len(length) != length) {
		return false;
	}
	if ((size_t)length + STUN_HEADER_SIZE != len) {
		return false;
	}

	return true;
}

bool stun_has_valid_header(const uint8_t *buf, size_t len)
{
	if (buf == NULL) {
		return false;
	}
	if (!has_stun_marks(buf, len)) {
		return false;
	}
	if (!length_matches_datagram(buf, len)) {
		return false;
	}

	return true;
}

/* The bit that marks SOFTWARE, REALM or NONCE as seen, see
 * mark_first_occurrence(). Any other type gets SEEN_NONE: every occurrence of it
 * is then checked, rather than taken for a repeat of another type and skipped.
 */
static uint32_t text_attr_seen_bit(uint16_t attr_type)
{
	switch (attr_type) {
	case STUN_ATTR_SOFTWARE:
		return SEEN_SOFTWARE;
	case STUN_ATTR_REALM:
		return SEEN_REALM;
	case STUN_ATTR_NONCE:
		return SEEN_NONCE;
	default:
		return SEEN_NONE;
	}
}

/**
 * @brief Tell the first occurrence of an attribute type from the later ones
 *
 * Marks the type as seen, so that only the first call for a type returns true
 * (RFC 8489 section 14).
 *
 * @param seen Types seen so far in the message, one bit each.
 * @param bit The bit of the type, one of the SEEN_ values. SEEN_NONE marks
 *            nothing, so every call with it returns true.
 *
 * @retval true The type had not been seen; it is now.
 * @retval false The type has been seen before.
 */
static bool mark_first_occurrence(uint32_t *seen, uint32_t bit)
{
	if ((*seen & bit) != 0U) {
		return false;
	}
	*seen |= bit;
	return true;
}

/* Check the value of an address attribute (RFC 8489 section 14.1): 0 if it is
 * an IPv4 or IPv6 address of the right length, -EBADMSG if it is too short to
 * hold a family or the length does not fit the family, -EAFNOSUPPORT if the
 * family is neither IPv4 nor IPv6; such an attribute is ignored, not malformed
 * (section 6.3.3).
 */
static int address_check(const uint8_t *val, uint16_t addr_len)
{
	if (addr_len < ADDR_IP_OFFSET) {
		return -EBADMSG;
	}

	switch (val[ADDR_FAMILY_OFFSET]) {
	case ADDR_FAMILY_VALUE_IPV4:
		if (addr_len != ADDR_LEN_IPV4) {
			return -EBADMSG;
		}
		return 0;
	case ADDR_FAMILY_VALUE_IPV6:
		if (addr_len != ADDR_LEN_IPV6) {
			return -EBADMSG;
		}
		return 0;
	default:
		return -EAFNOSUPPORT;
	}
}

/* Class 3 to 6 and number 0 to 99: the codes 300 to 699 (RFC 8489 section 14.8). */
static bool error_code_in_range(uint8_t error_class, uint8_t number)
{
	if (error_class < ERROR_CLASS_REDIRECTION) {
		return false;
	}
	if (error_class > ERROR_CLASS_GLOBAL_FAILURE) {
		return false;
	}
	if (number > ERROR_NUMBER_MAX) {
		return false;
	}

	return true;
}

/* Can MESSAGE-INTEGRITY-SHA256 be `len` bytes: 16 to 32, a multiple of 4
 * (RFC 8489 section 14.6)?
 */
static bool integrity_sha256_len_allowed(size_t len)
{
	if (len < STUN_INTEGRITY_SHA256_MIN_LEN) {
		return false;
	}
	if (len > STUN_INTEGRITY_SHA256_MAX_LEN) {
		return false;
	}
	if (padded_len(len) != len) {
		return false;
	}

	return true;
}

/* Note where MESSAGE-INTEGRITY-SHA256 is and how long its value is. */
static void record_integrity_sha256(struct stun_msg *out, size_t off, uint16_t alen)
{
	out->has_integrity_sha256 = true;
	out->integrity_sha256_off = off;
	out->integrity_sha256_len = alen;
}

/* Has the parser not yet reached MESSAGE-INTEGRITY or MESSAGE-INTEGRITY-SHA256? */
static bool before_integrity_boundary(const struct stun_msg *msg)
{
	if (msg->has_integrity) {
		return false;
	}
	if (msg->has_integrity_sha256) {
		return false;
	}

	return true;
}

/* Does an attribute past the integrity boundary still count?
 *
 * Attributes after MESSAGE-INTEGRITY are outside its HMAC: anyone on the path
 * can append them without breaking the signature, so agents ignore them
 * (RFC 8489 section 9). There are two exceptions. FINGERPRINT, which the caller
 * handles itself, and MESSAGE-INTEGRITY-SHA256, whose own HMAC covers
 * MESSAGE-INTEGRITY too, so that a message can carry both signatures. This
 * function picks out the latter: the first MESSAGE-INTEGRITY-SHA256 that
 * follows MESSAGE-INTEGRITY. Without MESSAGE-INTEGRITY the boundary is
 * MESSAGE-INTEGRITY-SHA256 itself, and nothing past it counts but FINGERPRINT.
 */
static bool counts_past_integrity_boundary(const struct stun_msg *msg, uint16_t atype)
{
	if (atype != STUN_ATTR_MESSAGE_INTEGRITY_SHA256) {
		return false;
	}
	if (!msg->has_integrity) {
		return false;
	}
	if (msg->has_integrity_sha256) {
		return false;
	}

	return true;
}

/**
 * @brief Check and record one attribute ahead of the integrity boundary
 *
 * Of each type only the first occurrence is looked at, and that one has to be
 * well-formed: the length and the values RFC 8489 fixes for it. What the
 * caller needs later goes into @p out; types the codec does not understand
 * are skipped.
 *
 * @param buf Message.
 * @param attr_offset Offset of the attribute header; the caller has checked that
 *                    the value and its padding lie inside the message.
 * @param attr_type Attribute type.
 * @param attr_len Length of the value.
 * @param out Parsed message, filled in as attributes are met.
 * @param seen_types Types seen so far, see mark_first_occurrence().
 *
 * @retval 0 The attribute is well-formed, a later occurrence, or unknown.
 * @retval -EBADMSG The attribute is malformed, or a second ICE role.
 */
static int parse_attr(const uint8_t *buf, size_t attr_offset, uint16_t attr_type, uint16_t attr_len,
		      struct stun_msg *out, uint32_t *seen_types)
{
	const uint8_t *attr_value = buf + attr_offset + ATTR_HEADER_SIZE;

	switch (attr_type) {
	case STUN_ATTR_MESSAGE_INTEGRITY:
		if (attr_len != STUN_INTEGRITY_LEN) {
			LOG_DBG("not parsed: MESSAGE-INTEGRITY at offset %zu is %u bytes, not %u",
				attr_offset, attr_len, STUN_INTEGRITY_LEN);
			return -EBADMSG;
		}
		out->has_integrity = true;
		out->integrity_off = attr_offset;
		out->attrs_end = attr_offset;
		break;
	case STUN_ATTR_MESSAGE_INTEGRITY_SHA256:
		if (!integrity_sha256_len_allowed(attr_len)) {
			LOG_DBG("not parsed: MESSAGE-INTEGRITY-SHA256 at offset %zu is %u bytes",
				attr_offset, attr_len);
			return -EBADMSG;
		}
		record_integrity_sha256(out, attr_offset, attr_len);
		out->attrs_end = attr_offset;
		break;
	case STUN_ATTR_USERNAME:
		if (!mark_first_occurrence(seen_types, SEEN_USERNAME)) {
			break;
		}
		if (attr_len > STUN_TEXT_MAX_LEN) {
			LOG_DBG("not parsed: USERNAME at offset %zu is %u bytes, over %u",
				attr_offset, attr_len, STUN_TEXT_MAX_LEN);
			return -EBADMSG;
		}
		out->username = attr_value;
		out->username_len = attr_len;
		break;
	case STUN_ATTR_SOFTWARE:
	case STUN_ATTR_REALM:
	case STUN_ATTR_NONCE: {
		uint32_t bit = text_attr_seen_bit(attr_type);

		if (!mark_first_occurrence(seen_types, bit)) {
			break;
		}
		if (attr_len > STUN_TEXT_MAX_LEN) {
			LOG_DBG("not parsed: attribute 0x%04x at offset %zu is %u bytes, over %u",
				attr_type, attr_offset, attr_len, STUN_TEXT_MAX_LEN);
			return -EBADMSG;
		}
		break;
	}
	case STUN_ATTR_PRIORITY:
		if (!mark_first_occurrence(seen_types, SEEN_PRIORITY)) {
			break;
		}
		if (attr_len != sizeof(uint32_t)) {
			LOG_DBG("not parsed: PRIORITY at offset %zu is %u bytes, not 4",
				attr_offset, attr_len);
			return -EBADMSG;
		}
		out->has_priority = true;
		out->priority = sys_get_be32(attr_value);
		break;
	case STUN_ATTR_USE_CANDIDATE:
		if (!mark_first_occurrence(seen_types, SEEN_USE_CANDIDATE)) {
			break;
		}
		if (attr_len != 0U) {
			LOG_DBG("not parsed: USE-CANDIDATE at offset %zu is %u bytes, not 0",
				attr_offset, attr_len);
			return -EBADMSG;
		}
		out->has_use_candidate = true;
		break;
	case STUN_ATTR_ERROR_CODE: {
		/* The reserved bytes are ignored. */
		uint8_t error_class, number;

		if (!mark_first_occurrence(seen_types, SEEN_ERROR_CODE)) {
			break;
		}
		/* The value holds at least the 2 reserved bytes, the class and the number. */
		if (attr_len < ERROR_REASON_OFFSET) {
			LOG_DBG("not parsed: ERROR-CODE at offset %zu is %u bytes, under 4",
				attr_offset, attr_len);
			return -EBADMSG;
		}
		/* The reason phrase after them is at most 763 bytes (RFC 8489 section 14.8). */
		if (attr_len - ERROR_REASON_OFFSET > STUN_TEXT_MAX_LEN) {
			LOG_DBG("not parsed: ERROR-CODE at offset %zu is %u bytes, over %u",
				attr_offset, attr_len, ERROR_REASON_OFFSET + STUN_TEXT_MAX_LEN);
			return -EBADMSG;
		}
		error_class = attr_value[ERROR_CLASS_OFFSET] & ERROR_CLASS_MASK;
		number = attr_value[ERROR_NUMBER_OFFSET];
		if (!error_code_in_range(error_class, number)) {
			LOG_DBG("not parsed: ERROR-CODE at offset %zu has class %u, number %u",
				attr_offset, error_class, number);
			return -EBADMSG;
		}
		out->has_error_code = true;
		out->error_code = (uint16_t)(error_class * ERROR_CLASS_FACTOR + number);
		break;
	}
	case STUN_ATTR_ICE_CONTROLLING:
	case STUN_ATTR_ICE_CONTROLLED: {
		bool is_controlling = (attr_type == STUN_ATTR_ICE_CONTROLLING);
		uint32_t bit = is_controlling ? SEEN_ICE_CONTROLLING : SEEN_ICE_CONTROLLED;

		if (!mark_first_occurrence(seen_types, bit)) {
			break;
		}
		if (attr_len != sizeof(uint64_t)) {
			LOG_DBG("not parsed: ICE role 0x%04x at offset %zu is %u bytes, not 8",
				attr_type, attr_offset, attr_len);
			return -EBADMSG;
		}
		/* An agent has one role (RFC 8445 section 7.1.3). */
		if (out->has_ice_controlling) {
			LOG_DBG("not parsed: a second ICE role at offset %zu", attr_offset);
			return -EBADMSG;
		}
		if (out->has_ice_controlled) {
			LOG_DBG("not parsed: a second ICE role at offset %zu", attr_offset);
			return -EBADMSG;
		}
		out->has_ice_controlling = is_controlling;
		out->has_ice_controlled = !is_controlling;
		out->ice_tiebreaker = sys_get_be64(attr_value);
		break;
	}
	case STUN_ATTR_MAPPED_ADDRESS:
	case STUN_ATTR_XOR_MAPPED_ADDRESS: {
		bool is_mapped_address = (attr_type == STUN_ATTR_MAPPED_ADDRESS);
		uint32_t bit = is_mapped_address ? SEEN_MAPPED_ADDRESS : SEEN_XOR_MAPPED_ADDRESS;

		if (!mark_first_occurrence(seen_types, bit)) {
			break;
		}
		/* An unsupported family is ignored (RFC 8489 section 6.3.3);
		 * the getter reports it.
		 */
		if (address_check(attr_value, attr_len) == -EBADMSG) {
			LOG_DBG("not parsed: address 0x%04x at offset %zu is %u bytes, malformed",
				attr_type, attr_offset, attr_len);
			return -EBADMSG;
		}
		break;
	}
	case STUN_ATTR_UNKNOWN_ATTRIBUTES:
		/* A list of 16-bit attribute types (RFC 8489 section 14.13). */
		if (!mark_first_occurrence(seen_types, SEEN_UNKNOWN_ATTRIBUTES)) {
			break;
		}
		if ((attr_len % sizeof(uint16_t)) != 0U) {
			LOG_DBG("not parsed: UNKNOWN-ATTRIBUTES at offset %zu is %u bytes, odd",
				attr_offset, attr_len);
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
	size_t attr_offset = STUN_HEADER_SIZE;
	uint32_t seen_types = 0U;

	if (buf == NULL) {
		return -EINVAL;
	}
	if (out == NULL) {
		return -EINVAL;
	}
	if (!has_stun_marks(buf, len)) {
		return -EINVAL;
	}
	if (!length_matches_datagram(buf, len)) {
		LOG_DBG("length %u does not make the %zu bytes of the datagram",
			sys_get_be16(buf + HDR_LENGTH_OFFSET), len);
		return -EBADMSG;
	}

	memset(out, 0, sizeof(*out));
	out->type = sys_get_be16(buf);
	out->length = sys_get_be16(buf + HDR_LENGTH_OFFSET);
	memcpy(out->txid, buf + HDR_TXID_OFFSET, STUN_TXID_SIZE);
	out->attrs_end = len;

	/* Only the first occurrence of an attribute type counts, and three things
	 * see to that. Ahead of the integrity boundary, parse_attr() marks each type
	 * it has processed in seen_types and skips later ones (RFC 8489 section 14).
	 * The first MESSAGE-INTEGRITY or MESSAGE-INTEGRITY-SHA256 is the boundary
	 * itself, and everything past it is ignored, repeats included (section 9).
	 * counts_past_integrity_boundary() picks out the one exception, the first
	 * MESSAGE-INTEGRITY-SHA256 after MESSAGE-INTEGRITY. FINGERPRINT is apart:
	 * it is the last attribute, so anything after it, even a second FINGERPRINT,
	 * makes the message malformed (section 14.7).
	 *
	 * attr_offset and len are multiples of 4, so while attr_offset < len there
	 * is room for an attribute header.
	 */
	while (attr_offset < len) {
		uint16_t attr_type = sys_get_be16(buf + attr_offset);
		uint16_t attr_len = sys_get_be16(buf + attr_offset + ATTR_LENGTH_OFFSET);
		int rc = 0;

		if (padded_len(attr_len) > len - attr_offset - ATTR_HEADER_SIZE) {
			LOG_DBG("attribute 0x%04x at offset %zu: its %u bytes run past the end",
				attr_type, attr_offset, attr_len);
			return -EBADMSG;
		}
		/* FINGERPRINT is the last attribute (RFC 8489 section 14.7). */
		if (out->has_fingerprint) {
			LOG_DBG("attribute 0x%04x at offset %zu follows FINGERPRINT", attr_type,
				attr_offset);
			return -EBADMSG;
		}

		if (attr_type == STUN_ATTR_FINGERPRINT) {
			if (attr_len != FINGERPRINT_LEN) {
				LOG_DBG("FINGERPRINT of %u bytes at offset %zu", attr_len,
					attr_offset);
				return -EBADMSG;
			}
			out->has_fingerprint = true;
			out->fingerprint_off = attr_offset;
		} else if (before_integrity_boundary(out)) {
			rc = parse_attr(buf, attr_offset, attr_type, attr_len, out, &seen_types);
		} else if (counts_past_integrity_boundary(out, attr_type)) {
			if (!integrity_sha256_len_allowed(attr_len)) {
				LOG_DBG("MESSAGE-INTEGRITY-SHA256 of %u bytes at offset %zu",
					attr_len, attr_offset);
				return -EBADMSG;
			}
			record_integrity_sha256(out, attr_offset, attr_len);
		} else {
			/* Past the integrity boundary: ignored, unexamined. */
		}
		if (rc != 0) {
			return rc;
		}
		attr_offset += ATTR_HEADER_SIZE + padded_len(attr_len);
	}

	return 0;
}

/*
 * Build the 14-bit message type from a method and a class (RFC 8489 section 5).
 * The bits of the two are not side by side: C0, the low bit of the class, sits
 * between M4 and M3, and C1 between M7 and M6:
 *
 *  0                 1
 *  2  3  4 5 6 7 8 9 0 1 2 3 4 5
 * +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M |M |M|M|M|C|M|M|M|C|M|M|M|M|
 * |11|10|9|8|7|1|6|5|4|0|3|2|1|0|
 * +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * M3..M0 stay where they are, M6..M4 move up one bit to make room for C0, and
 * M11..M7 move up two to make room for C0 and C1.
 */
uint16_t stun_msg_type_compose(uint16_t method, enum stun_class msg_class)
{
	uint16_t class_bits = (uint16_t)msg_class;
	uint16_t type = 0U;

	type |= method & METHOD_BITS_3_0;
	type |= (method & METHOD_BITS_6_4) << METHOD_BITS_6_4_SHIFT;
	type |= (method & METHOD_BITS_11_7) << METHOD_BITS_11_7_SHIFT;
	type |= (class_bits & CLASS_BIT_C0) << CLASS_BIT_C0_SHIFT;
	type |= (class_bits & CLASS_BIT_C1) << CLASS_BIT_C1_SHIFT;

	return type;
}

/* The method of a message type: the reverse of stun_msg_type_compose(), each
 * group of method bits shifted back down over the class bits.
 */
uint16_t stun_msg_type_get_method(uint16_t type)
{
	uint16_t method = 0U;

	method |= type & METHOD_BITS_3_0;
	method |= (type >> METHOD_BITS_6_4_SHIFT) & METHOD_BITS_6_4;
	method |= (type >> METHOD_BITS_11_7_SHIFT) & METHOD_BITS_11_7;

	return method;
}

/* The class of a message type: C0 and C1 picked out from between the method
 * bits, see stun_msg_type_compose(). Read as a number they give the class
 * (RFC 8489 section 5):
 *
 *   C1 C0   class_bits   class
 *    0  0        0       request
 *    0  1        1       indication
 *    1  0        2       success response
 *    1  1        3       error response
 */
enum stun_class stun_msg_type_get_class(uint16_t type)
{
	uint16_t class_bits = 0U;

	class_bits |= (type >> CLASS_BIT_C0_SHIFT) & CLASS_BIT_C0;
	class_bits |= (type >> CLASS_BIT_C1_SHIFT) & CLASS_BIT_C1;

	/* A switch, not a cast to the enum: MISRA C:2012 Rule 10.5. */
	switch (class_bits) {
	case 0U:
		return STUN_CLASS_REQUEST;
	case 1U:
		return STUN_CLASS_INDICATION;
	case 2U:
		return STUN_CLASS_SUCCESS;
	default:
		return STUN_CLASS_ERROR;
	}
}

/* Could `msg` be what stun_parse() made of `buf`? The functions that take both
 * read `buf` at the offsets `msg` holds, so a `msg` of another datagram or a
 * damaged one would send them past the end of the buffer. Only what the length
 * and the offsets imply is checked: the contents are not compared.
 */
static bool msg_belongs_to_buffer(const uint8_t *buf, size_t len, const struct stun_msg *msg)
{
	if (buf == NULL) {
		return false;
	}
	if (msg == NULL) {
		return false;
	}
	/* The parsed Length is a multiple of 4, as stun_parse() accepts it. */
	if (padded_len(msg->length) != msg->length) {
		return false;
	}
	/* The header and the parsed Length make exactly this buffer. */
	if (len != STUN_HEADER_SIZE + (size_t)msg->length) {
		return false;
	}
	/* The integrity boundary is not inside the header... */
	if (msg->attrs_end < STUN_HEADER_SIZE) {
		return false;
	}
	/* ...nor past the end of the buffer. */
	if (msg->attrs_end > len) {
		return false;
	}

	return true;
}

/* Do the header and `vlen` bytes of value of the attribute at `off` lie inside
 * the attributes of the `len` bytes of the buffer, past the message header, so
 * that reading them is safe? An offset taken from a damaged stun_msg can point
 * anywhere.
 */
static bool attr_within_buffer(size_t len, size_t off, size_t vlen)
{
	/* No attribute starts inside the message header. */
	if (off < STUN_HEADER_SIZE) {
		return false;
	}
	if (off > len) {
		return false;
	}
	if (len - off < ATTR_HEADER_SIZE) {
		return false;
	}
	if (len - off - ATTR_HEADER_SIZE < vlen) {
		return false;
	}

	return true;
}

int stun_find_attr(const uint8_t *buf, size_t len, const struct stun_msg *msg,
		   uint16_t attr_type, const uint8_t **value, uint16_t *value_len)
{
	size_t attr_offset;
	uint16_t attr_len = 0U;

	if (value == NULL) {
		return -EINVAL;
	}
	if (value_len == NULL) {
		return -EINVAL;
	}
	if (!msg_belongs_to_buffer(buf, len, msg)) {
		return -EINVAL;
	}

	/* Integrity attributes lie at or past the boundary the search stops at
	 * (RFC 8489 section 9).
	 */
	switch (attr_type) {
	case STUN_ATTR_MESSAGE_INTEGRITY:
		if (!msg->has_integrity) {
			return -ENOENT;
		}
		attr_offset = msg->integrity_off;
		break;
	case STUN_ATTR_MESSAGE_INTEGRITY_SHA256:
		if (!msg->has_integrity_sha256) {
			return -ENOENT;
		}
		attr_offset = msg->integrity_sha256_off;
		break;
	case STUN_ATTR_FINGERPRINT:
		if (!msg->has_fingerprint) {
			return -ENOENT;
		}
		attr_offset = msg->fingerprint_off;
		break;
	default:
		attr_offset = STUN_HEADER_SIZE;
		while (attr_offset < msg->attrs_end) {
			if (sys_get_be16(buf + attr_offset) == attr_type) {
				break;
			}
			attr_offset += ATTR_HEADER_SIZE +
				padded_len(sys_get_be16(buf + attr_offset + ATTR_LENGTH_OFFSET));
		}
		if (attr_offset >= msg->attrs_end) {
			return -ENOENT;
		}
		break;
	}

	if (!attr_within_buffer(len, attr_offset, 0U)) {
		return -EINVAL;
	}
	attr_len = sys_get_be16(buf + attr_offset + ATTR_LENGTH_OFFSET);
	if (!attr_within_buffer(len, attr_offset, padded_len(attr_len))) {
		return -EINVAL;
	}
	*value = buf + attr_offset + ATTR_HEADER_SIZE;
	*value_len = attr_len;
	return 0;
}

/* Is the type one of the comprehension-required attributes (below 0x8000,
 * RFC 8489 section 14) this codec knows the meaning of? Those of RFC 8489 but
 * for the long-term credential ones, PASSWORD-ALGORITHM and USERHASH, and those
 * ICE adds. An agent that does not know a required attribute cannot process the
 * message, so stun_unknown_attrs() reports any other one the caller does not
 * know either.
 */
static bool codec_knows_required_attr(uint16_t attr_type)
{
	switch (attr_type) {
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

/* Is `attr_type` among the `n_types` attribute types in `types`? */
static bool attr_type_in_list(const uint16_t *types, size_t n_types, uint16_t attr_type)
{
	for (size_t i = 0; i < n_types; i++) {
		if (types[i] == attr_type) {
			return true;
		}
	}
	return false;
}

/* Does `attr_type` occur among the attributes ahead of offset `end`? */
static bool type_occurs_before(const uint8_t *buf, size_t end, uint16_t attr_type)
{
	size_t attr_offset = STUN_HEADER_SIZE;

	while (attr_offset < end) {
		if (sys_get_be16(buf + attr_offset) == attr_type) {
			return true;
		}
		attr_offset += ATTR_HEADER_SIZE +
			       padded_len(sys_get_be16(buf + attr_offset + ATTR_LENGTH_OFFSET));
	}
	return false;
}

/* Is the attribute at `attr_offset` one to report: comprehension-required
 * (below 0x8000, RFC 8489 section 14), understood neither by the codec nor by
 * the caller, and the first of its type?
 *
 * If true, the message holds an attribute the sender requires to be understood
 * and we cannot understand it, so the message cannot be processed. A request is
 * answered with 420 (Unknown Attribute), any other message is discarded
 * (RFC 8489 section 6.3).
 */
static bool first_unknown_required_attr(const uint8_t *buf, size_t attr_offset, uint16_t attr_type,
					const uint16_t *known, size_t n_known)
{
	if (attr_type >= ATTR_COMPREHENSION_OPTIONAL_START) {
		return false;
	}
	if (codec_knows_required_attr(attr_type)) {
		return false;
	}
	if (attr_type_in_list(known, n_known, attr_type)) {
		return false;
	}
	if (type_occurs_before(buf, attr_offset, attr_type)) {
		return false;
	}

	return true;
}

int stun_unknown_attrs(const uint8_t *buf, size_t len, const struct stun_msg *msg,
		       const uint16_t *known, size_t n_known, uint16_t *out, size_t cap)
{
	size_t attr_offset = STUN_HEADER_SIZE;
	size_t count = 0;

	if (!msg_belongs_to_buffer(buf, len, msg)) {
		return -EINVAL;
	}
	if (!null_only_if_empty(known, n_known)) {
		return -EINVAL;
	}
	if (!null_only_if_empty(out, cap)) {
		return -EINVAL;
	}

	while (attr_offset < msg->attrs_end) {
		uint16_t attr_type = sys_get_be16(buf + attr_offset);

		if (first_unknown_required_attr(buf, attr_offset, attr_type, known, n_known)) {
			if (count < cap) {
				out[count] = attr_type;
			}
			count++;
		}
		attr_offset += ATTR_HEADER_SIZE +
			       padded_len(sys_get_be16(buf + attr_offset + ATTR_LENGTH_OFFSET));
	}
	return (int)count;
}

int stun_verify_integrity(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			  const uint8_t *key, size_t key_len)
{
	size_t attr_offset = 0U;
	uint16_t patched_len = 0U;

	if (key == NULL) {
		return -EINVAL;
	}
	if (key_len == 0U) {
		return -EINVAL;
	}
	if (!msg_belongs_to_buffer(buf, len, msg)) {
		return -EINVAL;
	}
	if (!msg->has_integrity) {
		return -ENOENT;
	}
	attr_offset = msg->integrity_off;
	if (!attr_within_buffer(len, attr_offset, STUN_INTEGRITY_LEN)) {
		return -EINVAL;
	}

	/* The header Length as if the message ended with MESSAGE-INTEGRITY: that is
	 * what the HMAC covers (RFC 8489 section 14.5).
	 */
	patched_len = (uint16_t)(attr_offset + ATTR_HEADER_SIZE + STUN_INTEGRITY_LEN -
				 STUN_HEADER_SIZE);

	return hmac_verify(PSA_ALG_HMAC(PSA_ALG_SHA_1), key, key_len, buf, attr_offset,
			   patched_len, buf + attr_offset + ATTR_HEADER_SIZE, STUN_INTEGRITY_LEN);
}

int stun_verify_integrity_sha256(const uint8_t *buf, size_t len, const struct stun_msg *msg,
				 const uint8_t *key, size_t key_len, size_t min_len)
{
	psa_algorithm_t alg = PSA_ALG_HMAC(PSA_ALG_SHA_256);
	size_t attr_offset = 0U;
	size_t mac_len = 0U;
	uint16_t patched_len = 0U;

	if (key == NULL) {
		return -EINVAL;
	}
	if (key_len == 0U) {
		return -EINVAL;
	}
	if (!integrity_sha256_len_allowed(min_len)) {
		return -EINVAL;
	}
	if (!msg_belongs_to_buffer(buf, len, msg)) {
		return -EINVAL;
	}
	if (!msg->has_integrity_sha256) {
		return -ENOENT;
	}
	attr_offset = msg->integrity_sha256_off;
	mac_len = msg->integrity_sha256_len;
	if (!integrity_sha256_len_allowed(mac_len)) {
		return -EINVAL;
	}
	if (!attr_within_buffer(len, attr_offset, mac_len)) {
		return -EINVAL;
	}
	/* Truncated further than the caller's STUN Usage allows (RFC 8489 section 14.6). */
	if (mac_len < min_len) {
		return -EBADMSG;
	}
	if (mac_len < STUN_INTEGRITY_SHA256_MAX_LEN) {
		alg = PSA_ALG_TRUNCATED_MAC(alg, mac_len);
	}

	/* The header Length as if the message ended with MESSAGE-INTEGRITY-SHA256:
	 * that is what the HMAC covers (RFC 8489 section 14.6).
	 */
	patched_len = (uint16_t)(attr_offset + ATTR_HEADER_SIZE + mac_len - STUN_HEADER_SIZE);

	return hmac_verify(alg, key, key_len, buf, attr_offset, patched_len,
			   buf + attr_offset + ATTR_HEADER_SIZE, mac_len);
}

int stun_verify_fingerprint(const uint8_t *buf, size_t len, const struct stun_msg *msg)
{
	size_t attr_offset = 0U;
	uint32_t expected = 0U;
	uint32_t received = 0U;

	if (!msg_belongs_to_buffer(buf, len, msg)) {
		return -EINVAL;
	}
	if (!msg->has_fingerprint) {
		return -ENOENT;
	}
	attr_offset = msg->fingerprint_off;
	if (!attr_within_buffer(len, attr_offset, FINGERPRINT_LEN)) {
		return -EINVAL;
	}

	/* CRC-32 of the message ahead of FINGERPRINT, XORed with "STUN"
	 * (RFC 8489 section 14.7). FINGERPRINT is the last attribute, so the
	 * header Length already counts it, as the CRC requires.
	 */
	expected = crc32_ieee(buf, attr_offset) ^ FINGERPRINT_XOR;
	received = sys_get_be32(buf + attr_offset + ATTR_HEADER_SIZE);
	if (expected != received) {
		return -EBADMSG;
	}

	return 0;
}

/* --- Addresses --- */

/* Both directions XOR with the same key (RFC 8489 section 14.2): the port with
 * the top half of the magic cookie, an IPv4 address with the cookie, an IPv6
 * address with the cookie followed by the transaction id. Together these are
 * bytes 4 to 19 of the message header, which `hdr` points at the start of.
 */
static void xor_with_cookie_and_txid(uint8_t *dst, const uint8_t *src, const uint8_t *hdr, size_t n)
{
	mem_xor_n(dst, src, hdr + HDR_COOKIE_OFFSET, n);
}

/**
 * @brief Decode an attribute in the XOR-MAPPED-ADDRESS format
 *
 * @param hdr Header of the message the attribute belongs to: the key of the
 *            XOR is in it (RFC 8489 section 14.2).
 * @param attr_value Value of the attribute.
 * @param attr_len Its length.
 * @param addr Address and port, not NULL; written only on success.
 *
 * @retval 0 Success.
 * @retval -EBADMSG The value is too short for a family, or the length does not
 *                  match the family.
 * @retval -EAFNOSUPPORT A family other than IPv4 and IPv6, or an IPv6 address
 *                       and @kconfig{CONFIG_NET_IPV6} is not enabled.
 */
static int xor_address_decode(const uint8_t *hdr, const uint8_t *attr_value, uint16_t attr_len,
			      struct net_sockaddr *addr)
{
	int rc = address_check(attr_value, attr_len);
	uint16_t port = 0U;

	if (rc != 0) {
		return rc;
	}
	/* The port was XORed with the top half of the magic cookie (RFC 8489 section 14.2). */
	port = sys_get_be16(attr_value + ADDR_PORT_OFFSET) ^ PORT_XOR_KEY;

	if (attr_value[ADDR_FAMILY_OFFSET] == ADDR_FAMILY_VALUE_IPV4) {
		struct net_sockaddr_in *sin4 = net_sin(addr);

		memset(addr, 0, sizeof(*addr));
		sin4->sin_family = NET_AF_INET;
		sin4->sin_port = net_htons(port);
		xor_with_cookie_and_txid(sin4->sin_addr.s4_addr, attr_value + ADDR_IP_OFFSET, hdr,
					 NET_IPV4_ADDR_SIZE);
		return 0;
	}

	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		return -EAFNOSUPPORT;
	}

	if (attr_value[ADDR_FAMILY_OFFSET] == ADDR_FAMILY_VALUE_IPV6) {
		struct net_sockaddr_in6 *sin6 = net_sin6(addr);

		memset(addr, 0, sizeof(*addr));
		sin6->sin6_family = NET_AF_INET6;
		sin6->sin6_port = net_htons(port);
		xor_with_cookie_and_txid(sin6->sin6_addr.s6_addr, attr_value + ADDR_IP_OFFSET,
					 hdr, NET_IPV6_ADDR_SIZE);
		return 0;
	}

	/* address_check() lets only IPv4 and IPv6 through. */
	return -EAFNOSUPPORT;
}

int stun_get_xor_address(const uint8_t *buf, size_t len, const struct stun_msg *msg,
			 uint16_t attr_type, struct net_sockaddr *addr)
{
	const uint8_t *attr_value = NULL;
	uint16_t attr_len = 0U;
	int rc = 0;

	if (addr == NULL) {
		return -EINVAL;
	}
	rc = stun_find_attr(buf, len, msg, attr_type, &attr_value, &attr_len);
	if (rc != 0) {
		return rc;
	}
	return xor_address_decode(buf, attr_value, attr_len, addr);
}

int stun_get_xor_mapped_address(const uint8_t *buf, size_t len, const struct stun_msg *msg,
				struct net_sockaddr *addr)
{
	return stun_get_xor_address(buf, len, msg, STUN_ATTR_XOR_MAPPED_ADDRESS, addr);
}

/* --- Builder --- */

void stun_builder_init(struct stun_builder *builder, uint8_t *buf, size_t buf_size,
		       uint16_t msg_type, const uint8_t transaction_id[STUN_TXID_SIZE])
{
	if (builder == NULL) {
		return;
	}
	/* A refused argument is remembered in err alone; the other fields are only
	 * set from arguments that passed the checks, and are read only while err is 0.
	 */
	if (buf == NULL) {
		builder->err = -EINVAL;
		return;
	}
	if (transaction_id == NULL) {
		builder->err = -EINVAL;
		return;
	}
	if ((msg_type & HDR_TYPE_TOP_BITS) != 0U) {
		builder->err = -EINVAL;
		return;
	}
	if (buf_size < STUN_HEADER_SIZE) {
		builder->err = -EMSGSIZE;
		return;
	}

	builder->buf = buf;
	builder->buf_size = buf_size;
	builder->msg_len = STUN_HEADER_SIZE;
	builder->stage = STAGE_ATTRS;
	builder->err = 0;

	sys_put_be16(msg_type, buf);
	sys_put_be16(0U, buf + HDR_LENGTH_OFFSET);
	sys_put_be32((uint32_t)STUN_MAGIC_COOKIE, buf + HDR_COOKIE_OFFSET);
	memcpy(buf + HDR_TXID_OFFSET, transaction_id, STUN_TXID_SIZE);
}

/* What every function that adds to a message starts with: a builder, no error
 * remembered, and not past the stage at which this attribute may still come.
 */
static int builder_ready(const struct stun_builder *builder, uint8_t max_stage)
{
	if (builder == NULL) {
		return -EINVAL;
	}
	if (builder->err != 0) {
		return builder->err;
	}
	if (builder->stage > max_stage) {
		return -EPERM;
	}
	return 0;
}

/* MESSAGE-INTEGRITY, MESSAGE-INTEGRITY-SHA256 and FINGERPRINT close a message:
 * each holds a value computed over what precedes it, so only their own
 * functions add them, in their order.
 */
static bool is_closing_attr(uint16_t attr_type)
{
	switch (attr_type) {
	case STUN_ATTR_MESSAGE_INTEGRITY:
	case STUN_ATTR_MESSAGE_INTEGRITY_SHA256:
	case STUN_ATTR_FINGERPRINT:
		return true;
	default:
		return false;
	}
}

/* Does an attribute with a value of `value_len` bytes still fit the buffer, and
 * the message with it the 16-bit Length of the header?
 */
static bool attr_fits(const struct stun_builder *builder, size_t value_len)
{
	size_t need = ATTR_HEADER_SIZE + padded_len(value_len);

	if (builder->buf_size - builder->msg_len < need) {
		return false;
	}
	if (builder->msg_len + need - STUN_HEADER_SIZE > UINT16_MAX) {
		return false;
	}

	return true;
}

/**
 * @brief Make room for one attribute at the end of the message
 *
 * Writes the attribute header, zeroes the padding and counts the attribute in
 * the header Length. The caller writes the value. When the attribute does not
 * fit, nothing is written and -EMSGSIZE is remembered in the builder.
 *
 * @param builder Builder with no error remembered.
 * @param attr_type Attribute type.
 * @param value_len Length of the value.
 *
 * @return Where the value goes, or NULL if the attribute does not fit.
 */
static uint8_t *attr_reserve(struct stun_builder *builder, uint16_t attr_type, size_t value_len)
{
	/* The new attribute starts where the message ends so far. */
	uint8_t *attr_start = builder->buf + builder->msg_len;

	if (!attr_fits(builder, value_len)) {
		builder->err = -EMSGSIZE;
		return NULL;
	}
	sys_put_be16(attr_type, attr_start);
	/* Length holds the value without its padding (RFC 8489 section 14). */
	sys_put_be16((uint16_t)value_len, attr_start + ATTR_LENGTH_OFFSET);
	/* The padding after the value is zero, whatever the buffer held. */
	memset(attr_start + ATTR_HEADER_SIZE + value_len, 0, padded_len(value_len) - value_len);
	builder->msg_len += ATTR_HEADER_SIZE + padded_len(value_len);
	/* The header Length follows every attribute, so the message is whole after each call. */
	sys_put_be16((uint16_t)(builder->msg_len - STUN_HEADER_SIZE),
		     builder->buf + HDR_LENGTH_OFFSET);
	return attr_start + ATTR_HEADER_SIZE;
}

/* Add an attribute whose value is copied as it is: room from attr_reserve(), then the bytes. */
static int add_value(struct stun_builder *builder, uint16_t attr_type, const uint8_t *value,
		     size_t value_len)
{
	uint8_t *value_start = attr_reserve(builder, attr_type, value_len);

	if (value_start == NULL) {
		return builder->err;
	}
	if (value_len != 0U) {
		memcpy(value_start, value, value_len);
	}
	return 0;
}

int stun_add_attr(struct stun_builder *builder, uint16_t attr_type, const uint8_t *value,
		  size_t value_len)
{
	int rc = builder_ready(builder, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	if (value_len > UINT16_MAX) {
		return -EINVAL;
	}
	if (!null_only_if_empty(value, value_len)) {
		return -EINVAL;
	}
	if (is_closing_attr(attr_type)) {
		return -EINVAL;
	}
	return add_value(builder, attr_type, value, value_len);
}

int stun_add_software(struct stun_builder *builder, const char *sw)
{
	int rc = builder_ready(builder, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	if (sw == NULL) {
		return -EINVAL;
	}
	if (!utf8_within_char_limit(sw)) {
		return -EINVAL;
	}
	return add_value(builder, STUN_ATTR_SOFTWARE, (const uint8_t *)sw, strlen(sw));
}

int stun_add_username(struct stun_builder *builder, const char *username)
{
	int rc = builder_ready(builder, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	if (username == NULL) {
		return -EINVAL;
	}
	if (strlen(username) > STUN_USERNAME_MAX_LEN) {
		return -EINVAL;
	}
	return add_value(builder, STUN_ATTR_USERNAME, (const uint8_t *)username, strlen(username));
}

int stun_add_xor_address(struct stun_builder *builder, uint16_t attr_type,
			 const struct net_sockaddr *addr)
{
	int rc = builder_ready(builder, STAGE_ATTRS);
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	if (addr == NULL) {
		return -EINVAL;
	}
	if (is_closing_attr(attr_type)) {
		return -EINVAL;
	}

	if (addr->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *sin4 = net_sin(addr);

		val = attr_reserve(builder, attr_type, ADDR_LEN_IPV4);
		if (val == NULL) {
			return builder->err;
		}
		val[ADDR_RESERVED_OFFSET] = 0U;
		val[ADDR_FAMILY_OFFSET] = ADDR_FAMILY_VALUE_IPV4;
		sys_put_be16(net_ntohs(sin4->sin_port) ^ PORT_XOR_KEY, val + ADDR_PORT_OFFSET);
		xor_with_cookie_and_txid(val + ADDR_IP_OFFSET, sin4->sin_addr.s4_addr, builder->buf,
					 NET_IPV4_ADDR_SIZE);
		return 0;
	}

	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		return -EAFNOSUPPORT;
	}

	if (addr->sa_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *sin6 = net_sin6(addr);

		val = attr_reserve(builder, attr_type, ADDR_LEN_IPV6);
		if (val == NULL) {
			return builder->err;
		}
		val[ADDR_RESERVED_OFFSET] = 0U;
		val[ADDR_FAMILY_OFFSET] = ADDR_FAMILY_VALUE_IPV6;
		sys_put_be16(net_ntohs(sin6->sin6_port) ^ PORT_XOR_KEY, val + ADDR_PORT_OFFSET);
		xor_with_cookie_and_txid(val + ADDR_IP_OFFSET, sin6->sin6_addr.s6_addr,
					 builder->buf, NET_IPV6_ADDR_SIZE);
		return 0;
	}

	return -EAFNOSUPPORT;
}

int stun_add_xor_mapped_address(struct stun_builder *builder, const struct net_sockaddr *addr)
{
	return stun_add_xor_address(builder, STUN_ATTR_XOR_MAPPED_ADDRESS, addr);
}

int stun_add_priority(struct stun_builder *builder, uint32_t priority)
{
	int rc = builder_ready(builder, STAGE_ATTRS);
	uint8_t *val;

	if (rc != 0) {
		return rc;
	}
	val = attr_reserve(builder, STUN_ATTR_PRIORITY, sizeof(priority));
	if (val == NULL) {
		return builder->err;
	}
	sys_put_be32(priority, val);
	return 0;
}

int stun_add_use_candidate(struct stun_builder *builder)
{
	int rc = builder_ready(builder, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	return add_value(builder, STUN_ATTR_USE_CANDIDATE, NULL, 0U);
}

int stun_add_error_code(struct stun_builder *builder, uint16_t code, const char *reason)
{
	int rc = builder_ready(builder, STAGE_ATTRS);
	size_t reason_len = (reason != NULL) ? strlen(reason) : 0U;
	uint8_t *value;

	if (rc != 0) {
		return rc;
	}
	if (code < ERROR_CODE_MIN) {
		return -EINVAL;
	}
	if (code > ERROR_CODE_MAX) {
		return -EINVAL;
	}
	if (reason != NULL) {
		if (!utf8_within_char_limit(reason)) {
			return -EINVAL;
		}
	}

	value = attr_reserve(builder, STUN_ATTR_ERROR_CODE, ERROR_REASON_OFFSET + reason_len);
	if (value == NULL) {
		return builder->err;
	}
	/* The reserved bytes ahead of the class. */
	memset(value, 0, ERROR_CLASS_OFFSET);
	value[ERROR_CLASS_OFFSET] = (uint8_t)(code / ERROR_CLASS_FACTOR);
	value[ERROR_NUMBER_OFFSET] = (uint8_t)(code % ERROR_CLASS_FACTOR);
	if (reason_len != 0U) {
		memcpy(value + ERROR_REASON_OFFSET, (const uint8_t *)reason, reason_len);
	}
	return 0;
}

int stun_add_unknown_attributes(struct stun_builder *builder, const uint16_t *types, size_t n_types)
{
	int rc = builder_ready(builder, STAGE_ATTRS);
	uint8_t *value = NULL;

	if (rc != 0) {
		return rc;
	}
	if (!null_only_if_empty(types, n_types)) {
		return -EINVAL;
	}
	if (n_types > UINT16_MAX / sizeof(uint16_t)) {
		return -EINVAL;
	}

	value = attr_reserve(builder, STUN_ATTR_UNKNOWN_ATTRIBUTES, n_types * sizeof(uint16_t));
	if (value == NULL) {
		return builder->err;
	}
	for (size_t i = 0; i < n_types; i++) {
		sys_put_be16(types[i], value + i * sizeof(uint16_t));
	}
	return 0;
}

/* Both role attributes carry the 64-bit tie-breaker (RFC 8445 section 7.1.3). */
static int add_role(struct stun_builder *builder, uint16_t attr_type, uint64_t tiebreaker)
{
	int rc = builder_ready(builder, STAGE_ATTRS);
	uint8_t *value = NULL;

	if (rc != 0) {
		return rc;
	}
	value = attr_reserve(builder, attr_type, sizeof(tiebreaker));
	if (value == NULL) {
		return builder->err;
	}
	sys_put_be64(tiebreaker, value);
	return 0;
}

int stun_add_ice_controlling(struct stun_builder *builder, uint64_t tiebreaker)
{
	return add_role(builder, STUN_ATTR_ICE_CONTROLLING, tiebreaker);
}

int stun_add_ice_controlled(struct stun_builder *builder, uint64_t tiebreaker)
{
	return add_role(builder, STUN_ATTR_ICE_CONTROLLED, tiebreaker);
}

/**
 * @brief Sign the message as it stands and append the result as an attribute
 *
 * The HMAC is computed before anything is written, so a backend that fails
 * leaves the message as it was. On success the builder moves to @p stage, after
 * which only later closing attributes can be added.
 *
 * @param builder Builder that builder_ready() has accepted for this attribute.
 * @param attr_type MESSAGE-INTEGRITY or MESSAGE-INTEGRITY-SHA256.
 * @param alg HMAC algorithm.
 * @param mac_len Length of the value, at most STUN_INTEGRITY_SHA256_MAX_LEN.
 * @param key Key.
 * @param key_len Its length.
 * @param stage Stage the builder moves to.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p key is NULL or @p key_len is 0; nothing is remembered.
 * @retval -EMSGSIZE The attribute does not fit; remembered in the builder.
 * @retval -EIO The PSA backend failed; remembered in the builder.
 */
static int add_integrity_attr(struct stun_builder *builder, uint16_t attr_type, psa_algorithm_t alg,
			      size_t mac_len, const uint8_t *key, size_t key_len, uint8_t stage)
{
	uint8_t mac[STUN_INTEGRITY_SHA256_MAX_LEN] = {0};
	size_t attr_offset = builder->msg_len;
	uint16_t patched_len = 0U;
	uint8_t *value = NULL;
	int rc = 0;

	if (key == NULL) {
		return -EINVAL;
	}
	if (key_len == 0U) {
		return -EINVAL;
	}
	if (!attr_fits(builder, mac_len)) {
		builder->err = -EMSGSIZE;
		return builder->err;
	}
	/* The header Length as it will read once the attribute is in: that is what the
	 * HMAC covers (RFC 8489 sections 14.5, 14.6).
	 */
	patched_len = (uint16_t)(attr_offset + ATTR_HEADER_SIZE + mac_len - STUN_HEADER_SIZE);
	rc = hmac_sign(alg, key, key_len, builder->buf, attr_offset, patched_len, mac, mac_len);
	if (rc != 0) {
		builder->err = rc;
		return rc;
	}
	value = attr_reserve(builder, attr_type, mac_len);
	if (value == NULL) {
		return builder->err;
	}
	memcpy(value, mac, mac_len);
	builder->stage = stage;
	return 0;
}

int stun_add_integrity(struct stun_builder *builder, const uint8_t *key, size_t key_len)
{
	int rc = builder_ready(builder, STAGE_ATTRS);

	if (rc != 0) {
		return rc;
	}
	return add_integrity_attr(builder, STUN_ATTR_MESSAGE_INTEGRITY, PSA_ALG_HMAC(PSA_ALG_SHA_1),
				  STUN_INTEGRITY_LEN, key, key_len, STAGE_INTEGRITY);
}

int stun_add_integrity_sha256(struct stun_builder *builder, const uint8_t *key, size_t key_len)
{
	int rc = builder_ready(builder, STAGE_INTEGRITY);

	if (rc != 0) {
		return rc;
	}
	return add_integrity_attr(builder, STUN_ATTR_MESSAGE_INTEGRITY_SHA256,
				  PSA_ALG_HMAC(PSA_ALG_SHA_256), STUN_INTEGRITY_SHA256_MAX_LEN, key,
				  key_len, STAGE_INTEGRITY_SHA256);
}

int stun_add_fingerprint(struct stun_builder *builder)
{
	int rc = builder_ready(builder, STAGE_INTEGRITY_SHA256);
	size_t attr_offset = 0U;
	uint8_t *value = NULL;

	if (rc != 0) {
		return rc;
	}
	/* The CRC covers the header with a Length that already counts
	 * FINGERPRINT, which is what reserving the attribute leaves there.
	 */
	attr_offset = builder->msg_len;
	value = attr_reserve(builder, STUN_ATTR_FINGERPRINT, FINGERPRINT_LEN);
	if (value == NULL) {
		return builder->err;
	}
	sys_put_be32(crc32_ieee(builder->buf, attr_offset) ^ FINGERPRINT_XOR, value);
	builder->stage = STAGE_FINGERPRINT;
	return 0;
}

int stun_builder_len(const struct stun_builder *builder)
{
	if (builder == NULL) {
		return -EINVAL;
	}
	/* A remembered error stands for the message: it is not usable. */
	if (builder->err != 0) {
		return builder->err;
	}

	return (int)builder->msg_len;
}

int stun_finish(struct stun_builder *builder, const uint8_t *key, size_t key_len)
{
	int rc = stun_add_integrity(builder, key, key_len);

	if (rc != 0) {
		return rc;
	}

	return stun_finish_plain(builder);
}

int stun_finish_plain(struct stun_builder *builder)
{
	int rc = stun_add_fingerprint(builder);

	if (rc != 0) {
		return rc;
	}

	return stun_builder_len(builder);
}
