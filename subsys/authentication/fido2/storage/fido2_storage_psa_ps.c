/*
 * Copyright (c) 2026 Siratul Islam <siratul.islam@linux.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2_types.h>
#include <zephyr/authentication/fido2/fido2_storage.h>
#include <zephyr/psa/ps_ids.h>
#include <psa/protected_storage.h>
#include <psa/error.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define FIDO2_PS_UID_BASE        ZEPHYR_PSA_FIDO2_PS_UID_RANGE_BEGIN
#define FIDO2_PS_UID_PIN_RETRIES (FIDO2_PS_UID_BASE + 0x00U)
#define FIDO2_PS_UID_PIN_HASH    (FIDO2_PS_UID_BASE + 0x01U)
#define FIDO2_PS_CREDENTIAL_BASE (FIDO2_PS_UID_BASE + 0x100U)

#if defined(CONFIG_SECURE_STORAGE)
BUILD_ASSERT(sizeof(struct fido2_credential) <= CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE,
	     "FIDO2 credential exceeds ITS max data size");
#endif

static int ps_save_one(psa_storage_uid_t uid, const void *data, size_t len)
{
	psa_status_t status;

	status = psa_ps_set(uid, len, data, PSA_STORAGE_FLAG_NONE);
	if (status == PSA_ERROR_INSUFFICIENT_STORAGE) {
		return -ENOSPC;
	}
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int cred_slot_get(const uint8_t *cred_id, size_t cred_id_len, struct fido2_credential *out)
{
	struct fido2_credential cred;
	psa_status_t status;

	if (cred_id == NULL || cred_id_len > sizeof(cred.id)) {
		return -EINVAL;
	}

	for (int i = 0; i < CONFIG_FIDO2_MAX_CREDENTIALS; ++i) {
		size_t cred_len;

		status =
			psa_ps_get(FIDO2_PS_CREDENTIAL_BASE + i, 0, sizeof(cred), &cred, &cred_len);
		if (status == PSA_ERROR_DOES_NOT_EXIST) {
			continue;
		}
		if (status != PSA_SUCCESS) {
			return -EIO;
		}

		if (cred_len != sizeof(cred)) {
			return -EIO;
		}
		if (cred.id_len == cred_id_len && memcmp(cred.id, cred_id, cred_id_len) == 0) {
			if (out != NULL) {
				memcpy(out, &cred, sizeof(cred));
			}
			return i;
		}
	}

	return -ENOENT;
}

static int psa_ps_backend_init(void)
{
	LOG_INF("Credential storage: PSA Protected Storage");
	return 0;
}

static int psa_ps_backend_store(const struct fido2_credential *cred)
{
	struct psa_storage_info_t info;
	psa_status_t status;

	for (int i = 0; i < CONFIG_FIDO2_MAX_CREDENTIALS; ++i) {
		status = psa_ps_get_info(FIDO2_PS_CREDENTIAL_BASE + i, &info);
		if (status == PSA_SUCCESS) {
			continue; /* slot occupied */
		}
		if (status != PSA_ERROR_DOES_NOT_EXIST) {
			return -EIO;
		}

		return ps_save_one(FIDO2_PS_CREDENTIAL_BASE + i, cred, sizeof(*cred));
	}

	return -ENOSPC;
}

static int psa_ps_backend_load(const uint8_t *cred_id, size_t cred_id_len,
			       struct fido2_credential *cred)
{
	int idx;

	idx = cred_slot_get(cred_id, cred_id_len, cred);
	if (idx < 0) {
		return idx;
	}

	return 0;
}

static int psa_ps_backend_remove(const uint8_t *cred_id, size_t cred_id_len,
				 struct fido2_credential *cred)
{
	psa_status_t status;
	int idx;

	idx = cred_slot_get(cred_id, cred_id_len, cred);
	if (idx < 0) {
		return idx;
	}

	status = psa_ps_remove(FIDO2_PS_CREDENTIAL_BASE + idx);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int psa_ps_backend_find_by_rp(const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				     struct fido2_credential *creds, size_t max_creds,
				     size_t *count)
{
	struct fido2_credential cred;
	psa_status_t status;

	*count = 0;

	for (int i = 0; i < CONFIG_FIDO2_MAX_CREDENTIALS; ++i) {
		size_t cred_len;

		status =
			psa_ps_get(FIDO2_PS_CREDENTIAL_BASE + i, 0, sizeof(cred), &cred, &cred_len);
		if (status == PSA_ERROR_DOES_NOT_EXIST) {
			continue;
		}
		if (status != PSA_SUCCESS) {
			return -EIO;
		}

		if (cred_len != sizeof(cred)) {
			return -EIO;
		}
		if (memcmp(cred.rp_id_hash, rp_id_hash, FIDO2_SHA256_SIZE) == 0) {
			if (*count < max_creds) {
				memcpy(creds + *count, &cred, sizeof(cred));
			}
			(*count)++;
		}
	}

	return 0;
}

static int psa_ps_backend_sign_count_increment(const uint8_t *cred_id, size_t cred_id_len,
					       uint32_t *new_count)
{
	struct fido2_credential cred;
	int idx;

	idx = cred_slot_get(cred_id, cred_id_len, &cred);
	if (idx < 0) {
		return idx;
	}

	*new_count = ++cred.sign_count;

	return ps_save_one(FIDO2_PS_CREDENTIAL_BASE + idx, &cred, sizeof(cred));
}

static int psa_ps_backend_pin_retries_get(uint8_t *retries)
{
	struct psa_storage_info_t info;
	psa_status_t status;
	size_t len;

	status = psa_ps_get(FIDO2_PS_UID_PIN_RETRIES, 0, sizeof(*retries), retries, &len);
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		status = psa_ps_get_info(FIDO2_PS_UID_PIN_HASH, &info);
		if (status == PSA_ERROR_DOES_NOT_EXIST) { /* Reset retries if it's a fresh flash */
			*retries = CONFIG_FIDO2_PIN_MAX_RETRIES;
			return 0;
		}
		return -EIO;
	}
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	if (len != sizeof(*retries)) {
		return -EIO;
	}

	return 0;
}

static int psa_ps_backend_pin_retries_decrement(void)
{
	uint8_t retries;
	int ret;

	ret = psa_ps_backend_pin_retries_get(&retries);
	if (ret) {
		return ret;
	}

	if (retries > 0) {
		--retries;
	}

	return ps_save_one(FIDO2_PS_UID_PIN_RETRIES, &retries, sizeof(retries));
}

static int psa_ps_backend_pin_retries_reset(void)
{
	uint8_t retries = CONFIG_FIDO2_PIN_MAX_RETRIES;

	return ps_save_one(FIDO2_PS_UID_PIN_RETRIES, &retries, sizeof(retries));
}

static int psa_ps_backend_pin_set(const uint8_t *pin_hash)
{
	return ps_save_one(FIDO2_PS_UID_PIN_HASH, pin_hash, FIDO2_PIN_HASH_SIZE);
}

static int psa_ps_backend_pin_get(uint8_t *pin_hash)
{
	psa_status_t status;
	size_t len;

	status = psa_ps_get(FIDO2_PS_UID_PIN_HASH, 0, FIDO2_PIN_HASH_SIZE, pin_hash, &len);
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		return -ENOENT;
	}
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	if (len != FIDO2_PIN_HASH_SIZE) {
		return -EIO;
	}

	return 0;
}

const struct fido2_storage_api fido2_storage_backend = {
	.init = psa_ps_backend_init,
	.store = psa_ps_backend_store,
	.load = psa_ps_backend_load,
	.remove = psa_ps_backend_remove,
	.find_by_rp = psa_ps_backend_find_by_rp,
	.sign_count_increment = psa_ps_backend_sign_count_increment,
	.pin_retries_get = psa_ps_backend_pin_retries_get,
	.pin_retries_decrement = psa_ps_backend_pin_retries_decrement,
	.pin_retries_reset = psa_ps_backend_pin_retries_reset,
	.pin_set = psa_ps_backend_pin_set,
	.pin_get = psa_ps_backend_pin_get,
};
