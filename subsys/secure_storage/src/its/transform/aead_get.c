/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/transform/aead_get.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>
#include <string.h>
#include <sys/types.h>

LOG_MODULE_DECLARE(secure_storage, CONFIG_SECURE_STORAGE_LOG_LEVEL);

#ifdef CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_AES_GCM
#define PSA_KEY_TYPE PSA_KEY_TYPE_AES
#define PSA_ALG PSA_ALG_GCM
#elif defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_CHACHA20_POLY1305)
#define PSA_KEY_TYPE PSA_KEY_TYPE_CHACHA20
#define PSA_ALG PSA_ALG_CHACHA20_POLY1305
#endif
#ifndef CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_CUSTOM
void secure_storage_its_transform_aead_get_scheme(psa_key_type_t *key_type, psa_algorithm_t *alg)
{
	*key_type = PSA_KEY_TYPE;
	*alg = PSA_ALG;
}
#endif /* !CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_CUSTOM */

#ifndef CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_CUSTOM

#define SHA256_OUTPUT_SIZE 32
BUILD_ASSERT(SHA256_OUTPUT_SIZE == PSA_HASH_LENGTH(PSA_ALG_SHA_256));
BUILD_ASSERT(SHA256_OUTPUT_SIZE >= CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE);

static psa_status_t hash_data_into_key(
		size_t data_len, const void *data,
		uint8_t key[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE])
{
	size_t hash_len;

#if CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE == SHA256_OUTPUT_SIZE
	/* Save stack usage and avoid unnecessary memory operations.*/
	return psa_hash_compute(PSA_ALG_SHA_256, data, data_len, key,
				CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE, &hash_len);
#else
	uint8_t hash_output[SHA256_OUTPUT_SIZE];
	const psa_status_t ret = psa_hash_compute(PSA_ALG_SHA_256, data, data_len, hash_output,
						  sizeof(hash_output), &hash_len);

	if (ret == PSA_SUCCESS) {
		memcpy(key, hash_output, CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE);
		mbedtls_platform_zeroize(hash_output, sizeof(hash_output));
	}
	return ret;
#endif
}

#ifdef CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_DEVICE_ID_HASH

#define WARNING "Using a potentially insecure PSA ITS encryption key provider."

psa_status_t secure_storage_its_transform_aead_get_key(
		secure_storage_its_uid_t uid,
		uint8_t key[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE])
{
	psa_status_t ret;
	ssize_t hwinfo_ret;
	struct {
		uint8_t device_id[8];
		secure_storage_its_uid_t uid; /* acts as a salt */
	} __packed data;

	hwinfo_ret = hwinfo_get_device_eui64(data.device_id);
	if (hwinfo_ret != 0) {
		hwinfo_ret = hwinfo_get_device_id(data.device_id, sizeof(data.device_id));
		if (hwinfo_ret <= 0) {
			return PSA_ERROR_HARDWARE_FAILURE;
		}
		if (hwinfo_ret < sizeof(data.device_id)) {
			memset(data.device_id + hwinfo_ret, 0, sizeof(data.device_id) - hwinfo_ret);
		}
	}
	data.uid = uid;
	ret = hash_data_into_key(sizeof(data), &data, key);

	mbedtls_platform_zeroize(data.device_id, sizeof(data.device_id));
	return ret;
}

#elif defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_ENTRY_UID_HASH)

#define WARNING "Using an insecure PSA ITS encryption key provider."

psa_status_t secure_storage_its_transform_aead_get_key(
		secure_storage_its_uid_t uid,
		uint8_t key[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE])
{
	return hash_data_into_key(sizeof(uid), &uid, key);
}

#endif /* CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER */

#ifndef CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NO_INSECURE_KEY_WARNING

static int warn_insecure_key(void)
{
	printk("WARNING: %s\n", WARNING);
	LOG_WRN("%s", WARNING);
	return 0;
}
SYS_INIT(warn_insecure_key, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* !CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NO_INSECURE_KEY_WARNING */

#endif /* !CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_CUSTOM */

#ifdef CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_PROVIDER_DEFAULT

psa_status_t secure_storage_its_transform_aead_get_nonce(
		uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE])
{
	psa_status_t ret;
	static uint8_t s_nonce[CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE];
	static bool s_nonce_initialized;

	if (!s_nonce_initialized) {
		ret = psa_generate_random(s_nonce, sizeof(s_nonce));
		if (ret != PSA_SUCCESS) {
			return ret;
		}
		s_nonce_initialized = true;
	} else {
		for (unsigned int i = 0; i != sizeof(s_nonce); ++i) {
			++s_nonce[i];
			if (s_nonce[i] != 0) {
				break;
			}
		}
	}

	memcpy(nonce, &s_nonce, sizeof(s_nonce));
	return PSA_SUCCESS;
}
#endif /* CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_PROVIDER_DEFAULT */
