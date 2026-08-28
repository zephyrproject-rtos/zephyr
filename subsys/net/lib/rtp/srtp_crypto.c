/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/** @file srtp_crypto.c
 *
 * @brief Internal PSA Crypto wrappers for SRTP (RFC 3711 / RFC 7714).
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(srtp_crypto, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/net/net_log.h>
#include <zephyr/net/srtp.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "srtp_crypto.h"

#define SRTP_HMAC_ALG(_tag_len) PSA_ALG_TRUNCATED_MAC(PSA_ALG_HMAC(PSA_ALG_SHA_1), _tag_len)

static int psa_to_errno(psa_status_t status)
{
	switch (status) {
	case PSA_SUCCESS:
		return 0;
	case PSA_ERROR_INVALID_SIGNATURE:
		return -EBADMSG;
	case PSA_ERROR_INSUFFICIENT_MEMORY:
		return -ENOMEM;
	case PSA_ERROR_NOT_SUPPORTED:
		return -ENOTSUP;
	default:
		return -EIO;
	}
}

static int aes_key_import(const uint8_t *key, size_t key_len, psa_algorithm_t alg,
			  psa_key_usage_t usage, psa_key_id_t *key_id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return psa_to_errno(status);
	}

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, key_len * 8U);
	psa_set_key_usage_flags(&attr, usage);
	psa_set_key_algorithm(&attr, alg);

	status = psa_import_key(&attr, key, key_len, key_id);
	psa_reset_key_attributes(&attr);

	return psa_to_errno(status);
}

int srtp_crypto_master_key_import(const uint8_t *key, size_t key_len, psa_key_id_t *key_id)
{
	return aes_key_import(key, key_len, PSA_ALG_ECB_NO_PADDING, PSA_KEY_USAGE_ENCRYPT, key_id);
}

int srtp_crypto_kdf(psa_key_id_t master_key, const uint8_t master_salt[SRTP_MASTER_SALT_LEN],
		    uint8_t label, uint64_t index_div_kdr, uint8_t *out, size_t out_len)
{
	uint8_t block[16];
	uint8_t keystream[16];
	uint8_t key_id[6];
	size_t off = 0;

	/* x = key_id XOR master_salt, key_id = label || index DIV kdr (RFC 3711 4.3.1) */
	memcpy(block, master_salt, SRTP_MASTER_SALT_LEN);
	block[7] ^= label;
	sys_put_be48(index_div_kdr, key_id);
	mem_xor_n(&block[8], &block[8], key_id, sizeof(key_id));

	for (uint16_t ctr = 0; off < out_len; ctr++) {
		size_t chunk = MIN(out_len - off, sizeof(keystream));
		size_t olen = 0;
		psa_status_t status;

		sys_put_be16(ctr, &block[14]);

		status = psa_cipher_encrypt(master_key, PSA_ALG_ECB_NO_PADDING, block,
					    sizeof(block), keystream, sizeof(keystream), &olen);
		if (status != PSA_SUCCESS || olen != sizeof(keystream)) {
			NET_DBG("KDF block encryption failed (%d)", (int)status);
			srtp_crypto_zeroize(keystream, sizeof(keystream));
			return psa_to_errno(status);
		}

		memcpy(out + off, keystream, chunk);
		off += chunk;
	}

	srtp_crypto_zeroize(keystream, sizeof(keystream));

	return 0;
}

int srtp_crypto_cm_key_import(const uint8_t key[SRTP_AES_128_KEY_LEN], psa_key_id_t *key_id)
{
	/* AES-CM is used as a keystream generator, only encryption is needed */
	return aes_key_import(key, SRTP_AES_128_KEY_LEN, PSA_ALG_CTR, PSA_KEY_USAGE_ENCRYPT,
			      key_id);
}

int srtp_crypto_gcm_key_import(const uint8_t *key, size_t key_len, psa_key_id_t *key_id)
{
	return aes_key_import(key, key_len, PSA_ALG_GCM,
			      PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT, key_id);
}

int srtp_crypto_auth_key_import(const uint8_t key[SRTP_HMAC_SHA1_KEY_LEN], size_t tag_len,
				psa_key_id_t *key_id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return psa_to_errno(status);
	}

	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, SRTP_HMAC_ALG(tag_len));

	status = psa_import_key(&attr, key, SRTP_HMAC_SHA1_KEY_LEN, key_id);
	psa_reset_key_attributes(&attr);

	return psa_to_errno(status);
}

int srtp_crypto_cm_crypt(psa_key_id_t key, const uint8_t iv[16], uint8_t *data, size_t len)
{
	psa_cipher_operation_t op = PSA_CIPHER_OPERATION_INIT;
	const size_t bulk = ROUND_DOWN(len, 16);
	const size_t tail = len - bulk;
	psa_status_t status;
	size_t olen = 0;
	size_t total = 0;

	if (len == 0) {
		return 0;
	}

	status = psa_cipher_encrypt_setup(&op, key, PSA_ALG_CTR);
	if (status != PSA_SUCCESS) {
		return psa_to_errno(status);
	}

	status = psa_cipher_set_iv(&op, iv, 16);
	if (status != PSA_SUCCESS) {
		goto abort;
	}

	/* PSA providers need not support in-place updates of partial blocks;
	 * bounce the trailing partial block through a scratch buffer.
	 */
	if (bulk > 0) {
		status = psa_cipher_update(&op, data, bulk, data, bulk, &olen);
		if (status != PSA_SUCCESS || olen != bulk) {
			goto abort;
		}
	}

	if (tail > 0) {
		uint8_t in[16];
		uint8_t out[32];

		memcpy(in, &data[bulk], tail);

		status = psa_cipher_update(&op, in, tail, out, sizeof(out), &olen);
		if (status != PSA_SUCCESS) {
			goto abort;
		}
		total = olen;

		status = psa_cipher_finish(&op, &out[total], sizeof(out) - total, &olen);
		if (status != PSA_SUCCESS) {
			goto abort;
		}
		total += olen;

		if (total != tail) {
			NET_DBG("AES-CM length mismatch (%zu != %zu)", total, tail);
			return -EIO;
		}

		memcpy(&data[bulk], out, tail);
	} else {
		status = psa_cipher_finish(&op, NULL, 0, &olen);
		if (status != PSA_SUCCESS || olen != 0) {
			goto abort;
		}
	}

	return 0;

abort:
	NET_DBG("AES-CM operation failed (%d)", (int)status);
	(void)psa_cipher_abort(&op);
	return psa_to_errno(status);
}

static int auth_setup_update(psa_mac_operation_t *op, psa_key_id_t key, size_t tag_len,
			     const uint8_t *data, size_t len, uint32_t roc, bool sign)
{
	uint8_t roc_be[sizeof(uint32_t)];
	psa_status_t status;

	if (sign) {
		status = psa_mac_sign_setup(op, key, SRTP_HMAC_ALG(tag_len));
	} else {
		status = psa_mac_verify_setup(op, key, SRTP_HMAC_ALG(tag_len));
	}
	if (status != PSA_SUCCESS) {
		return psa_to_errno(status);
	}

	status = psa_mac_update(op, data, len);
	if (status == PSA_SUCCESS) {
		sys_put_be32(roc, roc_be);
		status = psa_mac_update(op, roc_be, sizeof(roc_be));
	}

	if (status != PSA_SUCCESS) {
		(void)psa_mac_abort(op);
		return psa_to_errno(status);
	}

	return 0;
}

int srtp_crypto_auth_compute(psa_key_id_t key, size_t tag_len, const uint8_t *data, size_t len,
			     uint32_t roc, uint8_t *tag)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_status_t status;
	size_t olen = 0;
	int ret;

	ret = auth_setup_update(&op, key, tag_len, data, len, roc, true);
	if (ret < 0) {
		return ret;
	}

	status = psa_mac_sign_finish(&op, tag, tag_len, &olen);
	if (status != PSA_SUCCESS || olen != tag_len) {
		(void)psa_mac_abort(&op);
		return psa_to_errno(status);
	}

	return 0;
}

int srtp_crypto_auth_verify(psa_key_id_t key, size_t tag_len, const uint8_t *data, size_t len,
			    uint32_t roc, const uint8_t *tag)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_status_t status;
	int ret;

	ret = auth_setup_update(&op, key, tag_len, data, len, roc, false);
	if (ret < 0) {
		return ret;
	}

	status = psa_mac_verify_finish(&op, tag, tag_len);
	if (status != PSA_SUCCESS) {
		(void)psa_mac_abort(&op);
		return psa_to_errno(status);
	}

	return 0;
}

int srtp_crypto_gcm_encrypt(psa_key_id_t key, const uint8_t iv[SRTP_AES_GCM_IV_LEN],
			    const uint8_t *aad, size_t aad_len, uint8_t *data, size_t len)
{
	psa_status_t status;
	size_t olen = 0;

	/* In-place operation: PSA requires support for exactly overlapping
	 * input and output buffers; the tag is appended after the ciphertext.
	 */
	status = psa_aead_encrypt(key, PSA_ALG_GCM, iv, SRTP_AES_GCM_IV_LEN, aad, aad_len, data,
				  len, data, len + SRTP_AES_GCM_TAG_LEN, &olen);
	if (status != PSA_SUCCESS || olen != len + SRTP_AES_GCM_TAG_LEN) {
		NET_DBG("AES-GCM encryption failed (%d)", (int)status);
		return psa_to_errno(status);
	}

	return 0;
}

int srtp_crypto_gcm_decrypt(psa_key_id_t key, const uint8_t iv[SRTP_AES_GCM_IV_LEN],
			    const uint8_t *aad, size_t aad_len, uint8_t *data, size_t len)
{
	psa_status_t status;
	size_t olen = 0;

	if (len < SRTP_AES_GCM_TAG_LEN) {
		return -EBADMSG;
	}

	status = psa_aead_decrypt(key, PSA_ALG_GCM, iv, SRTP_AES_GCM_IV_LEN, aad, aad_len, data,
				  len, data, len - SRTP_AES_GCM_TAG_LEN, &olen);
	if (status != PSA_SUCCESS || olen != len - SRTP_AES_GCM_TAG_LEN) {
		NET_DBG("AES-GCM decryption failed (%d)", (int)status);
		return psa_to_errno(status);
	}

	return 0;
}

void srtp_crypto_key_destroy(psa_key_id_t *key_id)
{
	if (*key_id != PSA_KEY_ID_NULL) {
		(void)psa_destroy_key(*key_id);
		*key_id = PSA_KEY_ID_NULL;
	}
}
