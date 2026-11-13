/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "it51xxx_crypto.h"
#include "it51xxx_crypto_rsa.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <soc_common.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_IT51XXX_PSA_CRYPTO_LOG_LEVEL
LOG_MODULE_REGISTER(it51xxx_psa_crypto_rsa);

#define DT_DRV_COMPAT    ite_it51xxx_psa_rsa
#define IT51XXX_RSA_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "it51xxx: supported one psa-crypto-rsa node only");

#define IT51XXX_RSA_IRQ DT_IRQN(DT_PARENT(IT51XXX_RSA_NODE))

#define rsa_dlm (hwcrypto_dlm.rsa)

#define RSA40_CTRL             0x40
#define RSA_ENABLE             BIT(7)
#define RSA_EXPONENT_MASK      GENMASK(5, 4)
#define RSA_EXPONENT_SELECT(n) FIELD_PREP(GENMASK(5, 4), (n))
#define RSA_INTERRUPT_ENABLE   BIT(1)
#define RSA_CLOCK_ENABLE       BIT(0)

#define RSA41_CTRL_2                      0x41
#define RSA_PUBLIC_MODULUS_LENGTH_DIVISOR 32U
#define PUBLIC_MODULUS_LENGTH(n)          ((n) / RSA_PUBLIC_MODULUS_LENGTH_DIVISOR)

#define RSA42_STATUS          0x42
#define RSA_CALCULATION_START BIT(7)
#define RSA_CALCULATION_DONE  BIT(1)

#define RSA44_BASE_ADDR_BYTE_1 0x44
#define RSA45_BASE_ADDR_BYTE_2 0x45

enum it51xxx_rsa_key_select {
	KEY_PUBLIC_EXP_65537 = 0,
	KEY_PUBLIC_EXP_3,
	KEY_PRIVATE,
	KEY_RESERVED,
};

/* (ITU-T X.690, section 8.1.2.2)
 * Identifier Octet Layout
 */
#define IT51XXX_ASN1_CLASS_UNIVERSAL FIELD_PREP(GENMASK(7, 6), 0)
#define IT51XXX_ASN1_CONSTRUCTED     BIT(5)
#define IT51XXX_ASN1_TAG_MASK        GENMASK(4, 0)

/* (ITU-T X.680, Table 1)
 * Universal Class Tag Assignments
 */
#define IT51XXX_ASN1_TAG_INTEGER     FIELD_PREP(IT51XXX_ASN1_TAG_MASK, 2)
#define IT51XXX_ASN1_TAG_OCTETSTRING FIELD_PREP(IT51XXX_ASN1_TAG_MASK, 4)
#define IT51XXX_ASN1_TAG_NULL        FIELD_PREP(IT51XXX_ASN1_TAG_MASK, 5)
#define IT51XXX_ASN1_TAG_OID         FIELD_PREP(IT51XXX_ASN1_TAG_MASK, 6)
#define IT51XXX_ASN1_TAG_SEQUENCE    FIELD_PREP(IT51XXX_ASN1_TAG_MASK, 16)

#define IT51XXX_DER_TAG_INTEGER     (IT51XXX_ASN1_CLASS_UNIVERSAL | IT51XXX_ASN1_TAG_INTEGER)
#define IT51XXX_DER_TAG_OCTETSTRING (IT51XXX_ASN1_CLASS_UNIVERSAL | IT51XXX_ASN1_TAG_OCTETSTRING)
#define IT51XXX_DER_TAG_NULL        (IT51XXX_ASN1_CLASS_UNIVERSAL | IT51XXX_ASN1_TAG_NULL)
#define IT51XXX_DER_TAG_OID         (IT51XXX_ASN1_CLASS_UNIVERSAL | IT51XXX_ASN1_TAG_OID)
#define IT51XXX_DER_TAG_SEQUENCE                                                                   \
	(IT51XXX_ASN1_CLASS_UNIVERSAL | IT51XXX_ASN1_CONSTRUCTED | IT51XXX_ASN1_TAG_SEQUENCE)

/* Hash algorithm OID ::= {
 *   joint-iso-ccitt(2) country(16) us(840) organization(1)
 *   gov(101) csor(3) nistAlgorithm(4) 2 <id>
 * }
 */
#define IT51XXX_NIST_HASH_OID(id) {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, (id)}

struct it51xxx_der_view {
	const uint8_t *data;
	size_t size;
};

struct it51xxx_rsa_key {
	struct it51xxx_der_view n;
	struct it51xxx_der_view d;
	size_t bits;
	uint8_t exp_select;
};

struct it51xxx_digest_info {
	psa_algorithm_t alg;
	uint8_t hash_size;
	uint8_t oid_size;
	uint8_t oid[9];
};

/* Hash algorithm object identifiers (OIDs) used in DigestInfo.
 *
 * References:
 * - RFC 8017, Appendix B.1: MD5, SHA-1 and SHA-2 OIDs.
 * - NIST CSOR, hashAlgs registry: SHA-2 and SHA-3 OIDs.
 * - RFC 4880, section 5.2.2: RIPEMD-160 OID.
 */
static const struct it51xxx_digest_info it51xxx_digests[] = {
	{PSA_ALG_MD5, 16, 8, {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x05}},
	{PSA_ALG_RIPEMD160, 20, 5, {0x2b, 0x24, 0x03, 0x02, 0x01}},
	{PSA_ALG_SHA_1, 20, 5, {0x2b, 0x0e, 0x03, 0x02, 0x1a}},
	{PSA_ALG_SHA_224, 28, 9, IT51XXX_NIST_HASH_OID(4)},
	{PSA_ALG_SHA_256, 32, 9, IT51XXX_NIST_HASH_OID(1)},
	{PSA_ALG_SHA_384, 48, 9, IT51XXX_NIST_HASH_OID(2)},
	{PSA_ALG_SHA_512, 64, 9, IT51XXX_NIST_HASH_OID(3)},
	{PSA_ALG_SHA_512_224, 28, 9, IT51XXX_NIST_HASH_OID(5)},
	{PSA_ALG_SHA_512_256, 32, 9, IT51XXX_NIST_HASH_OID(6)},
	{PSA_ALG_SHA3_224, 28, 9, IT51XXX_NIST_HASH_OID(7)},
	{PSA_ALG_SHA3_256, 32, 9, IT51XXX_NIST_HASH_OID(8)},
	{PSA_ALG_SHA3_384, 48, 9, IT51XXX_NIST_HASH_OID(9)},
	{PSA_ALG_SHA3_512, 64, 9, IT51XXX_NIST_HASH_OID(10)},
};

static struct k_sem rsa_cal_done;

static void it51xxx_rsa_zeroize(void *buffer, size_t size)
{
	volatile uint8_t *p = buffer;

	while (size != 0) {
		*p++ = 0;
		size--;
	}
}

/* (RFC 8017, section 9.2, steps 2 through 5)
 * Encode the hash using EMSA-PKCS1-v1_5.
 *
 * EM: 00 || 01 || PS || 00 || DigestInfo (T).
 *
 * which:
 *   DigestInfo ::= SEQUENCE {
 *     digestAlgorithm AlgorithmIdentifier,
 *     digest OCTET STRING
 *   }
 */
static psa_status_t it51xxx_rsa_add_pkcs1_v15_padding(psa_algorithm_t alg, const uint8_t *hash,
						      size_t hash_length, uint8_t *encoded,
						      size_t encoded_size)
{
	const struct it51xxx_digest_info *digest = NULL;
	size_t prefix_size = 0;
	size_t padding_size;
	uint8_t *p;

	if (!PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (!hash && hash_length != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (alg != PSA_ALG_RSA_PKCS1V15_SIGN_RAW) {
		psa_algorithm_t hash_alg = PSA_ALG_SIGN_GET_HASH(alg);

		for (size_t i = 0; i < ARRAY_SIZE(it51xxx_digests); i++) {
			if (it51xxx_digests[i].alg == hash_alg) {
				digest = &it51xxx_digests[i];
				break;
			}
		}
		if (digest == NULL) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (hash_length != digest->hash_size) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* DigestInfo prefix contains five tag-length pairs (10 bytes) and the oid value */
		prefix_size = 10 + digest->oid_size;
	}

	/* reserve 3 fixed octets (00, 01 and the delimiter) and at least 8 octets
	 * for padding string (PS).
	 */
	if (encoded_size < 11 || prefix_size > (encoded_size - 11) ||
	    hash_length > (encoded_size - 11 - prefix_size)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* construct [ 00 || 01 || PS || 00 ] */
	padding_size = encoded_size - 3 - prefix_size - hash_length;
	encoded[0] = 0;
	encoded[1] = 1;
	memset(encoded + 2, 0xff, padding_size);
	p = encoded + 2 + padding_size;
	*p++ = 0;

	/* write the DigestInfo prefix if required */
	if (digest != NULL) {
		*p++ = IT51XXX_DER_TAG_SEQUENCE;
		*p++ = (uint8_t)(8 + digest->oid_size + hash_length);
		*p++ = IT51XXX_DER_TAG_SEQUENCE;
		*p++ = (uint8_t)(4 + digest->oid_size);
		*p++ = IT51XXX_DER_TAG_OID;
		*p++ = digest->oid_size;
		memcpy(p, digest->oid, digest->oid_size);
		p += digest->oid_size;
		*p++ = IT51XXX_DER_TAG_NULL;
		*p++ = 0;
		*p++ = IT51XXX_DER_TAG_OCTETSTRING;
		*p++ = (uint8_t)hash_length;
	}

	/* append the hash */
	if (hash_length != 0) {
		memcpy(p, hash, hash_length);
	}

	return PSA_SUCCESS;
}

/* (RFC 8017, section 7.2.1)
 * Encode the message using RSAES-PKCS1-V1_5
 *
 * EM = 00 || 02 || PS || 00 || M
 *
 * which:
 *   PS contains at least 8 pseudo-randomly nonzero octets
 */
static psa_status_t it51xxx_rsa_add_encryption_padding(const uint8_t *input, size_t input_length,
						       uint8_t *encoded, size_t encoded_size)
{
	size_t padding_size;
	psa_status_t status;

	/* at least 11 octets: 00 || 02, 8 octets PS, and 1 delimiter */
	if (encoded_size < 11 || input_length > encoded_size - 11) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	padding_size = encoded_size - input_length - 3;
	encoded[0] = 0;
	encoded[1] = 2;
	status = psa_generate_random(encoded + 2, padding_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* re-generate zero octets */
	for (size_t i = 0; i < padding_size; i++) {
		for (size_t retries = 0; encoded[2 + i] == 0; retries++) {
			if (retries == 128) {
				return PSA_ERROR_INSUFFICIENT_ENTROPY;
			}
			status = psa_generate_random(encoded + 2 + i, 1);
			if (status != PSA_SUCCESS) {
				return status;
			}
		}
	}

	encoded[2 + padding_size] = 0;

	if (input_length != 0) {
		memcpy(encoded + 3 + padding_size, input, input_length);
	}

	return PSA_SUCCESS;
}

static psa_status_t it51xxx_rsa_remove_encryption_padding(const uint8_t *encoded,
							  size_t encoded_size, uint8_t *output,
							  size_t output_size, size_t *output_length)
{
	size_t delimiter = 0;
	size_t message_length;
	uint32_t is_found = 0;
	uint32_t error;

	if (encoded_size < 11) {
		return PSA_ERROR_INVALID_PADDING;
	}

	/* (RFC 8017, section 7.2.2)
	 * Check the header and scan the entire block before reporting padding
	 * errors to prevent opponents from distinguishing error conditions
	 * through error messages or timing.
	 */
	error = encoded[0] | (encoded[1] ^ 0x02);

	/* find the first 00h delimiter after the header */
	for (size_t i = 2; i < encoded_size; i++) {
		uint32_t is_zero = ((uint32_t)encoded[i] - 1U) >> 31;
		size_t mask = (size_t)0 - (size_t)(is_zero & (is_found ^ 1U));

		delimiter = (delimiter & ~mask) | (i & mask);
		is_found |= is_zero;
	}

	/* require delimiter */
	error |= is_found ^ 1U;

	/* require at least 8 non-zero PS */
	error |= (uint32_t)(delimiter < 10);

	if (error) {
		return PSA_ERROR_INVALID_PADDING;
	}

	message_length = encoded_size - delimiter - 1;
	if (output_size < message_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (message_length != 0) {
		memcpy(output, encoded + delimiter + 1, message_length);
	}

	*output_length = message_length;

	return PSA_SUCCESS;
}

static psa_status_t it51xxx_der_read(struct it51xxx_der_view *input, uint8_t tag,
				     struct it51xxx_der_view *value)
{
	const uint8_t *p = input->data;
	size_t remaining = input->size;
	size_t length;
	size_t count;

	if (remaining < 2 || *p++ != tag) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	remaining--;
	length = *p++;
	remaining--;

	/* (ITU-T X.690, section 8.1.3.5) handle long-form length octets */
	if (length & BIT(7)) {
		count = length & GENMASK(6, 0);
		if (count == 0 || count > sizeof(size_t) || count > remaining || p[0] == 0) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		length = 0;
		for (size_t i = 0; i < count; i++) {
			length = (length << 8) | *p++;
		}

		remaining -= count;

		if (length < 128) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	if (length > remaining) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	value->data = p;
	value->size = length;
	input->data = p + length;
	input->size = remaining - length;

	return PSA_SUCCESS;
}

static psa_status_t it51xxx_der_read_positive_integer(struct it51xxx_der_view *input,
						      struct it51xxx_der_view *value)
{
	psa_status_t status = it51xxx_der_read(input, IT51XXX_DER_TAG_INTEGER, value);

	if (status != PSA_SUCCESS) {
		return status;
	}

	/* reject empty or negative values */
	if (value->size == 0 || value->data[0] & BIT(7)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (value->data[0] == 0) {
		/* reject zero and unnecessary leading zero byte */
		if (value->size == 1 || !(value->data[1] & BIT(7))) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		value->data++;
		value->size--;
	}

	return PSA_SUCCESS;
}

static psa_status_t it51xxx_parse_rsa_key(const psa_key_attributes_t *attributes,
					  const uint8_t *key_buffer, size_t key_buffer_size,
					  struct it51xxx_rsa_key *key)
{
	struct it51xxx_der_view input = {key_buffer, key_buffer_size};
	struct it51xxx_der_view sequence;
	struct it51xxx_der_view value;
	struct it51xxx_der_view e;
	psa_key_type_t type = psa_get_key_type(attributes);
	psa_status_t status;
	size_t bits;
	uint8_t top;

	memset(key, 0, sizeof(*key));

	if (type != PSA_KEY_TYPE_RSA_PUBLIC_KEY && type != PSA_KEY_TYPE_RSA_KEY_PAIR) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = it51xxx_der_read(&input, IT51XXX_DER_TAG_SEQUENCE, &sequence);
	if (status != PSA_SUCCESS || input.size != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* (RFC 8017, Appendix A.1)
	 * Parse RSAPrivateKey or RSAPublicKey which
	 *
	 * RSAPrivateKey ::= SEQUENCE {
	 *     version           Version,
	 *     modulus           INTEGER,  -- n
	 *     publicExponent    INTEGER,  -- e
	 *     privateExponent   INTEGER,  -- d
	 *     prime1            INTEGER,  -- p
	 *     prime2            INTEGER,  -- q
	 *     exponent1         INTEGER,  -- d mod (p-1)
	 *     exponent2         INTEGER,  -- d mod (q-1)
	 *     coefficient       INTEGER,  -- (inverse of q) mod p
	 *     otherPrimeInfos   OtherPrimeInfos OPTIONAL
	 * }
	 *
	 * RSAPublicKey ::= SEQUENCE {
	 *     modulus           INTEGER,  -- n
	 *     publicExponent    INTEGER   -- e
	 * }
	 */

	if (type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
		status = it51xxx_der_read(&sequence, IT51XXX_DER_TAG_INTEGER, &value);
		if (status != PSA_SUCCESS || value.size != 1) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (value.data[0] == 1) {
			/* unsupported multi-prime rsa */
			return PSA_ERROR_NOT_SUPPORTED;
		}
		if (value.data[0] != 0) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	status = it51xxx_der_read_positive_integer(&sequence, &key->n);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = it51xxx_der_read_positive_integer(&sequence, &e);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
		status = it51xxx_der_read_positive_integer(&sequence, &key->d);
		if (status != PSA_SUCCESS) {
			return status;
		}

		/* remaining fields: p, q, dp, dq and qp */
		for (uint8_t i = 0; i < 5; i++) {
			status = it51xxx_der_read_positive_integer(&sequence, &value);
			if (status != PSA_SUCCESS) {
				return status;
			}
		}
	}

	if (sequence.size != 0 || key->d.size > key->n.size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* (RFC 8017, section 3.1) RSA modulus must be odd */
	if (!(key->n.data[key->n.size - 1] & BIT(0))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (key->n.size < IT51XXX_HWCRYPTO_RSA_MIN_BYTE_LEN ||
	    key->n.size > IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN ||
	    key->n.size > sizeof(rsa_dlm.key_public) || key->n.size > sizeof(rsa_dlm.key_private) ||
	    key->n.size > sizeof(rsa_dlm.messages)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	bits = (key->n.size - 1) * BITS_PER_BYTE;
	for (top = key->n.data[0]; top != 0; top >>= 1) {
		bits++;
	}

	if (psa_get_key_bits(attributes) != 0 && psa_get_key_bits(attributes) != bits) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if ((bits % RSA_PUBLIC_MODULUS_LENGTH_DIVISOR) != 0U) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	key->bits = bits;

	/* it51xxx only supports rsa exponent 3 and 65537(10001'h) */
	if (e.size == 1 && e.data[0] == 3) {
		key->exp_select = KEY_PUBLIC_EXP_3;
	} else if (e.size == 3 && e.data[0] == 1 && e.data[1] == 0 && e.data[2] == 1) {
		key->exp_select = KEY_PUBLIC_EXP_65537;
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

static void it51xxx_set_rsa_keys(const struct it51xxx_rsa_key *key, bool use_private)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	memset(rsa_dlm.key_public, 0, sizeof(rsa_dlm.key_public));
	it51xxx_rsa_zeroize(rsa_dlm.key_private, sizeof(rsa_dlm.key_private));

	for (size_t i = 0; i < key->n.size; i++) {
		rsa_dlm.key_public[i] = key->n.data[key->n.size - 1 - i];
	}

	if (use_private) {
		for (size_t i = 0; i < key->d.size; i++) {
			rsa_dlm.key_private[i] = key->d.data[key->d.size - 1 - i];
		}
	}

	sys_write8(PUBLIC_MODULUS_LENGTH(key->bits), base + RSA41_CTRL_2);
}

static void it51xxx_rsa_isr(const struct device *dev)
{
	const mm_reg_t base = it51xxx_crypto_get_base();
	uint8_t rsa_sts = sys_read8(base + RSA42_STATUS);

	ARG_UNUSED(dev);

	if (rsa_sts & RSA_CALCULATION_DONE) {
		k_sem_give(&rsa_cal_done);
		sys_write8(rsa_sts | RSA_CALCULATION_DONE, base + RSA42_STATUS);
	}
}

static void it51xxx_start_rsa_calculation(void)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	sys_write8(RSA_CALCULATION_START, base + RSA42_STATUS);

	/* wait for completion */
	k_sem_take(&rsa_cal_done, K_FOREVER);

	sys_write8(sys_read8(base + RSA40_CTRL) & ~(RSA_ENABLE | RSA_CLOCK_ENABLE),
		   base + RSA40_CTRL);
}

static psa_status_t it51xxx_crypto_rsa_calculate(const struct it51xxx_rsa_key *key,
						 const uint8_t *input, uint8_t selector,
						 uint8_t *output, const uint8_t *expected)
{
	const mm_reg_t base = it51xxx_crypto_get_base();
	psa_status_t status;
	uint8_t control;
	uint8_t difference = 0;

	if (it51xxx_crypto_lock(K_FOREVER) != 0) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	chip_block_idle();

	it51xxx_set_rsa_keys(key, selector == KEY_PRIVATE);
	memset(rsa_dlm.messages, 0, sizeof(rsa_dlm.messages));

	for (size_t i = 0; i < key->n.size; i++) {
		rsa_dlm.messages[i] = input[key->n.size - 1 - i];
	}

	control = sys_read8(base + RSA40_CTRL) & ~RSA_EXPONENT_MASK;
	control |= RSA_ENABLE | RSA_CLOCK_ENABLE | RSA_EXPONENT_SELECT(selector);
	sys_write8(control, base + RSA40_CTRL);
	it51xxx_start_rsa_calculation();

	for (size_t i = 0; i < key->n.size; i++) {
		if (expected != NULL) {
			difference |= rsa_dlm.messages[i] ^ expected[key->n.size - 1 - i];
		} else {
			output[key->n.size - 1 - i] = rsa_dlm.messages[i];
		}
	}

	status = difference ? PSA_ERROR_INVALID_SIGNATURE : PSA_SUCCESS;

	it51xxx_rsa_zeroize(rsa_dlm.key_private, sizeof(rsa_dlm.key_private));
	it51xxx_rsa_zeroize(rsa_dlm.key_public, sizeof(rsa_dlm.key_public));
	it51xxx_rsa_zeroize(rsa_dlm.messages, sizeof(rsa_dlm.messages));

	chip_permit_idle();
	it51xxx_crypto_unlock();

	return status;
}

psa_status_t ite_it51xxx_sign_hash(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
				   uint8_t *signature, size_t signature_size,
				   size_t *signature_length)
{
	struct it51xxx_rsa_key key;
	uint8_t encoded[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	psa_status_t status;

	if (signature_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*signature_length = 0;

	if (attributes == NULL || key_buffer == NULL || key_buffer_size == 0 || signature == NULL ||
	    (hash == NULL && hash_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_RSA_KEY_PAIR) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (!PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) && !PSA_ALG_IS_RSA_PSS(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = it51xxx_parse_rsa_key(attributes, key_buffer, key_buffer_size, &key);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (signature_size < key.n.size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (PSA_ALG_IS_RSA_PSS(alg)) {
		/* unsupported hash signing using pss padding currently */
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = it51xxx_rsa_add_pkcs1_v15_padding(alg, hash, hash_length, encoded, key.n.size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* (RFC 8017, section 5.2.1) RSASP1: require 0 <= m < n */
	if (memcmp(encoded, key.n.data, key.n.size) >= 0) {
		status = PSA_ERROR_INVALID_ARGUMENT;
	} else {
		status = it51xxx_crypto_rsa_calculate(&key, encoded, KEY_PRIVATE, signature, NULL);
	}

	it51xxx_rsa_zeroize(encoded, sizeof(encoded));

	if (status == PSA_SUCCESS) {
		*signature_length = key.n.size;
		LOG_HEXDUMP_DBG(signature, *signature_length, "hash signature:");
	}

	return status;
}

psa_status_t ite_it51xxx_verify_hash(const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
				     const uint8_t *signature, size_t signature_length)
{
	struct it51xxx_rsa_key key;
	uint8_t expected[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	psa_status_t status;

	if (attributes == NULL || key_buffer == NULL || key_buffer_size == 0 || signature == NULL ||
	    (hash == NULL && hash_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) && !PSA_ALG_IS_RSA_PSS(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = it51xxx_parse_rsa_key(attributes, key_buffer, key_buffer_size, &key);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (signature_length != key.n.size) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	/* (RFC 8017, section 5.2.2) RSAVP1: require 0 <= s < n */
	if (memcmp(signature, key.n.data, key.n.size) >= 0) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	if (PSA_ALG_IS_RSA_PSS(alg)) {
		/* unsupported signature verification using pss padding currently */
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = it51xxx_rsa_add_pkcs1_v15_padding(alg, hash, hash_length, expected, key.n.size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = it51xxx_crypto_rsa_calculate(&key, signature, key.exp_select, NULL, expected);

	it51xxx_rsa_zeroize(expected, sizeof(expected));

	return status;
}

psa_status_t ite_it51xxx_asymmetric_encrypt(const psa_key_attributes_t *attributes,
					    const uint8_t *key_buffer, size_t key_buffer_size,
					    psa_algorithm_t alg, const uint8_t *input,
					    size_t input_length, const uint8_t *salt,
					    size_t salt_length, uint8_t *output, size_t output_size,
					    size_t *output_length)
{
	struct it51xxx_rsa_key key;
	uint8_t encoded[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	psa_status_t status;

	if (output_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*output_length = 0;

	if (attributes == NULL || key_buffer == NULL || key_buffer_size == 0 || output == NULL ||
	    (input == NULL && input_length != 0) || (salt == NULL && salt_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (alg != PSA_ALG_RSA_PKCS1V15_CRYPT && !PSA_ALG_IS_RSA_OAEP(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT && salt_length != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = it51xxx_parse_rsa_key(attributes, key_buffer, key_buffer_size, &key);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (output_size < key.n.size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (PSA_ALG_IS_RSA_OAEP(alg)) {
		/* unsupported rsa encrypt using oaep padding currently */
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = it51xxx_rsa_add_encryption_padding(input, input_length, encoded, key.n.size);
	if (status == PSA_SUCCESS) {
		/* (RFC 8017, section 5.1.1) RSAEP: requires 0 <= m < n */
		if (memcmp(encoded, key.n.data, key.n.size) >= 0) {
			status = PSA_ERROR_INVALID_ARGUMENT;
		} else {
			status = it51xxx_crypto_rsa_calculate(&key, encoded, key.exp_select, output,
							      NULL);
		}
	}

	it51xxx_rsa_zeroize(encoded, sizeof(encoded));

	if (status == PSA_SUCCESS) {
		*output_length = key.n.size;
		LOG_HEXDUMP_DBG(output, *output_length, "rsa encrypt:");
	}

	return status;
}

psa_status_t ite_it51xxx_asymmetric_decrypt(const psa_key_attributes_t *attributes,
					    const uint8_t *key_buffer, size_t key_buffer_size,
					    psa_algorithm_t alg, const uint8_t *input,
					    size_t input_length, const uint8_t *salt,
					    size_t salt_length, uint8_t *output, size_t output_size,
					    size_t *output_length)
{
	struct it51xxx_rsa_key key;
	uint8_t encoded[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	psa_status_t status;

	if (output_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*output_length = 0;

	if (attributes == NULL || key_buffer == NULL || key_buffer_size == 0 || input == NULL ||
	    (output == NULL && output_size != 0) || (salt == NULL && salt_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if ((alg != PSA_ALG_RSA_PKCS1V15_CRYPT && !PSA_ALG_IS_RSA_OAEP(alg)) ||
	    psa_get_key_type(attributes) != PSA_KEY_TYPE_RSA_KEY_PAIR) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT && salt_length != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = it51xxx_parse_rsa_key(attributes, key_buffer, key_buffer_size, &key);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (input_length != key.n.size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (PSA_ALG_IS_RSA_OAEP(alg)) {
		/* unsupported rsa decrypt using oaep padding currently */
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/* (RFC 8017, section 5.1.2) RSADP: require 0 <= c < n */
	if (memcmp(input, key.n.data, key.n.size) >= 0) {
		return PSA_ERROR_INVALID_PADDING;
	}

	status = it51xxx_crypto_rsa_calculate(&key, input, KEY_PRIVATE, encoded, NULL);
	if (status == PSA_SUCCESS) {
		status = it51xxx_rsa_remove_encryption_padding(encoded, key.n.size, output,
							       output_size, output_length);
	}

	it51xxx_rsa_zeroize(encoded, sizeof(encoded));

	return status;
}

psa_status_t it51xxx_crypto_rsa_init(void)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	k_sem_init(&rsa_cal_done, 0, 1);

	LOG_INF("rsa dlm addr %p (%zu)", (void *)&rsa_dlm, sizeof(rsa_dlm));
	sys_write8(FIELD_GET(GENMASK(15, 8), (uintptr_t)&rsa_dlm), base + RSA44_BASE_ADDR_BYTE_1);
	sys_write8(FIELD_GET(GENMASK(17, 16), (uintptr_t)&rsa_dlm), base + RSA45_BASE_ADDR_BYTE_2);
	sys_write8(RSA_INTERRUPT_ENABLE, base + RSA40_CTRL);

	IRQ_CONNECT(IT51XXX_RSA_IRQ, 0, it51xxx_rsa_isr, NULL, 0);
	irq_enable(IT51XXX_RSA_IRQ);

	return PSA_SUCCESS;
}
