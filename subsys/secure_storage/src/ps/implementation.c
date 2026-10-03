/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/uid.h>
#include <zephyr/secure_storage/its.h>
#include <zephyr/secure_storage/ps.h>
#include <zephyr/secure_storage/ps/store.h>
#include <zephyr/secure_storage/ps/transform.h>
#include <zephyr/secure_storage/ps/replay_protection.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <string.h>

LOG_MODULE_REGISTER(secure_storage_ps, CONFIG_SECURE_STORAGE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE <=
	     CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE);

static void log_failed_operation(const char *operation, const char *preposition, psa_status_t ret)
{
	LOG_ERR("Failed to %s data %s storage. (%d)", operation, preposition, ret);
}

static psa_status_t get_stored_data(
		secure_storage_uid_t uid,
		uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len)
{
	psa_status_t ret;

	ret = secure_storage_ps_store_get(uid, SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE,
					   stored_data, stored_data_len);
	if (ret != PSA_SUCCESS) {
		if (ret != PSA_ERROR_DOES_NOT_EXIST) {
			log_failed_operation("retrieve", "from", ret);
		}
	}
	return ret;
}

static psa_status_t transform_stored_data(
		secure_storage_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t data_size, void *data, size_t *data_len,
		psa_storage_create_flags_t *create_flags)
{
	psa_status_t ret;

	ret = secure_storage_ps_transform_from_store(uid, stored_data_len, stored_data,
						      data_size, data, data_len, create_flags);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("transform", "from", ret);
		if (ret == PSA_ERROR_INVALID_SIGNATURE || ret == PSA_ERROR_DATA_CORRUPT) {
			return ret;
		}
		return PSA_ERROR_GENERIC_ERROR;
	}
	return PSA_SUCCESS;
}

static psa_status_t check_replay_protection(
		psa_storage_uid_t uid,
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t stored_data_len)
{
	uint8_t its_rp[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
	size_t its_rp_len;
	uint8_t ps_rp[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
	size_t ps_rp_len;
	psa_status_t ret;

	ret = secure_storage_its_get(SECURE_STORAGE_CALLER_PSA_PS, uid, 0,
				     sizeof(its_rp), its_rp,
				     &its_rp_len);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to retrieve replay protection value for entry " PSA_UID_FMT,
			PSA_UID_ARGS(uid));
		/* When this function is called the data from PS has already been read, so
		 * the entry exists there. If for some reason there is no corresponding field
		 * in ITS then the correct error is PSA_ERROR_INVALID_SIGNATURE.
		 */
		if (ret == PSA_ERROR_DOES_NOT_EXIST) {
			return PSA_ERROR_INVALID_SIGNATURE;
		}
		if (ret == PSA_ERROR_STORAGE_FAILURE ||
		    ret == PSA_ERROR_INVALID_SIGNATURE ||
		    ret == PSA_ERROR_DATA_CORRUPT) {
			return ret;
		}
		return PSA_ERROR_GENERIC_ERROR;
	}

	ret = secure_storage_ps_get_replay_protection(stored_data, stored_data_len,
						      ps_rp, &ps_rp_len);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to compute replay protection value for entry " PSA_UID_FMT,
			PSA_UID_ARGS(uid));
		return PSA_ERROR_GENERIC_ERROR;
	}

	if ((its_rp_len != ps_rp_len) ||
	    (memcmp(its_rp, ps_rp, ps_rp_len) != 0)) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}
	return PSA_SUCCESS;
}

static psa_status_t get_entry(secure_storage_uid_t uid, size_t data_size, uint8_t *data,
			      size_t *data_len, psa_storage_create_flags_t *create_flags)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;

	ret = get_stored_data(uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = check_replay_protection(uid.uid, stored_data, stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	return transform_stored_data(uid, stored_data_len, stored_data, data_size, data, data_len,
				     create_flags);
}

static bool keep_stored_entry(secure_storage_uid_t uid, size_t data_length, const void *p_data,
			      psa_storage_create_flags_t create_flags, psa_status_t *ret)
{
	psa_storage_create_flags_t existing_create_flags;
	uint8_t existing_data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t existing_data_len;

	*ret = get_entry(uid, sizeof(existing_data), existing_data, &existing_data_len,
			 &existing_create_flags);
	if (*ret != PSA_SUCCESS) {
		/* Allow overwriting entries that can't be read back to not be stuck with them
		 * forever, but make it visible as it may be a sign of corruption or tampering.
		 */
		if (*ret != PSA_ERROR_DOES_NOT_EXIST) {
			LOG_WRN("%s entry " SS_UID_FMT " that failed to be read back. (%d)",
				"Overwriting", SS_UID_ARGS(uid), *ret);
		}
		return false;
	}
	if (existing_create_flags & PSA_STORAGE_FLAG_WRITE_ONCE) {
		*ret = PSA_ERROR_NOT_PERMITTED;
		return true;
	}
	if (existing_data_len == data_length &&
	    existing_create_flags == create_flags &&
	    !memcmp(existing_data, p_data, data_length)) {
		LOG_DBG("Not writing entry " SS_UID_FMT " to storage because its stored data"
			" (of length %zu) is identical.", SS_UID_ARGS(uid), data_length);
		*ret = PSA_SUCCESS;
		return true;
	}
	return false;
}

static psa_status_t store_entry(secure_storage_uid_t uid, size_t data_length,
				const void *p_data, psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;
	uint8_t replay_prot[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
	size_t replay_prot_len;
	uint8_t old_replay_prot[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
	size_t old_replay_prot_len;
	bool has_old_replay_prot;
	psa_status_t rollback_ret;

	ret = secure_storage_ps_transform_to_store(uid, data_length, p_data, create_flags,
						    stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("transform", "for", ret);
		return PSA_ERROR_GENERIC_ERROR;
	}

	ret = secure_storage_ps_get_replay_protection(stored_data, stored_data_len,
						      replay_prot, &replay_prot_len);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to compute replay protection for entry " PSA_UID_FMT,
			PSA_UID_ARGS(uid.uid));
		return PSA_ERROR_GENERIC_ERROR;
	}

	/* Save the current replay protection value (if any) so that it can be restored if
	 * writing the new data to the PS store fails. This keeps the old entry readable.
	 */
	has_old_replay_prot = (secure_storage_its_get(SECURE_STORAGE_CALLER_PSA_PS, uid.uid, 0,
						      sizeof(old_replay_prot), old_replay_prot,
						      &old_replay_prot_len) == PSA_SUCCESS);

	ret = secure_storage_its_set(SECURE_STORAGE_CALLER_PSA_PS, uid.uid,
				     replay_prot_len, replay_prot, PSA_STORAGE_FLAG_NONE);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to store replay protection value for entry " PSA_UID_FMT " (%d)",
			PSA_UID_ARGS(uid.uid), ret);
		if ((ret == PSA_ERROR_INSUFFICIENT_STORAGE) ||
		    (ret == PSA_ERROR_STORAGE_FAILURE)) {
			return ret;
		}
		return PSA_ERROR_GENERIC_ERROR;
	}

	ret = secure_storage_ps_store_set(uid, stored_data_len, stored_data);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("write", "to", ret);

		if (has_old_replay_prot) {
			rollback_ret = secure_storage_its_set(SECURE_STORAGE_CALLER_PSA_PS,
							      uid.uid, old_replay_prot_len,
							      old_replay_prot,
							      PSA_STORAGE_FLAG_NONE);
		} else {
			rollback_ret = secure_storage_its_remove(SECURE_STORAGE_CALLER_PSA_PS,
								 uid.uid);
		}
		if (rollback_ret != PSA_SUCCESS) {
			LOG_ERR("Failed to roll back replay protection for entry " PSA_UID_FMT
				" (%d)", PSA_UID_ARGS(uid.uid), rollback_ret);
		}
	}
	return ret;
}

static psa_status_t ps_set(psa_storage_uid_t uid,
			   size_t data_length, const void *p_data,
			   psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;
	secure_storage_uid_t ps_uid;

	if (secure_storage_make_uid(SECURE_STORAGE_CALLER_PSA_PS, uid, &ps_uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (create_flags & ~SECURE_STORAGE_ALL_CREATE_FLAGS) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
	if (data_length > CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE) {
		LOG_DBG("Passed data length (%zu) exceeds maximum allowed (%u).",
			data_length, CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE);
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (keep_stored_entry(ps_uid, data_length, p_data, create_flags, &ret)) {
		return ret;
	}

	ret = store_entry(ps_uid, data_length, p_data, create_flags);
	return ret;
}

static psa_status_t ps_get(psa_storage_uid_t uid, size_t data_offset, size_t data_size,
			   void *p_data, size_t *p_data_length)
{
	psa_status_t ret;
	secure_storage_uid_t ps_uid;
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;
	psa_storage_create_flags_t create_flags;

	if (secure_storage_make_uid(SECURE_STORAGE_CALLER_PSA_PS, uid, &ps_uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_stored_data(ps_uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = check_replay_protection(uid, stored_data, stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	if (data_offset == 0
	 && data_size >= SECURE_STORAGE_PS_TRANSFORM_DATA_SIZE(stored_data_len)) {
		/* All the data fits directly in the provided buffer. */
		return transform_stored_data(ps_uid, stored_data_len, stored_data, data_size,
					     p_data, p_data_length, &create_flags);
	}
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t data_len;

	ret = transform_stored_data(ps_uid, stored_data_len, stored_data, sizeof(data), data,
				    &data_len, &create_flags);
	if (ret == PSA_SUCCESS) {
		if (data_offset > data_len) {
			LOG_DBG("Passed data offset (%zu) exceeds existing data length (%zu).",
				data_offset, data_len);
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		*p_data_length = MIN(data_size, data_len - data_offset);
		memcpy(p_data, data + data_offset, *p_data_length);
	}
	return ret;
}

static psa_status_t ps_get_info(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
	psa_status_t ret;
	secure_storage_uid_t ps_uid;
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];

	if (secure_storage_make_uid(SECURE_STORAGE_CALLER_PSA_PS, uid, &ps_uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_entry(ps_uid, sizeof(data), data, &p_info->size, &p_info->flags);
	if (ret == PSA_SUCCESS) {
		p_info->capacity = p_info->size;
	}
	return ret;
}

static psa_status_t ps_remove(psa_storage_uid_t uid)
{
	psa_status_t ret;
	secure_storage_uid_t ps_uid;
	psa_storage_create_flags_t create_flags;
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t data_len;

	if (secure_storage_make_uid(SECURE_STORAGE_CALLER_PSA_PS, uid, &ps_uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_entry(ps_uid, sizeof(data), data, &data_len, &create_flags);
	if (ret == PSA_SUCCESS && (create_flags & PSA_STORAGE_FLAG_WRITE_ONCE)) {
		return PSA_ERROR_NOT_PERMITTED;
	}
	/* Allow overwriting corrupted entries as well to not be stuck with them forever. */
	if (ret == PSA_SUCCESS ||
	    ret == PSA_ERROR_STORAGE_FAILURE ||
	    ret == PSA_ERROR_GENERIC_ERROR ||
	    ret == PSA_ERROR_INVALID_SIGNATURE ||
	    ret == PSA_ERROR_DATA_CORRUPT) {
		if (ret != PSA_SUCCESS) {
			LOG_WRN("%s entry " SS_UID_FMT " that failed to be read back. (%d)",
				"Removing", SS_UID_ARGS(ps_uid), ret);
		}
		ret = secure_storage_its_remove(SECURE_STORAGE_CALLER_PSA_PS, uid);
		if (ret != PSA_SUCCESS) {
			LOG_ERR("Failed to remove replay protection from ITS for " PSA_UID_FMT
				" (%d)", PSA_UID_ARGS(uid), ret);
		}
		ret = secure_storage_ps_store_remove(ps_uid);
		if (ret != PSA_SUCCESS) {
			log_failed_operation("remove", "from", ret);
			return PSA_ERROR_STORAGE_FAILURE;
		}
	}
	return ret;
}

/* Serializes all the operations on entries. The ones that modify an entry read it back
 * before deciding what to write or remove. Retrieving an entry needs it as well, because it
 * reads the data from the PS store and the replay protection value from ITS in two separate
 * steps, which must not be interleaved with a concurrent modification of the same entry.
 */
static K_MUTEX_DEFINE(s_mutex);

psa_status_t secure_storage_ps_get(const psa_storage_uid_t uid,
				   size_t data_offset, size_t data_size,
				   void *p_data, size_t *p_data_length)
{
	psa_status_t ret;

	k_mutex_lock(&s_mutex, K_FOREVER);
	ret = ps_get(uid, data_offset, data_size, p_data, p_data_length);
	k_mutex_unlock(&s_mutex);

	return ret;
}

psa_status_t secure_storage_ps_get_info(const psa_storage_uid_t uid,
					struct psa_storage_info_t *p_info)
{
	psa_status_t ret;

	k_mutex_lock(&s_mutex, K_FOREVER);
	ret = ps_get_info(uid, p_info);
	k_mutex_unlock(&s_mutex);

	return ret;
}

psa_status_t secure_storage_ps_set(const psa_storage_uid_t uid,
				   size_t data_length, const void *p_data,
				   psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;

	k_mutex_lock(&s_mutex, K_FOREVER);
	ret = ps_set(uid, data_length, p_data, create_flags);
	k_mutex_unlock(&s_mutex);

	return ret;
}

psa_status_t secure_storage_ps_remove(const psa_storage_uid_t uid)
{
	psa_status_t ret;

	k_mutex_lock(&s_mutex, K_FOREVER);
	ret = ps_remove(uid);
	k_mutex_unlock(&s_mutex);

	return ret;
}
