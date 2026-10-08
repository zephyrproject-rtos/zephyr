/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/common.h>
#include <zephyr/secure_storage/ps.h>
#include <zephyr/secure_storage/ps/store.h>
#include <zephyr/secure_storage/ps/transform.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <string.h>

LOG_MODULE_REGISTER(secure_storage_ps, CONFIG_SECURE_STORAGE_LOG_LEVEL);

/* PS entries follow the same UID constraints as ITS ones. */
static psa_status_t check_uid(psa_storage_uid_t uid)
{
	if (uid == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

#ifndef CONFIG_SECURE_STORAGE_64_BIT_UID
	/* Check that the UID is not bigger than the maximum defined size. */
	if (uid & GENMASK64(63, SECURE_STORAGE_ITS_UID_BIT_SIZE)) {
		LOG_DBG("UID %#llx cannot be used as it has bits set past "
			"the first " STRINGIFY(SECURE_STORAGE_ITS_UID_BIT_SIZE) " ones.",
			(unsigned long long)uid);
		return PSA_ERROR_INVALID_ARGUMENT;
	}
#endif /* !CONFIG_SECURE_STORAGE_64_BIT_UID */

	return PSA_SUCCESS;
}

static void log_failed_operation(const char *operation, const char *preposition, psa_status_t ret)
{
	LOG_ERR("Failed to %s data %s storage. (%d)", operation, preposition, ret);
}

static psa_status_t get_stored_data(
		psa_storage_uid_t uid,
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
		psa_storage_uid_t uid, size_t stored_data_len,
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

static psa_status_t get_entry(psa_storage_uid_t uid, size_t data_size, uint8_t *data,
			      size_t *data_len, psa_storage_create_flags_t *create_flags)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;

	ret = get_stored_data(uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	return transform_stored_data(uid, stored_data_len, stored_data, data_size, data, data_len,
				     create_flags);
}

static bool keep_stored_entry(psa_storage_uid_t uid, size_t data_length, const void *p_data,
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
			LOG_WRN("%s entry " PSA_UID_FMT " that failed to be read back. (%d)",
				"Overwriting", PSA_UID_ARGS(uid), *ret);
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
		LOG_DBG("Not writing entry " PSA_UID_FMT " to storage because its stored data"
			" (of length %zu) is identical.", PSA_UID_ARGS(uid), data_length);
		*ret = PSA_SUCCESS;
		return true;
	}
	return false;
}

static psa_status_t store_entry(psa_storage_uid_t uid, size_t data_length,
				const void *p_data, psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;

	ret = secure_storage_ps_transform_to_store(uid, data_length, p_data, create_flags,
						    stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("transform", "for", ret);
		return PSA_ERROR_GENERIC_ERROR;
	}

	ret = secure_storage_ps_store_set(uid, stored_data_len, stored_data);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("write", "to", ret);
	}
	return ret;
}

static psa_status_t ps_set(psa_storage_uid_t uid,
			   size_t data_length, const void *p_data,
			   psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;

	if (check_uid(uid) != PSA_SUCCESS) {
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

	if (keep_stored_entry(uid, data_length, p_data, create_flags, &ret)) {
		return ret;
	}

	ret = store_entry(uid, data_length, p_data, create_flags);
	return ret;
}

static psa_status_t ps_get(psa_storage_uid_t uid, size_t data_offset, size_t data_size,
			   void *p_data, size_t *p_data_length)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;
	psa_storage_create_flags_t create_flags;

	if (check_uid(uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_stored_data(uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}
	if (data_offset == 0
	 && data_size >= SECURE_STORAGE_PS_TRANSFORM_DATA_SIZE(stored_data_len)) {
		/* All the data fits directly in the provided buffer. */
		return transform_stored_data(uid, stored_data_len, stored_data, data_size,
					     p_data, p_data_length, &create_flags);
	}
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t data_len;

	ret = transform_stored_data(uid, stored_data_len, stored_data, sizeof(data), data,
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
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];

	if (check_uid(uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_entry(uid, sizeof(data), data, &p_info->size, &p_info->flags);
	if (ret == PSA_SUCCESS) {
		p_info->capacity = p_info->size;
	}
	return ret;
}

static psa_status_t ps_remove(psa_storage_uid_t uid)
{
	psa_status_t ret;
	psa_storage_create_flags_t create_flags;
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t data_len;

	if (check_uid(uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_entry(uid, sizeof(data), data, &data_len, &create_flags);
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
			LOG_WRN("%s entry " PSA_UID_FMT " that failed to be read back. (%d)",
				"Removing", PSA_UID_ARGS(uid), ret);
		}
		ret = secure_storage_ps_store_remove(uid);
		if (ret != PSA_SUCCESS) {
			log_failed_operation("remove", "from", ret);
			return PSA_ERROR_STORAGE_FAILURE;
		}
	}
	return ret;
}

/* Serializes all the operations on entries. The ones that modify an entry read it back
 * before deciding what to write or remove.
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
