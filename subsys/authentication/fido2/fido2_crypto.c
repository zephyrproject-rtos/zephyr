/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <psa/crypto_types.h>
#include <zephyr/psa/key_ids.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include "fido2_crypto.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

/* Max number of retries for key generation */
#define FIDO2_KEYGEN_ID_RETRIES 16

/* Key size for ES256 in bits */
#define FIDO2_ES256_KEY_BITS     256
/* r||s, 32 bytes each for P-256 */
#define FIDO2_ES256_RAW_SIG_SIZE 64

/* Wrap key ID for non-discoverable credentials */
#define FIDO2_WRAP_KEY_PSA_ID   ZEPHYR_PSA_FIDO2_KEY_ID_RANGE_BEGIN
/* Wrap Key size in bits */
#define FIDO2_WRAP_KEY_BITS     256
/* nonce (12) */
#define FIDO2_WRAP_NONCE_SIZE   12
/* key_id (4 BE) || rp_id_hash (32) */
#define FIDO2_WRAP_PAYLOAD_SIZE (sizeof(uint32_t) + FIDO2_SHA256_SIZE)
/* tag (16) */
#define FIDO2_WRAP_TAG_SIZE     PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES, 256, PSA_ALG_GCM)

static psa_status_t ensure_wrap_key(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_status_t status;

	status = psa_get_key_attributes(FIDO2_WRAP_KEY_PSA_ID, &attr);
	psa_reset_key_attributes(&attr);

	if (status == PSA_SUCCESS) {
		return status;
	}

	if (status != PSA_ERROR_INVALID_HANDLE) {
		LOG_ERR("Wrap key attribute query failed: %d", status);
		return status;
	}

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, FIDO2_WRAP_KEY_BITS);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, FIDO2_WRAP_KEY_PSA_ID);

	status = psa_generate_key(&attr, &key_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Wrap key generation failed: %d", status);
		return status;
	}

	return status;
}

static psa_status_t ecdsa_int32_to_der(const uint8_t in[32], uint8_t *out, size_t out_size,
				       size_t *out_len)
{
	const uint8_t *p = in;
	size_t len = 32;
	bool needs_zero_prefix;
	size_t written = 0;

	while (len > 1 && *p == 0) {
		p++;
		len--;
	}

	needs_zero_prefix = (p[0] & 0x80U) != 0U;

	if (len + (needs_zero_prefix ? 1U : 0U) > out_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (needs_zero_prefix) {
		out[written++] = 0x00;
	}

	memcpy(out + written, p, len);
	written += len;
	*out_len = written;

	return PSA_SUCCESS;
}

static psa_status_t ecdsa_raw64_to_der(const uint8_t raw_sig[FIDO2_ES256_RAW_SIG_SIZE],
				       uint8_t *der_sig, size_t der_sig_size, size_t *der_sig_len)
{
	uint8_t r_der[33];
	uint8_t s_der[33];
	size_t r_len;
	size_t s_len;
	size_t seq_len;
	size_t total_len;
	psa_status_t status;

	status = ecdsa_int32_to_der(raw_sig, r_der, sizeof(r_der), &r_len);
	if (status) {
		return status;
	}

	status = ecdsa_int32_to_der(raw_sig + 32, s_der, sizeof(s_der), &s_len);
	if (status) {
		return status;
	}

	seq_len = 2 + r_len + 2 + s_len;
	total_len = 2 + seq_len;

	if (seq_len > 0x7f || total_len > der_sig_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	der_sig[0] = 0x30;
	der_sig[1] = (uint8_t)seq_len;
	der_sig[2] = 0x02;
	der_sig[3] = (uint8_t)r_len;
	memcpy(der_sig + 4, r_der, r_len);
	der_sig[4 + r_len] = 0x02;
	der_sig[5 + r_len] = (uint8_t)s_len;
	memcpy(der_sig + 6 + r_len, s_der, s_len);

	*der_sig_len = total_len;

	return PSA_SUCCESS;
}

/* Public API */

int fido2_crypto_init(void)
{
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA init failed status=%d", status);
		return status;
	}

	status = ensure_wrap_key();
	if (status != PSA_SUCCESS) {
		LOG_ERR("Credential wrap key init failed: %d", status);
		return status;
	}

	return status;
}

int fido2_crypto_generate_keypair(uint32_t *key_id)
{
	psa_status_t status;
	psa_key_id_t id;

	for (int i = 0; i < FIDO2_KEYGEN_ID_RETRIES; ++i) {
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_key_id_t candidate =
			(ZEPHYR_PSA_FIDO2_KEY_ID_RANGE_BEGIN + 1) +
			sys_rand32_get() % (ZEPHYR_PSA_FIDO2_KEY_ID_RANGE_SIZE - 1);

		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH);
		psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_bits(&attr, FIDO2_ES256_KEY_BITS);
		psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
		psa_set_key_id(&attr, candidate);

		status = psa_generate_key(&attr, &id);
		psa_reset_key_attributes(&attr);

		if (status == PSA_SUCCESS) {
			*key_id = id;
			return PSA_SUCCESS;
		} else if (status == PSA_ERROR_ALREADY_EXISTS) {
			continue;
		}

		LOG_ERR("Key generation failed: %d", status);
		return status;
	}

	LOG_ERR("Key ID space exhausted after %d retries", FIDO2_KEYGEN_ID_RETRIES);
	return PSA_ERROR_INSUFFICIENT_STORAGE;
}

int fido2_crypto_sign(uint32_t key_id, const uint8_t hash[FIDO2_SHA256_SIZE], uint8_t *sig,
		      size_t sig_size, size_t *sig_len)
{
	psa_status_t status;
	uint8_t raw_sig[FIDO2_ES256_RAW_SIG_SIZE];
	size_t raw_sig_len;

	status = psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), hash, FIDO2_SHA256_SIZE,
			       raw_sig, sizeof(raw_sig), &raw_sig_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Sign failed key_id=0x%08x status=%d", key_id, status);
		return status;
	}

	status = ecdsa_raw64_to_der(raw_sig, sig, sig_size, sig_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("ECDSA DER conversion failed: %d", status);
		return status;
	}

	return status;
}

int fido2_crypto_export_pubkey(uint32_t key_id, uint8_t *pub_key, size_t pub_key_size,
			       size_t *pub_key_len)
{
	psa_status_t status;

	status = psa_export_public_key(key_id, pub_key, pub_key_size, pub_key_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Public key export failed key_id=0x%08x status=%d", key_id, status);
	}

	return status;
}

int fido2_crypto_sha256(const uint8_t *data, size_t len, uint8_t hash[FIDO2_SHA256_SIZE])
{
	psa_status_t status;
	size_t hash_len;

	status = psa_hash_compute(PSA_ALG_SHA_256, data, len, hash, FIDO2_SHA256_SIZE, &hash_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Hash compute failed status=%d", status);
	}

	return status;
}

int fido2_crypto_hash_authdata(const uint8_t *auth_data, size_t auth_data_len,
			       const uint8_t client_data_hash[FIDO2_SHA256_SIZE],
			       uint8_t out[FIDO2_SHA256_SIZE])
{
	psa_hash_operation_t op = PSA_HASH_OPERATION_INIT;
	psa_status_t status;
	size_t hash_len;

	status = psa_hash_setup(&op, PSA_ALG_SHA_256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = psa_hash_update(&op, auth_data, auth_data_len);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(&op);
		return status;
	}

	status = psa_hash_update(&op, client_data_hash, FIDO2_SHA256_SIZE);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(&op);
		return status;
	}

	status = psa_hash_finish(&op, out, FIDO2_SHA256_SIZE, &hash_len);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(&op);
		return status;
	}

	return status;
}

int fido2_crypto_destroy_key(uint32_t key_id)
{
	psa_status_t status;

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Key destroy failed key_id=0x%08x status=%d", key_id, status);
	}

	return status;
}

int fido2_crypto_wrap_credential(uint32_t key_id, const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				 uint8_t *cred_id_out)
{
	uint8_t nonce[FIDO2_WRAP_NONCE_SIZE];
	uint8_t plaintext[FIDO2_WRAP_PAYLOAD_SIZE];
	uint8_t ciphertext_w_tag[FIDO2_WRAP_PAYLOAD_SIZE + FIDO2_WRAP_TAG_SIZE];
	size_t ciphertext_w_tag_len;
	psa_status_t status;

	sys_put_be32(key_id, plaintext);
	memcpy(plaintext + sizeof(key_id), rp_id_hash, FIDO2_SHA256_SIZE);

	if (sys_csrand_get(nonce, sizeof(nonce))) {
		LOG_ERR("Wrap nonce generation failed");
		return PSA_ERROR_INSUFFICIENT_ENTROPY;
	}

	/* rp_id_hash is also used as AAD */
	status =
		psa_aead_encrypt(FIDO2_WRAP_KEY_PSA_ID, PSA_ALG_GCM, nonce, sizeof(nonce),
				 rp_id_hash, FIDO2_SHA256_SIZE, plaintext, sizeof(plaintext),
				 ciphertext_w_tag, sizeof(ciphertext_w_tag), &ciphertext_w_tag_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("GCM wrap failed: %d", status);
		return status;
	}

	/* nonce(12) || ciphertext||tag(52) = 64 */
	memcpy(cred_id_out, nonce, FIDO2_WRAP_NONCE_SIZE);
	memcpy(cred_id_out + FIDO2_WRAP_NONCE_SIZE, ciphertext_w_tag, ciphertext_w_tag_len);

	return status;
}

int fido2_crypto_unwrap_credential(const uint8_t *cred_id,
				   const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				   uint32_t *key_id_out)
{
	uint8_t plaintext[FIDO2_WRAP_PAYLOAD_SIZE];
	size_t plaintext_len;
	psa_status_t status;

	status =
		psa_aead_decrypt(FIDO2_WRAP_KEY_PSA_ID, PSA_ALG_GCM, cred_id, FIDO2_WRAP_NONCE_SIZE,
				 rp_id_hash, FIDO2_SHA256_SIZE, cred_id + FIDO2_WRAP_NONCE_SIZE,
				 FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE - FIDO2_WRAP_NONCE_SIZE,
				 plaintext, sizeof(plaintext), &plaintext_len);
	if (status == PSA_SUCCESS) {
		*key_id_out = sys_get_be32(plaintext);
	}

	return status;
}
