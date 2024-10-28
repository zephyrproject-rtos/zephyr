/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <psa/crypto.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(persistent_key);

#define SAMPLE_KEY_ID   PSA_KEY_ID_USER_MIN
#define SAMPLE_KEY_TYPE PSA_KEY_TYPE_AES
#define SAMPLE_ALG      PSA_ALG_CTR
#define SAMPLE_KEY_BITS 256

int generate_persistent_key(void)
{
	LOG_INF("Generating a persistent key...");
	psa_status_t ret;
	psa_key_id_t key_id;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_id(&key_attributes, SAMPLE_KEY_ID);
	psa_set_key_type(&key_attributes, SAMPLE_KEY_TYPE);
	psa_set_key_algorithm(&key_attributes, SAMPLE_ALG);
	psa_set_key_bits(&key_attributes, SAMPLE_KEY_BITS);

	ret = psa_generate_key(&key_attributes, &key_id);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to generate the key. (%d)", ret);
		return -1;
	}
	__ASSERT_NO_MSG(key_id == SAMPLE_KEY_ID);

	/* Purge the key from volatile memory. Has the same affect than resetting the device. */
	ret = psa_purge_key(SAMPLE_KEY_ID);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to purge the generated key from volatile memory. (%d).", ret);
		return -1;
	}

	LOG_INF("Persistent key generated.");
	return 0;
}

int use_persistent_key(void)
{
	LOG_INF("Using the persistent key to encrypt and decrypt some plaintext...");
	psa_status_t ret;
	size_t ciphertext_len;
	size_t decrypted_text_len;

	static uint8_t plaintext[] =
		"Example plaintext to demonstrate basic usage of a persistent key.";
	static uint8_t ciphertext[PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(SAMPLE_KEY_TYPE, SAMPLE_ALG,
								 sizeof(plaintext))];
	static uint8_t decrypted_text[sizeof(plaintext)];

	ret = psa_cipher_encrypt(SAMPLE_KEY_ID, SAMPLE_ALG, plaintext, sizeof(plaintext),
				 ciphertext, sizeof(ciphertext), &ciphertext_len);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to encrypt the plaintext. (%d)", ret);
		return -1;
	}

	ret = psa_cipher_decrypt(SAMPLE_KEY_ID, SAMPLE_ALG, ciphertext, ciphertext_len,
				 decrypted_text, sizeof(decrypted_text), &decrypted_text_len);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to decrypt the ciphertext. (%d)", ret);
		return -1;
	}
	__ASSERT_NO_MSG(decrypted_text_len == sizeof(plaintext));

	/* Make sure the decryption gives us the original plaintext back. */
	if (memcmp(plaintext, decrypted_text, sizeof(plaintext))) {
		LOG_HEXDUMP_INF(plaintext, sizeof(plaintext), "Plaintext:");
		LOG_HEXDUMP_INF(ciphertext, ciphertext_len, "Ciphertext:");
		LOG_HEXDUMP_INF(decrypted_text, sizeof(decrypted_text), "Decrypted text:");
		LOG_ERR("The decrypted text doesn't match the plaintext.");
		return -1;
	}

	LOG_INF("Persistent key usage successful.");
	return 0;
}

static int destroy_persistent_key(void)
{
	LOG_INF("Destroying the persistent key...");
	psa_status_t ret;

	ret = psa_destroy_key(SAMPLE_KEY_ID);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy the key. (%d)", ret);
		return -1;
	}

	LOG_INF("Persistent key destroyed.");
	return 0;
}

int main(void)
{
	LOG_INF("Persistent key sample started.");

	/* Ensure there is not already a key with this ID. */
	psa_destroy_key(SAMPLE_KEY_ID);

	if (generate_persistent_key()) {
		return -1;
	}

	if (use_persistent_key()) {
		return -1;
	}

	if (destroy_persistent_key()) {
		return -1;
	}

	LOG_INF("Sample finished successfully.");
	return 0;
}
