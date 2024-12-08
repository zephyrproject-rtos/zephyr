/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its.h>
#include <zephyr/secure_storage/its/store.h>
#include <zephyr/secure_storage/its/transform.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <string.h>

LOG_MODULE_DECLARE(secure_storage, CONFIG_SECURE_STORAGE_LOG_LEVEL);

static void log_failed_operation(const char *operation, const char *preposition, psa_status_t ret)
{
	LOG_ERR("Failed to %s data %s storage. (%d)", operation, preposition, ret);
}

static psa_status_t get_stored_data(
		secure_storage_its_uid_t uid,
		uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len)
{
	psa_status_t ret;

	ret = secure_storage_its_store_get(uid, SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE,
					   stored_data, stored_data_len);
	if (ret != PSA_SUCCESS) {
		if (ret != PSA_ERROR_DOES_NOT_EXIST) {
			log_failed_operation("retrieve", "from", ret);
		}
		return ret;
	}
	return PSA_SUCCESS;
}

static psa_status_t transform_stored_data(
		secure_storage_its_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t data_size, void *data, size_t *data_len,
		psa_storage_create_flags_t *create_flags)
{
	psa_status_t ret;

	ret = secure_storage_its_transform_from_store(uid, stored_data_len, stored_data,
						      data_size, data, data_len, create_flags);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("transform", "from", ret);
		return PSA_ERROR_STORAGE_FAILURE;
	}
	return PSA_SUCCESS;
}

static psa_status_t get_entry(secure_storage_its_uid_t uid, size_t data_size, uint8_t *data,
			      size_t *data_len, psa_storage_create_flags_t *create_flags)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;

	ret = get_stored_data(uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	return transform_stored_data(uid, stored_data_len, stored_data, data_size, data, data_len,
				     create_flags);
}

static bool keep_stored_entry(secure_storage_its_uid_t uid, size_t data_length, const void *p_data,
			      psa_storage_create_flags_t create_flags, psa_status_t *ret)
{
	psa_storage_create_flags_t existing_create_flags;
	uint8_t existing_data[CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE];
	size_t existing_data_len;

	*ret = get_entry(uid, sizeof(existing_data), existing_data, &existing_data_len,
			 &existing_create_flags);
	if (*ret != PSA_SUCCESS) {
		/* The entry either doesn't exist or is corrupted. */
		/* Allow overwriting corrupted entries to not be stuck with them forever. */
		return false;
	}
	if (existing_create_flags & PSA_STORAGE_FLAG_WRITE_ONCE) {
		*ret = PSA_ERROR_NOT_PERMITTED;
		return true;
	}
	if (existing_data_len == data_length &&
	    existing_create_flags == create_flags &&
	    !memcmp(existing_data, p_data, data_length)) {
		LOG_DBG("Not writing entry %u/%llu to storage because its stored data"
			" (of length %zu) is identical.", uid.caller_id, uid.uid, data_length);
		*ret = PSA_SUCCESS;
		return true;
	}
	return false;
}

static psa_status_t store_entry(secure_storage_its_uid_t uid, size_t data_length,
				const void *p_data, psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;

	ret = secure_storage_its_transform_to_store(uid, data_length, p_data, create_flags,
						    stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("transform", "for", ret);
		return PSA_ERROR_STORAGE_FAILURE;
	}

	ret = secure_storage_its_store_set(uid, stored_data_len, stored_data);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("write", "to", ret);
	}
	return ret;
}

psa_status_t secure_storage_its_set(secure_storage_its_uid_t uid, size_t data_length,
				    const void *p_data, psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;

	if (uid.uid == 0 || (p_data == NULL && data_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (create_flags & ~SECURE_STORAGE_ALL_CREATE_FLAGS) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
	if (data_length > CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE) {
		LOG_DBG("Passed data length (%zu) exceeds maximum allowed (%u).",
			data_length, CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE);
		return PSA_ERROR_INSUFFICIENT_STORAGE;
	}

	if (keep_stored_entry(uid, data_length, p_data, create_flags, &ret)) {
		return ret;
	}

	ret = store_entry(uid, data_length, p_data, create_flags);
	return ret;
}

psa_status_t secure_storage_its_get(secure_storage_its_uid_t uid, size_t data_offset,
				    size_t data_size, void *p_data, size_t *p_data_length)
{
	if (uid.uid == 0 || (p_data == NULL && data_size != 0) || p_data_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (data_size == 0) {
		*p_data_length = 0;
		return PSA_SUCCESS;
	}
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;
	psa_storage_create_flags_t create_flags;

	ret = get_stored_data(uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}
	if (data_offset == 0
	 && data_size >= SECURE_STORAGE_ITS_TRANSFORM_DATA_SIZE(stored_data_len)) {
		/* All the data fits directly in the provided buffer. */
		return transform_stored_data(uid, stored_data_len, stored_data, data_size, p_data,
					     p_data_length, &create_flags);
	}
	uint8_t data[CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE];
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

psa_status_t secure_storage_its_get_info(secure_storage_its_uid_t uid,
					 struct psa_storage_info_t *p_info)
{
	psa_status_t ret;
	uint8_t data[CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE];

	if (uid.uid == 0 || p_info == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_entry(uid, sizeof(data), data, &p_info->size, &p_info->flags);
	if (ret == PSA_SUCCESS) {
		p_info->capacity = p_info->size;
	}
	return ret;
}

psa_status_t secure_storage_its_remove(secure_storage_its_uid_t uid)
{
	psa_status_t ret;
	psa_storage_create_flags_t create_flags;
	uint8_t data[CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE];
	size_t data_len;

	if (uid.uid == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_entry(uid, sizeof(data), data, &data_len, &create_flags);
	if (ret == PSA_SUCCESS && (create_flags & PSA_STORAGE_FLAG_WRITE_ONCE)) {
		return PSA_ERROR_NOT_PERMITTED;
	}
	/* Allow overwriting corrupted entries as well to not be stuck with them forever. */
	if (ret == PSA_SUCCESS || ret == PSA_ERROR_STORAGE_FAILURE) {
		ret = secure_storage_its_store_remove(uid);
		if (ret != PSA_SUCCESS) {
			log_failed_operation("remove", "from", ret);
			return PSA_ERROR_STORAGE_FAILURE;
		}
	}
	return ret;
}
