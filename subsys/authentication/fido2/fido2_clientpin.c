/*
 * Copyright (c) 2026 Siratul Islam <siratul.islam@linux.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2_storage.h>
#include <string.h>

#include "fido2_cbor.h"
#include "fido2_crypto.h"
#include "fido2_clientpin.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

static uint32_t key_agreement_key_id;
static uint8_t shared_secret[FIDO2_SHA256_SIZE];

static int generate_key_agreement(void)
{
	int ret;

	if (key_agreement_key_id != 0) {
		ret = fido2_crypto_destroy_key(key_agreement_key_id);
		if (ret) {
			return ret;
		}
	}

	ret = fido2_crypto_ecdh_generate_keypair(&key_agreement_key_id);
	if (ret) {
		LOG_ERR("KeyAgreement key gen failed: %d", ret);
		return ret;
	}

	return 0;
}

static int decapsulate(const uint8_t *platform_key, size_t platform_key_len, uint8_t protocol)
{
	uint8_t z[FIDO2_SHA256_SIZE];
	size_t z_len;
	int ret;

	ret = fido2_crypto_ecdh_agree(key_agreement_key_id, platform_key, platform_key_len, z,
				      &z_len);
	if (ret) {
		LOG_ERR("ECDH agreement failed: %d", ret);
		return ret;
	}

	if (protocol == FIDO2_PIN_PROTOCOL_V1) {
		ret = fido2_crypto_sha256(z, z_len, shared_secret);
		if (ret) {
			return ret;
		}
	} else if (protocol == FIDO2_PIN_PROTOCOL_V2) {
		/* TODO: proto 2 */
		return -EINVAL;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int verify(uint8_t protocol, const uint8_t *msg, size_t msg_len,
		  const uint8_t *pin_uv_auth_param, size_t pin_uv_auth_param_len)
{
	const uint8_t *key = shared_secret; /* Indirection as key differs for protocol 2 */
	uint8_t mac[FIDO2_SHA256_SIZE];
	int ret;

	ret = fido2_crypto_hmac_sha256(key, FIDO2_SHA256_SIZE, msg, msg_len, mac);
	if (ret) {
		return ret;
	}

	if (memcmp(mac, pin_uv_auth_param, pin_uv_auth_param_len) != 0) {
		return -EACCES;
	}

	return 0;
}

static int decrypt(uint8_t protocol, const uint8_t *ciphertext, size_t ciphertext_len,
		   uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len)
{
	const uint8_t *key = shared_secret;
	uint8_t iv[16];

	if (protocol == FIDO2_PIN_PROTOCOL_V1) {
		if (ciphertext_len == 0 || ciphertext_len % 16 != 0 ||
		    ciphertext_len > plaintext_size) {
			return -EINVAL;
		}
	} else if (FIDO2_PIN_PROTOCOL_V2) {
		/* TODO: proto 2 */
		return -EINVAL;
	}

	return fido2_crypto_aes256_cbc_decrypt(key, iv, ciphertext, ciphertext_len, plaintext,
					       plaintext_size, plaintext_len);
}

int fido2_clientpin_init(void)
{
	return generate_key_agreement();
}

int fido2_clientpin_pin_is_set(void)
{
	uint8_t hash[FIDO2_PIN_HASH_SIZE];

	return (fido2_storage_pin_get(hash) == 0);
}

/* authenticatorClientPIN subCommands */
enum fido2_status fido2_clientpin_cmd_get_retries(uint8_t *cbor_out, size_t cbor_out_cap,
						  size_t *cbor_out_len)
{
	uint8_t retries;
	int ret;

	ret = fido2_storage_pin_retries_get(&retries);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_cbor_encode_client_pin_retries(retries, false, cbor_out, cbor_out_cap,
						   cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	return FIDO2_OK;
}

enum fido2_status fido2_clientpin_cmd_get_key_agreement(uint8_t *cbor_out, size_t cbor_out_cap,
							size_t *cbor_out_len)
{
	uint8_t pub_key[FIDO2_P256_UNCOMPRESSED_KEY_SIZE];
	size_t pub_key_len;
	int ret;

	ret = fido2_crypto_export_pubkey(key_agreement_key_id, pub_key, sizeof(pub_key),
					 &pub_key_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_cbor_encode_client_pin_key_agreement(pub_key, pub_key_len, cbor_out,
							 cbor_out_cap, cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	return FIDO2_OK;
}

enum fido2_status fido2_clientpin_cmd_set_pin(uint8_t protocol, const uint8_t *platform_key,
					      size_t platform_key_len, const uint8_t *new_pin_enc,
					      size_t new_pin_enc_len,
					      const uint8_t *pin_uv_auth_param)
{
	uint8_t padded_pin[FIDO2_PIN_PADDED_SIZE];
	size_t padded_pin_len;
	size_t new_pin_len;
	uint8_t new_pin_hash[FIDO2_SHA256_SIZE];
	int ret;

	if (fido2_clientpin_pin_is_set()) {
		return FIDO2_ERR_PIN_AUTH_INVALID;
	}

	ret = decapsulate(platform_key, platform_key_len, protocol);
	if (ret) {
		return FIDO2_ERR_INVALID_PARAMETER;
	}

	ret = verify(protocol, new_pin_enc, new_pin_enc_len, pin_uv_auth_param,
		     protocol == FIDO2_PIN_PROTOCOL_V1 ? FIDO2_PIN_AUTH_SIZE_P1
						       : FIDO2_PIN_AUTH_SIZE_P2);
	if (ret) {
		return FIDO2_ERR_PIN_AUTH_INVALID;
	}

	ret = decrypt(protocol, new_pin_enc, new_pin_enc_len, padded_pin, sizeof(padded_pin),
		      &padded_pin_len);
	if (ret) {
		return FIDO2_ERR_PIN_AUTH_INVALID;
	}

	if (padded_pin_len != FIDO2_PIN_PADDED_SIZE) {
		return FIDO2_ERR_INVALID_PARAMETER;
	}

	/* Remove trailng 0s */
	new_pin_len = strnlen((const char *)padded_pin, FIDO2_PIN_PADDED_SIZE);
	if (new_pin_len < CONFIG_FIDO2_MIN_PIN_LENGTH) {
		return FIDO2_ERR_PIN_POLICY_VIOLATION;
	}

	ret = fido2_crypto_sha256(padded_pin, new_pin_len, new_pin_hash);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_storage_pin_set(new_pin_hash);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_storage_pin_retries_reset();
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	return FIDO2_OK;
}
