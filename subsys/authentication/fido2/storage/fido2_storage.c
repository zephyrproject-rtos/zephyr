/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2_types.h>
#include <zephyr/authentication/fido2/fido2_storage.h>
#include <psa/crypto.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

static int destroy_credential_key(uint32_t key_id)
{
	psa_status_t status;

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy credential key 0x%08x: %d", (unsigned int)key_id,
			status);
		return -EIO;
	}

	return 0;
}

int fido2_storage_init(void)
{
	return fido2_storage_backend.init();
}

int fido2_storage_store(const struct fido2_credential *cred)
{
	return fido2_storage_backend.store(cred);
}

int fido2_storage_load(const uint8_t *cred_id, size_t cred_id_len, struct fido2_credential *cred)
{
	return fido2_storage_backend.load(cred_id, cred_id_len, cred);
}

int fido2_storage_remove(const uint8_t *cred_id, size_t cred_id_len)
{
	struct fido2_credential cred;
	int ret;

	ret = fido2_storage_backend.remove(cred_id, cred_id_len, &cred);
	if (ret) {
		return ret;
	}

	ret = destroy_credential_key(cred.key_id);

	return ret;
}

int fido2_storage_find_by_rp(const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
			     struct fido2_credential *creds, size_t max_creds, size_t *count)
{
	return fido2_storage_backend.find_by_rp(rp_id_hash, creds, max_creds, count);
}

int fido2_storage_sign_count_increment(const uint8_t *cred_id, size_t cred_id_len,
				       uint32_t *new_count)
{
	return fido2_storage_backend.sign_count_increment(cred_id, cred_id_len, new_count);
}
