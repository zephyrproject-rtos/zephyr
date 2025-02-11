/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/ead.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/check.h>

#include <zephyr/logging/log.h>

/** nonce size in bytes */
#define BT_EAD_NONCE_SIZE 13

/* This value is used to set the directionBit of the CCM  nonce to the MSB of the Randomizer field
 * (see Supplement to the Bluetooth Core   Specification v11, Part A 1.23.3)
 */
#define BT_EAD_RANDOMIZER_DIRECTION_BIT 7

/** Additional Authenticated Data size in bytes */
#define BT_EAD_AAD_SIZE 1

/* Fixed value used for the Additional Authenticated Data (see Supplement to the Bluetooth Core
 * Specification v11, Part A 1.23.3)
 */
static uint8_t bt_ead_aad[] = {0xEA};
BUILD_ASSERT(sizeof(bt_ead_aad) == BT_EAD_AAD_SIZE);

LOG_MODULE_REGISTER(bt_encrypted_ad_data, CONFIG_BT_EAD_LOG_LEVEL);

static int bt_ead_generate_randomizer(uint8_t randomizer[BT_EAD_RANDOMIZER_SIZE])
{
	int err;

	err = bt_rand(randomizer, BT_EAD_RANDOMIZER_SIZE);

	if (err != 0) {
		return -ECANCELED;
	}

	/* From Supplement to the Bluetooth Core Specification v11, Part A 1.23.3: The directionBit
	 * of the CCM nonce shall be set to the most significant bit of the Randomizer field.
	 */
	randomizer[4] |= 1 << BT_EAD_RANDOMIZER_DIRECTION_BIT;

	return 0;
}

static int bt_ead_generate_nonce(const uint8_t iv[BT_EAD_IV_SIZE],
				 const uint8_t randomizer[BT_EAD_RANDOMIZER_SIZE], uint8_t *nonce)
{
	uint8_t new_randomizer[BT_EAD_RANDOMIZER_SIZE];

	if (randomizer == NULL) {
		int err;

		err = bt_ead_generate_randomizer(new_randomizer);

		if (err != 0) {
			LOG_DBG("Failed to generate Randomizer");
			return -ECANCELED;
		}

		randomizer = new_randomizer;
	}

	memcpy(&nonce[0], randomizer, BT_EAD_RANDOMIZER_SIZE);
	memcpy(&nonce[BT_EAD_RANDOMIZER_SIZE], iv, BT_EAD_IV_SIZE);

	return 0;
}

static int ead_encrypt(const uint8_t session_key[BT_EAD_KEY_SIZE], const uint8_t iv[BT_EAD_IV_SIZE],
		       const uint8_t randomizer[BT_EAD_RANDOMIZER_SIZE], const uint8_t *payload,
		       size_t payload_size, uint8_t *encrypted_payload)
{
	int err;
	uint8_t nonce[BT_EAD_NONCE_SIZE];
	size_t ead_size = BT_EAD_RANDOMIZER_SIZE + payload_size + BT_EAD_MIC_SIZE;

	err = bt_ead_generate_nonce(iv, randomizer, nonce);
	if (err != 0) {
		return -ECANCELED;
	}

	memcpy(encrypted_payload, nonce, BT_EAD_RANDOMIZER_SIZE);

	err = bt_ccm_encrypt(session_key, nonce, payload, payload_size, bt_ead_aad, BT_EAD_AAD_SIZE,
			     &encrypted_payload[BT_EAD_RANDOMIZER_SIZE], BT_EAD_MIC_SIZE);
	if (err != 0) {
		LOG_DBG("Failed to encrypt the payload (bt_ccm_encrypt err %d)", err);
		return -EIO;
	}

	LOG_HEXDUMP_DBG(encrypted_payload, ead_size, "Encrypted Data: ");

	return 0;
}

int bt_ead_encrypt(const uint8_t session_key[BT_EAD_KEY_SIZE], const uint8_t iv[BT_EAD_IV_SIZE],
		   const uint8_t *payload, size_t payload_size, uint8_t *encrypted_payload)
{
	CHECKIF(session_key == NULL) {
		LOG_DBG("session_key is NULL");
		return -EINVAL;
	}

	CHECKIF(iv == NULL) {
		LOG_DBG("iv is NULL");
		return -EINVAL;
	}

	CHECKIF(payload == NULL) {
		LOG_DBG("payload is NULL");
		return -EINVAL;
	}

	CHECKIF(encrypted_payload == NULL) {
		LOG_DBG("encrypted_payload is NULL");
		return -EINVAL;
	}

	if (payload_size == 0) {
		LOG_WRN("payload_size is set to 0. The encrypted result will only contain the "
			"Randomizer and the MIC.");
	}

	return ead_encrypt(session_key, iv, NULL, payload, payload_size, encrypted_payload);
}

#if defined(CONFIG_BT_TESTING)

int bt_test_ead_encrypt(const uint8_t session_key[BT_EAD_KEY_SIZE],
			const uint8_t iv[BT_EAD_IV_SIZE],
			const uint8_t randomizer[BT_EAD_RANDOMIZER_SIZE], const uint8_t *payload,
			size_t payload_size, uint8_t *encrypted_payload)
{
	CHECKIF(session_key == NULL) {
		LOG_DBG("session_key is NULL");
		return -EINVAL;
	}

	CHECKIF(iv == NULL) {
		LOG_DBG("iv is NULL");
		return -EINVAL;
	}

	CHECKIF(randomizer == NULL) {
		LOG_DBG("randomizer is NULL");
		return -EINVAL;
	}

	CHECKIF(payload == NULL) {
		LOG_DBG("payload is NULL");
		return -EINVAL;
	}

	CHECKIF(encrypted_payload == NULL) {
		LOG_DBG("encrypted_payload is NULL");
		return -EINVAL;
	}

	if (payload_size == 0) {
		LOG_WRN("payload_size is set to 0. The encrypted result will be filled with only "
			"the Randomizer and the MIC.");
	}

	return ead_encrypt(session_key, iv, randomizer, payload, payload_size, encrypted_payload);
}

#endif /* CONFIG_BT_TESTING */

static int ead_decrypt(const uint8_t session_key[BT_EAD_KEY_SIZE], const uint8_t iv[BT_EAD_IV_SIZE],
		       const uint8_t *encrypted_payload, size_t encrypted_payload_size,
		       uint8_t *payload)
{
	int err;
	uint8_t nonce[BT_EAD_NONCE_SIZE];
	const uint8_t *encrypted_ad_data = &encrypted_payload[BT_EAD_RANDOMIZER_SIZE];
	size_t encrypted_ad_data_size = encrypted_payload_size - BT_EAD_RANDOMIZER_SIZE;
	size_t payload_size = encrypted_ad_data_size - BT_EAD_MIC_SIZE;

	const uint8_t *randomizer = encrypted_payload;

	err = bt_ead_generate_nonce(iv, randomizer, nonce);
	if (err != 0) {
		return -EIO;
	}

	LOG_HEXDUMP_DBG(encrypted_ad_data, encrypted_ad_data_size, "Encrypted Data: ");

	err = bt_ccm_decrypt(session_key, nonce, encrypted_ad_data, payload_size, bt_ead_aad,
			     BT_EAD_AAD_SIZE, payload, BT_EAD_MIC_SIZE);
	LOG_HEXDUMP_DBG(payload, payload_size, "Decrypted Data: ");
	if (err != 0) {
		LOG_DBG("Failed to decrypt the data (bt_ccm_decrypt err %d)", err);
		return -EIO;
	}

	return 0;
}

int bt_ead_decrypt(const uint8_t session_key[BT_EAD_KEY_SIZE], const uint8_t iv[BT_EAD_IV_SIZE],
		   const uint8_t *encrypted_payload, size_t encrypted_payload_size,
		   uint8_t *payload)
{
	CHECKIF(session_key == NULL) {
		LOG_DBG("session_key is NULL");
		return -EINVAL;
	}

	CHECKIF(iv == NULL) {
		LOG_DBG("iv is NULL");
		return -EINVAL;
	}

	CHECKIF(encrypted_payload == NULL) {
		LOG_DBG("encrypted_payload is NULL");
		return -EINVAL;
	}

	CHECKIF(payload == NULL) {
		LOG_DBG("payload is NULL");
		return -EINVAL;
	}

	if (encrypted_payload_size < BT_EAD_RANDOMIZER_SIZE + BT_EAD_MIC_SIZE) {
		LOG_DBG("encrypted_payload_size is not large enough.");
		return -EINVAL;
	} else if (encrypted_payload_size == BT_EAD_RANDOMIZER_SIZE + BT_EAD_MIC_SIZE) {
		LOG_WRN("encrypted_payload_size not large enough to contain encrypted data.");
	}

	return ead_decrypt(session_key, iv, encrypted_payload, encrypted_payload_size, payload);
}
