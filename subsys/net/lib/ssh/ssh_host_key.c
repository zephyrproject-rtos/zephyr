/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include <psa/crypto.h>
#include <zephyr/net/ssh/keygen.h>

#include "ssh_host_key.h"

/* mbedtls_pk used only for PEM/DER import/export bridge */
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>

/* mbedtls_pk_write_pubkey() outputs raw PKCS#1 RSAPublicKey DER (without
 * SPKI wrapper) which is the format PSA expects for public key import.
 * It is declared as a private API behind MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS.
 */
#if !defined(MBEDTLS_ALLOW_PRIVATE_ACCESS)
#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#endif
#include <mbedtls/private/pk_private.h>

#define SSH_RSA_MAX_KEY_BITS 4096
#define SSH_RSA_MAX_KEY_BYTES (SSH_RSA_MAX_KEY_BITS / 8)

static int ensure_psa_crypto_init(void)
{
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		NET_ERR("PSA crypto init failed: %d", status);
		return -EIO;
	}

	return 0;
}

struct ssh_host_key {
	enum ssh_host_key_type key_type;
	psa_key_id_t key_id;
	size_t key_bits;
	bool in_use;
};

struct host_key_alg_info {
	struct ssh_string signature_type_string;
	psa_algorithm_t hash_alg;
};

static struct ssh_host_key ssh_host_key[CONFIG_SSH_MAX_HOST_KEYS];

static const struct host_key_alg_info host_key_alg_info[] = {
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_256
	[SSH_HOST_KEY_ALG_RSA_SHA2_256] = {
		.signature_type_string = SSH_STRING_LITERAL("rsa-sha2-256"),
		.hash_alg = PSA_ALG_SHA_256
	},
#endif
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_512
	[SSH_HOST_KEY_ALG_RSA_SHA2_512] = {
		.signature_type_string = SSH_STRING_LITERAL("rsa-sha2-512"),
		.hash_alg = PSA_ALG_SHA_512
	},
#endif
};

#define SSH_RSA_EXPONENT 65537

static const struct host_key_alg_info *get_host_key_alg_info(enum ssh_host_key_alg alg)
{
	switch (alg) {
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_256
	case SSH_HOST_KEY_ALG_RSA_SHA2_256:
#endif
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_512
	case SSH_HOST_KEY_ALG_RSA_SHA2_512:
#endif
		return &host_key_alg_info[alg];
	default:
		return NULL;
	}
}

static int signature_type_str_to_id(const struct ssh_string *signature_type)
{
	for (int i = 0; i < ARRAY_SIZE(host_key_alg_info); i++) {
		if (ssh_strings_equal(&host_key_alg_info[i].signature_type_string,
				      signature_type)) {
			return i;
		}
	}

	return -ENOTSUP;
}

/* Minimal ASN.1 DER helpers for RSA public key N/E extraction and construction */

static int der_parse_tag_length(const uint8_t **p, const uint8_t *end,
				uint8_t expected_tag, size_t *content_len)
{
	if (*p >= end || **p != expected_tag) {
		return -1;
	}
	(*p)++;
	if (*p >= end) {
		return -1;
	}

	if (**p < 0x80) {
		*content_len = **p;
		(*p)++;
	} else {
		size_t num_bytes = **p & 0x7F;

		(*p)++;

		if (num_bytes > sizeof(size_t) || *p + num_bytes > end) {
			return -1;
		}

		*content_len = 0;

		for (size_t i = 0; i < num_bytes; i++) {
			*content_len = (*content_len << 8) | **p;
			(*p)++;
		}
	}

	if (*p + *content_len > end) {
		return -1;
	}

	return 0;
}

/* Parse RSAPublicKey DER: SEQUENCE { INTEGER N, INTEGER E } */
static int der_parse_rsa_pubkey(const uint8_t *der, size_t der_len,
				const uint8_t **n, size_t *n_len,
				const uint8_t **e, size_t *e_len)
{
	const uint8_t *p = der;
	const uint8_t *end = der + der_len;
	const uint8_t *seq_end;
	size_t seq_len;

	if (der_parse_tag_length(&p, end, 0x30, &seq_len) != 0) {
		return -1;
	}

	seq_end = p + seq_len;

	/* INTEGER N */
	if (der_parse_tag_length(&p, seq_end, 0x02, n_len) != 0) {
		return -1;
	}

	*n = p;
	p += *n_len;

	/* INTEGER E */
	if (der_parse_tag_length(&p, seq_end, 0x02, e_len) != 0) {
		return -1;
	}

	*e = p;

	return 0;
}

static size_t der_length_encoding_size(size_t length)
{
	size_t n = 0;
	size_t tmp = length;

	if (length < 0x80) {
		return 1;
	}

	while (tmp > 0) {
		n++;
		tmp >>= 8;
	}

	return 1 + n;
}

static size_t der_write_length(uint8_t *p, size_t length)
{
	size_t n = 0;
	size_t tmp = length;

	if (length < 0x80) {
		p[0] = (uint8_t)length;
		return 1;
	}

	while (tmp > 0) {
		n++;
		tmp >>= 8;
	}

	p[0] = 0x80 | (uint8_t)n;

	for (size_t i = n; i > 0; i--) {
		p[i] = length & 0xFF;
		length >>= 8;
	}

	return 1 + n;
}

/* Compute size of a DER INTEGER encoding for a big-endian unsigned value */
static size_t der_integer_encoded_size(const uint8_t *data, size_t len)
{
	size_t val_len;

	/* Strip leading zeros but keep at least one byte */
	while (len > 1 && data[0] == 0) {
		data++;
		len--;
	}

	/* Add leading zero if high bit is set */
	val_len = len + ((data[0] & 0x80) ? 1 : 0);

	return 1 + der_length_encoding_size(val_len) + val_len;
}

static size_t der_write_integer(uint8_t *p, const uint8_t *data, size_t len)
{
	size_t offset = 0;
	size_t val_len;

	/* Strip leading zeros */
	while (len > 1 && data[0] == 0) {
		data++;
		len--;
	}

	val_len = len + ((data[0] & 0x80) ? 1 : 0);

	p[offset++] = 0x02; /* INTEGER tag */
	offset += der_write_length(&p[offset], val_len);

	if (data[0] & 0x80) {
		p[offset++] = 0x00; /* sign byte */
	}

	memcpy(&p[offset], data, len);
	offset += len;

	return offset;
}

/* Build DER-encoded RSAPublicKey from big-endian N and E */
static int der_build_rsa_pubkey(const uint8_t *n, size_t n_len,
				const uint8_t *e, size_t e_len,
				uint8_t *der, size_t der_size, size_t *der_len_out)
{
	size_t int_n_size = der_integer_encoded_size(n, n_len);
	size_t int_e_size = der_integer_encoded_size(e, e_len);
	size_t seq_content_len = int_n_size + int_e_size;
	size_t total_len = 1 + der_length_encoding_size(seq_content_len) + seq_content_len;
	size_t offset = 0;

	if (total_len > der_size) {
		return -ENOBUFS;
	}

	der[offset++] = 0x30; /* SEQUENCE tag */
	offset += der_write_length(&der[offset], seq_content_len);
	offset += der_write_integer(&der[offset], n, n_len);
	offset += der_write_integer(&der[offset], e, e_len);

	*der_len_out = offset;
	return 0;
}

int ssh_keygen(int key_index, enum ssh_host_key_type key_type, size_t key_size_bits)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	struct ssh_host_key *host_key;
	psa_status_t status;
	int ret;

	ret = ensure_psa_crypto_init();
	if (ret != 0) {
		return ret;
	}

	if (key_index < 0 || key_index >= ARRAY_SIZE(ssh_host_key) ||
	    key_size_bits < 1024 || key_size_bits > SSH_RSA_MAX_KEY_BITS) {
		return -EINVAL;
	}

	host_key = &ssh_host_key[key_index];
	if (host_key->in_use) {
		return -EALREADY;
	}

	switch (key_type) {
#ifdef CONFIG_SSH_HOST_KEY_RSA
	case SSH_HOST_KEY_TYPE_RSA:
		break;
#endif
	default:
		return -ENOTSUP;
	}

	psa_set_key_type(&attr, PSA_KEY_TYPE_RSA_KEY_PAIR);
	psa_set_key_bits(&attr, key_size_bits);
	psa_set_key_algorithm(&attr, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_ANY_HASH));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH |
				PSA_KEY_USAGE_EXPORT);

	status = psa_generate_key(&attr, &host_key->key_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		NET_ERR("RSA host key gen failed: %d", status);
		return -EIO;
	}

	host_key->in_use = true;
	host_key->key_type = key_type;
	host_key->key_bits = key_size_bits;

	return 0;
}

static void free_host_key(struct ssh_host_key *host_key)
{
	psa_destroy_key(host_key->key_id);
	host_key->key_id = PSA_KEY_ID_NULL;
}

int ssh_keygen_free(int key_index)
{
	struct ssh_host_key *host_key;

	if (key_index < 0 || key_index >= ARRAY_SIZE(ssh_host_key)) {
		return -EINVAL;
	}

	host_key = &ssh_host_key[key_index];
	if (!host_key->in_use) {
		return -EALREADY;
	}

	host_key->in_use = false;
	free_host_key(host_key);

	return 0;
}

int ssh_keygen_export(int key_index, bool private_key, enum ssh_host_key_format fmt,
		      void *buf, size_t buf_len)
{
	struct ssh_host_key *host_key;
	int ret;

	if (key_index < 0 || key_index >= ARRAY_SIZE(ssh_host_key)) {
		return -EINVAL;
	}

	host_key = &ssh_host_key[key_index];
	if (!host_key->in_use) {
		return -EINVAL;
	}

	if (fmt == SSH_HOST_KEY_FORMAT_DER) {
		size_t der_len;
		psa_status_t status;

		if (private_key) {
			status = psa_export_key(host_key->key_id, buf, buf_len, &der_len);
		} else {
			status = psa_export_public_key(host_key->key_id, buf, buf_len, &der_len);
		}

		if (status != PSA_SUCCESS) {
			return -EIO;
		}

		ret = (int)der_len;
	} else if (fmt == SSH_HOST_KEY_FORMAT_PEM) {
		/* Export DER from PSA, then convert to PEM via mbedtls_pk bridge */
		uint8_t der_buf[PSA_EXPORT_KEY_PAIR_MAX_SIZE];
		mbedtls_pk_context pk;
		psa_status_t status;
		size_t der_len;

		if (private_key) {
			status = psa_export_key(host_key->key_id, der_buf,
						sizeof(der_buf), &der_len);
		} else {
			status = psa_export_public_key(host_key->key_id, der_buf,
						       sizeof(der_buf), &der_len);
		}

		if (status != PSA_SUCCESS) {
			return -EIO;
		}

		/* Use mbedtls_pk to parse the DER and write PEM */
		mbedtls_pk_init(&pk);

		if (private_key) {
			ret = mbedtls_pk_parse_key(&pk, der_buf, der_len, NULL, 0);
		} else {
			ret = mbedtls_pk_parse_public_key(&pk, der_buf, der_len);
		}

		if (ret != 0) {
			mbedtls_pk_free(&pk);
			return -EIO;
		}

		if (private_key) {
			ret = mbedtls_pk_write_key_pem(&pk, buf, buf_len);
		} else {
			ret = mbedtls_pk_write_pubkey_pem(&pk, buf, buf_len);
		}

		mbedtls_pk_free(&pk);
	} else {
		return -EINVAL;
	}

	if (ret < 0) {
		return -EIO;
	}

	return ret;
}

int ssh_keygen_import(int key_index, bool private_key, enum ssh_host_key_format fmt,
		      const void *buf, size_t buf_len)
{
	psa_key_attributes_t attr_out = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t der_buf[PSA_EXPORT_KEY_PAIR_MAX_SIZE];
	struct ssh_host_key *host_key;
	mbedtls_pk_context pk;
	psa_status_t status;
	size_t der_len;
	uint8_t *der;
	int ret;

	/* mbedtls_pk_parse_key handles PEM, PKCS#1, and PKCS#8 formats */
	ARG_UNUSED(fmt);

	ret = ensure_psa_crypto_init();
	if (ret != 0) {
		return ret;
	}

	if (key_index < 0 || key_index >= ARRAY_SIZE(ssh_host_key)) {
		return -EINVAL;
	}

	host_key = &ssh_host_key[key_index];
	if (host_key->in_use) {
		return -EALREADY;
	}

	/* Parse with mbedtls to normalize to PKCS#1 DER, then import to PSA */
	mbedtls_pk_init(&pk);

	if (private_key) {
		ret = mbedtls_pk_parse_key(&pk, buf, buf_len, NULL, 0);
	} else {
		ret = mbedtls_pk_parse_public_key(&pk, buf, buf_len);
	}

	if (ret != 0) {
		NET_ERR("Cannot parse key: -0x%04X", -ret);
		mbedtls_pk_free(&pk);
		return -EIO;
	}

	/* Re-export as DER to get a known format for PSA */
	if (private_key) {
		ret = mbedtls_pk_write_key_der(&pk, der_buf, sizeof(der_buf));
	} else {
		/* mbedtls_pk_write_pubkey writes PKCS#1 RSAPublicKey (N, E only),
		 * which is the format PSA expects. mbedtls_pk_write_pubkey_der
		 * would write SubjectPublicKeyInfo (with AlgorithmIdentifier
		 * wrapper) which PSA rejects.
		 */
		unsigned char *p = der_buf + sizeof(der_buf);

		ret = mbedtls_pk_write_pubkey(&p, der_buf, &pk);
	}

	mbedtls_pk_free(&pk);

	if (ret < 0) {
		NET_ERR("Cannot write in der format: -0x%04X", -ret);
		return -EIO;
	}

	/* mbedtls writes DER to end of buffer */
	der = der_buf + sizeof(der_buf) - ret;
	der_len = (size_t)ret;

	/* Import into PSA */
	if (private_key) {
		psa_set_key_type(&attr, PSA_KEY_TYPE_RSA_KEY_PAIR);
		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH |
					PSA_KEY_USAGE_VERIFY_HASH |
					PSA_KEY_USAGE_EXPORT);
	} else {
		psa_set_key_type(&attr, PSA_KEY_TYPE_RSA_PUBLIC_KEY);
		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_VERIFY_HASH |
					PSA_KEY_USAGE_EXPORT);
	}

	psa_set_key_algorithm(&attr, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_ANY_HASH));

	status = psa_import_key(&attr, der, der_len, &host_key->key_id);
	psa_reset_key_attributes(&attr);

	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to import key: %d", status);
		return -EIO;
	}

	/* Get actual key size */
	psa_get_key_attributes(host_key->key_id, &attr_out);
	host_key->key_bits = psa_get_key_bits(&attr_out);
	psa_reset_key_attributes(&attr_out);

	host_key->in_use = true;
	host_key->key_type = SSH_HOST_KEY_TYPE_RSA;

	return 0;
}

int ssh_host_key_write_pub_key(struct ssh_payload *payload, int host_key_index)
{
	static const struct ssh_string type_string = SSH_STRING_LITERAL("ssh-rsa");
	uint8_t der_buf[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	struct ssh_host_key *host_key;
	uint32_t start_offset;
	const uint8_t *N, *E;
	size_t N_len, E_len;
	psa_status_t status;
	size_t der_len;

	if (host_key_index < 0 || host_key_index >= ARRAY_SIZE(ssh_host_key)) {
		return -EINVAL;
	}

	host_key = &ssh_host_key[host_key_index];
	if (!host_key->in_use) {
		NET_DBG("Key index %d not used", host_key_index);
		return -EINVAL;
	}

	switch (host_key->key_type) {
#ifdef CONFIG_SSH_HOST_KEY_RSA
	case SSH_HOST_KEY_TYPE_RSA:
#endif
		break;
	default:
		return -ENOTSUP;
	}

	/* Export public key as DER, then parse N and E */

	status = psa_export_public_key(host_key->key_id, der_buf, sizeof(der_buf), &der_len);
	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to export pubkey: %d", status);
		return -EIO;
	}

	if (der_parse_rsa_pubkey(der_buf, der_len, &N, &N_len, &E, &E_len) != 0) {
		NET_ERR("Failed to parse pubkey DER");
		return -EIO;
	}

	/* Skip the length field, write it at the end once it is known */
	start_offset = payload->len;

	if (!ssh_payload_skip_bytes(payload, 4)) {
		return -ENOBUFS;
	}

	if (!ssh_payload_write_string(payload, &type_string)) {
		return -ENOBUFS;
	}

	if (!ssh_payload_write_mpint(payload, E, E_len, false)) {
		return -ENOBUFS;
	}

	if (!ssh_payload_write_mpint(payload, N, N_len, false)) {
		return -ENOBUFS;
	}

	if (payload->data != NULL) {
		uint32_t string_len;

		string_len = payload->len - start_offset - 4;
		sys_put_be32(string_len, &payload->data[start_offset]);
	}

	return 0;
}

int ssh_host_key_write_signature(struct ssh_payload *payload, int host_key_index,
				 enum ssh_host_key_alg host_key_alg,
				 const void *data, uint32_t data_len)
{
	const struct host_key_alg_info *alg_info;
	uint8_t signature_buff[SSH_RSA_MAX_KEY_BYTES];
	uint8_t hash[PSA_HASH_MAX_SIZE];
	struct ssh_host_key *host_key;
	uint32_t signature_start_offset;
	struct ssh_string signature;
	size_t hash_len;
	size_t sig_len;
	uint32_t string_len;
	psa_status_t status;

	if (host_key_index < 0 || host_key_index >= ARRAY_SIZE(ssh_host_key)) {
		return -EINVAL;
	}

	host_key = &ssh_host_key[host_key_index];
	if (!host_key->in_use) {
		return -EINVAL;
	}

	alg_info = get_host_key_alg_info(host_key_alg);
	if (alg_info == NULL) {
		return -ENOTSUP;
	}

	/* Hash the data using PSA */
	status = psa_hash_compute(alg_info->hash_alg, data, data_len,
				  hash, sizeof(hash), &hash_len);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	/* Skip the length field, write it at the end once it is known */
	signature_start_offset = payload->len;
	if (!ssh_payload_skip_bytes(payload, 4)) {
		return -ENOBUFS;
	}

	if (!ssh_payload_write_string(payload, &alg_info->signature_type_string)) {
		return -ENOBUFS;
	}

	status = psa_sign_hash(host_key->key_id,
			       PSA_ALG_RSA_PKCS1V15_SIGN(alg_info->hash_alg),
			       hash, hash_len, signature_buff, sizeof(signature_buff),
			       &sig_len);
	if (status != PSA_SUCCESS) {
		if (status == PSA_ERROR_NOT_PERMITTED) {
			NET_ERR("Host key cannot sign (is it a public key? "
				"Server needs a private key)");
		} else {
			NET_ERR("Failed to sign hash: %d", status);
		}
		return -EIO;
	}

	signature = (struct ssh_string){
		.len = sig_len,
		.data = signature_buff
	};

	if (!ssh_payload_write_string(payload, &signature)) {
		return -ENOBUFS;
	}

	string_len = payload->len - signature_start_offset - sizeof(uint32_t);
	sys_put_be32(string_len, &payload->data[signature_start_offset]);

	return 0;
}

static int import_pubkey_blob(struct ssh_host_key *host_key,
			      const struct ssh_string *pubkey_blob)
{
	static const struct ssh_string supported_host_key_type = SSH_STRING_LITERAL("ssh-rsa");
	psa_key_attributes_t imported_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	struct ssh_string host_key_type, host_key_E, host_key_N;
	uint8_t der_buf[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	size_t der_len;
	psa_status_t status;
	struct ssh_payload host_key_payload = {
		.size = pubkey_blob->len,
		.data = (void *)pubkey_blob->data
	};

	if (!ssh_payload_read_string(&host_key_payload, &host_key_type)) {
		NET_ERR("Length error");
		return -1;
	}

	if (!ssh_strings_equal(&host_key_type, &supported_host_key_type)) {
		NET_WARN("Unsupported host key type");
		return -1;
	}

	if (!ssh_payload_read_string(&host_key_payload, &host_key_E) ||
	    !ssh_payload_read_string(&host_key_payload, &host_key_N) ||
	    !ssh_payload_read_complete(&host_key_payload)) {
		NET_ERR("Length error");
		return -1;
	}

	/* Build DER RSAPublicKey from N and E, then import with PSA */

	if (der_build_rsa_pubkey(host_key_N.data, host_key_N.len,
				 host_key_E.data, host_key_E.len,
				 der_buf, sizeof(der_buf), &der_len) != 0) {
		NET_ERR("Failed to build DER from N/E");
		return -EIO;
	}

	psa_set_key_type(&attr, PSA_KEY_TYPE_RSA_PUBLIC_KEY);
	psa_set_key_algorithm(&attr, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_ANY_HASH));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_VERIFY_HASH);

	status = psa_import_key(&attr, der_buf, der_len, &host_key->key_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		NET_ERR("Failed to import RSA pub key: %d", status);
		return -EIO;
	}

	/* Store key size from imported key */
	status = psa_get_key_attributes(host_key->key_id, &imported_attr);
	if (status == PSA_SUCCESS) {
		host_key->key_bits = psa_get_key_bits(&imported_attr);
		psa_reset_key_attributes(&imported_attr);
	}

	return 0;
}

int ssh_host_key_verify_signature(const enum ssh_host_key_alg *host_key_alg,
				  const struct ssh_string *pubkey_blob,
				  const struct ssh_string *signature,
				  const void *data, uint32_t data_len)
{
	struct ssh_string signature_type, signature_raw;
	struct ssh_payload signature_payload = {
		.size = signature->len,
		.data = (void *)signature->data
	};
	const struct host_key_alg_info *alg_info;
	enum ssh_host_key_alg host_key_alg_tmp;
	uint8_t hash[PSA_HASH_MAX_SIZE];
	size_t hash_len;
	psa_status_t status;
	struct ssh_host_key host_key;
	size_t expected_sig_len;
	int ret;

	if (!ssh_payload_read_string(&signature_payload, &signature_type) ||
	    !ssh_payload_read_string(&signature_payload, &signature_raw) ||
	    !ssh_payload_read_complete(&signature_payload)) {
		NET_ERR("Length error");
		return -1;
	}

	if (host_key_alg == NULL) {
		/* Any supported alg */
		ret = signature_type_str_to_id(&signature_type);
		if (ret < 0) {
			return ret;
		}
		host_key_alg_tmp = ret;
		host_key_alg = &host_key_alg_tmp;
	}

	alg_info = get_host_key_alg_info(*host_key_alg);
	if (alg_info == NULL) {
		return -ENOTSUP;
	}

	if (!ssh_strings_equal(&signature_type, &alg_info->signature_type_string)) {
		NET_WARN("Incorrect signature type");
		return -1;
	}

	/* Hash the data using PSA */
	status = psa_hash_compute(alg_info->hash_alg, data, data_len,
				  hash, sizeof(hash), &hash_len);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	/* Import the host key */

	ret = import_pubkey_blob(&host_key, pubkey_blob);
	if (ret != 0) {
		NET_ERR("Failed to import pubkey");
		return ret;
	}

	/* Verify signature length */
	expected_sig_len = PSA_BITS_TO_BYTES(host_key.key_bits);
	if (signature_raw.len != expected_sig_len) {
		NET_WARN("Incorrect signature length");
		ret = -1;
		goto exit;
	}

	/* Verify signature */
	status = psa_verify_hash(host_key.key_id,
				 PSA_ALG_RSA_PKCS1V15_SIGN(alg_info->hash_alg),
				 hash, hash_len, signature_raw.data, signature_raw.len);
	if (status != PSA_SUCCESS) {
		NET_WARN("Incorrect signature");
		ret = -1;
		goto exit;
	}

exit:
	free_host_key(&host_key);

	return ret;
}
