/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <psa/crypto.h>

#include <zephyr/bluetooth/mesh.h>

#define LOG_LEVEL CONFIG_BT_MESH_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_crypto_psa);

#include "mesh.h"
#include "crypto.h"
#include "prov.h"

static struct {
	bool is_ready;
	psa_key_id_t priv_key_id;
	uint8_t public_key_be[PUB_KEY_SIZE + 1];
} key;

int bt_mesh_crypto_init(void)
{
	if (psa_crypto_init() != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int bt_mesh_encrypt(const uint8_t key[16], const uint8_t plaintext[16], uint8_t enc_data[16])
{
	psa_key_id_t key_id;
	uint32_t output_len;
	psa_status_t status;
	int err = 0;

	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

	status = psa_import_key(&key_attributes, key, 16, &key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
				    plaintext, 16,
				    enc_data, 16,
				    &output_len);

	if (status != PSA_SUCCESS || output_len != 16) {
		err = -EIO;
	}

	psa_reset_key_attributes(&key_attributes);

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return err;
}

int bt_mesh_ccm_encrypt(const uint8_t key[16], uint8_t nonce[13],
			const uint8_t *plaintext, size_t len, const uint8_t *aad,
			size_t aad_len, uint8_t *enc_data, size_t mic_size)
{
	psa_key_id_t key_id;
	uint32_t output_len;
	psa_status_t status;
	int err = 0;

	psa_algorithm_t alg = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, mic_size);

	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, alg);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

	status = psa_import_key(&key_attributes, key, 16, &key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	status = psa_aead_encrypt(key_id, alg,
				  nonce, 13,
				  aad, aad_len,
				  plaintext, len,
				  enc_data, len + mic_size,
				  &output_len);

	if (status != PSA_SUCCESS || output_len != len + mic_size) {
		err = -EIO;
	}

	psa_reset_key_attributes(&key_attributes);

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return err;
}

int bt_mesh_ccm_decrypt(const uint8_t key[16], uint8_t nonce[13],
			const uint8_t *enc_data, size_t len, const uint8_t *aad,
			size_t aad_len, uint8_t *plaintext, size_t mic_size)
{
	psa_key_id_t key_id;
	uint32_t output_len;
	psa_status_t status;
	int err = 0;

	psa_algorithm_t alg = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, mic_size);

	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, alg);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

	status = psa_import_key(&key_attributes, key, 16, &key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	status = psa_aead_decrypt(key_id, alg,
				  nonce, 13,
				  aad, aad_len,
				  enc_data, len + mic_size,
				  plaintext, len,
				  &output_len);

	if (status != PSA_SUCCESS || output_len != len) {
		err = -EIO;
	}

	psa_reset_key_attributes(&key_attributes);

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return err;
}

int bt_mesh_aes_cmac(const uint8_t key[16], struct bt_mesh_sg *sg,
			size_t sg_len, uint8_t mac[16])
{
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
	psa_algorithm_t alg = PSA_ALG_CMAC;

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;

	psa_status_t status;
	int err = 0;

	/* Import a key */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_CMAC);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);

	status = psa_import_key(&attributes, key, 16, &key_id);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	psa_reset_key_attributes(&attributes);

	status = psa_mac_sign_setup(&operation, key_id, alg);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	for (; sg_len; sg_len--, sg++) {
		status = psa_mac_update(&operation, sg->data, sg->len);
		if (status != PSA_SUCCESS) {
			err = -EIO;
			goto end;
		}
	}

	if (PSA_MAC_LENGTH(PSA_KEY_TYPE_AES, 128, alg) > 16) {
		err = -ERANGE;
		goto end;
	}

	size_t mac_len;

	status = psa_mac_sign_finish(&operation, mac, 16, &mac_len);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	if (mac_len != 16) {
		err = -ERANGE;
	}

end:
	/* Destroy the key */
	psa_destroy_key(key_id);

	return err;
}

int bt_mesh_sha256_hmac(const uint8_t key[32], struct bt_mesh_sg *sg, size_t sg_len,
			uint8_t mac[32])
{
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
	psa_algorithm_t alg = PSA_ALG_HMAC(PSA_ALG_SHA_256);

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;

	psa_status_t status;
	int err = 0;

	/* Import a key */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&attributes, 256);

	status = psa_import_key(&attributes, key, 32, &key_id);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	psa_reset_key_attributes(&attributes);

	status = psa_mac_sign_setup(&operation, key_id, alg);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	for (; sg_len; sg_len--, sg++) {
		status = psa_mac_update(&operation, sg->data, sg->len);
		if (status != PSA_SUCCESS) {
			err = -EIO;
			goto end;
		}
	}

	if (PSA_MAC_LENGTH(PSA_KEY_TYPE_HMAC, 256, alg) > 32) {
		err = -ERANGE;
		goto end;
	}

	size_t mac_len;

	status = psa_mac_sign_finish(&operation, mac, 32, &mac_len);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	if (mac_len != 32) {
		err = -ERANGE;
	}

end:
	/* Destroy the key */
	psa_destroy_key(key_id);

	return err;
}

int bt_mesh_pub_key_gen(void)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	int err = 0;
	size_t key_len;

	psa_destroy_key(key.priv_key_id);
	key.is_ready = false;

	/* Crypto settings for ECDH using the SHA256 hashing algorithm,
	 * the secp256r1 curve
	 */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDH);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	/* Generate a key pair */
	status = psa_generate_key(&key_attributes, &key.priv_key_id);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	status = psa_export_public_key(key.priv_key_id, key.public_key_be,
				sizeof(key.public_key_be), &key_len);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	if (key_len != PUB_KEY_SIZE + 1) {
		err = -ERANGE;
		goto end;
	}

	key.is_ready = true;

end:
	psa_reset_key_attributes(&key_attributes);

	return err;
}

const uint8_t *bt_mesh_pub_key_get(void)
{
	return key.is_ready ? key.public_key_be + 1 : NULL;
}

BUILD_ASSERT(PSA_RAW_KEY_AGREEMENT_OUTPUT_SIZE(
	PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1), 256) == DH_KEY_SIZE,
	"Diffie-Hellman shared secret size should be the same in PSA and BLE Mesh");

BUILD_ASSERT(PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(256) == PUB_KEY_SIZE + 1,
	"Exported PSA public key should be 1 byte larger than BLE Mesh public key");

int bt_mesh_dhkey_gen(const uint8_t *pub_key, const uint8_t *priv_key, uint8_t *dhkey)
{
	int err = 0;
	psa_key_id_t priv_key_id  = PSA_KEY_ID_NULL;
	uint8_t public_key_repr[PUB_KEY_SIZE + 1];
	psa_status_t status;
	size_t dh_key_len;

	if (priv_key) {
		psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

		/* Import a custom private key */
		psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
		psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
		psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
		psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&attributes, 256);

		status = psa_import_key(&attributes, priv_key, PRIV_KEY_SIZE, &priv_key_id);
		if (status != PSA_SUCCESS) {
			err = -EIO;
			goto end;
		}

		psa_reset_key_attributes(&attributes);
	} else {
		priv_key_id = key.priv_key_id;
	}

	/* For elliptic curve key pairs for Weierstrass curve families (PSA_ECC_FAMILY_SECP_R1)
	 *  the representations of public key is:
	 *  - The byte 0x04;
	 *  - x_P as a ceiling(m/8)-byte string, big-endian;
	 *  - y_P as a ceiling(m/8)-byte string, big-endian.
	 */
	public_key_repr[0] = 0x04;
	memcpy(public_key_repr + 1, pub_key, PUB_KEY_SIZE);

	/* Calculate the secret */
	status = psa_raw_key_agreement(PSA_ALG_ECDH, priv_key_id, public_key_repr,
			PUB_KEY_SIZE + 1, dhkey, DH_KEY_SIZE, &dh_key_len);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto end;
	}

	if (dh_key_len != DH_KEY_SIZE) {
		err = -ERANGE;
	}

end:

	if (priv_key) {
		psa_destroy_key(priv_key_id);
	}

	return err;
}
