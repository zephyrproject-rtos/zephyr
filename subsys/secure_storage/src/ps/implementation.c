/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/common.h>
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

BUILD_ASSERT(SECURE_STORAGE_ALL_CREATE_FLAGS
	     <= (1 << (8 * sizeof(secure_storage_packed_create_flags_t))) - 1);

struct its_stored_data {
	secure_storage_packed_create_flags_t flags;
	uint8_t replay_protection[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
};

BUILD_ASSERT(sizeof(struct its_stored_data) <= CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE,
	     "CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE <= "
	     "CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE - 1");

static void log_failed_operation(const char *operation, const char *preposition,
				 psa_storage_uid_t uid, psa_status_t ret)
{
	LOG_ERR("Failed to %s data %s storage for entry " PSA_UID_FMT " (%d)",
		operation, preposition, PSA_UID_ARGS(uid), ret);
}

static psa_status_t get_its_data(psa_storage_uid_t uid,
				 struct its_stored_data *its_data)
{
	size_t its_data_len;
	psa_status_t ret;

	ret = secure_storage_its_get(SECURE_STORAGE_ITS_CALLER_PSA_PS, uid, 0,
				     sizeof(*its_data), its_data,
				     &its_data_len);
	if (ret == PSA_ERROR_DOES_NOT_EXIST) {
		LOG_DBG("ITS data for entry " PSA_UID_FMT " does not exist", PSA_UID_ARGS(uid));
		return PSA_ERROR_DOES_NOT_EXIST;
	} else if ((ret == PSA_ERROR_INVALID_SIGNATURE) || (ret == PSA_ERROR_DATA_CORRUPT)) {
		LOG_WRN("ITS data for entry " PSA_UID_FMT " is corrupted", PSA_UID_ARGS(uid));
		return PSA_ERROR_DATA_CORRUPT;
	} else if (ret != PSA_SUCCESS) {
		LOG_ERR("Fatal error (%d) retrieving ITS data for entry " PSA_UID_FMT,
			ret, PSA_UID_ARGS(uid));
		return PSA_ERROR_GENERIC_ERROR;
	}

	return PSA_SUCCESS;
}

static psa_status_t store_its_data(psa_storage_uid_t uid, struct its_stored_data *its_data)
{
	psa_status_t ret;

	ret = secure_storage_its_set(SECURE_STORAGE_ITS_CALLER_PSA_PS, uid,
				     sizeof(struct its_stored_data), its_data,
				     PSA_STORAGE_FLAG_NONE);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Failed to store ITS data for entry " PSA_UID_FMT " (%d)",
			PSA_UID_ARGS(uid), ret);
		if ((ret == PSA_ERROR_INSUFFICIENT_STORAGE) ||
		    (ret == PSA_ERROR_STORAGE_FAILURE)) {
			return ret;
		}
		return PSA_ERROR_GENERIC_ERROR;
	}

	return PSA_SUCCESS;
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
			log_failed_operation("retrieve", "from", uid, ret);
		}
	}
	return ret;
}

static psa_status_t transform_stored_data(
		psa_storage_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		struct its_stored_data *its_data,
		size_t data_size, void *data, size_t *data_len)
{
	psa_status_t ret;

	ret = secure_storage_ps_transform_from_store(uid, stored_data_len, stored_data,
						     its_data->replay_protection,
						     data_size, data, data_len);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("transform", "from", uid, ret);
		if (ret == PSA_ERROR_INVALID_SIGNATURE || ret == PSA_ERROR_DATA_CORRUPT) {
			return ret;
		}
		return PSA_ERROR_GENERIC_ERROR;
	}
	return PSA_SUCCESS;
}

static psa_status_t get_entry(psa_storage_uid_t uid, struct its_stored_data *its_data,
			      size_t data_size, uint8_t *data,
			      size_t *data_len)
{
	psa_status_t ret;
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;

	ret = get_stored_data(uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	return transform_stored_data(uid, stored_data_len, stored_data, its_data,
				     data_size, data, data_len);
}

static bool keep_stored_entry(psa_storage_uid_t uid, size_t data_length, const void *p_data,
			      psa_storage_create_flags_t new_flags,
			      struct its_stored_data *its_data, psa_status_t *ret)
{
	uint8_t existing_data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t existing_data_len;

	*ret = get_entry(uid, its_data, sizeof(existing_data), existing_data, &existing_data_len);
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

	if (its_data->flags == new_flags &&
	    existing_data_len == data_length &&
	    memcmp(existing_data, p_data, data_length) == 0) {
		LOG_DBG("Not writing entry " PSA_UID_FMT " to storage because its stored data"
			" (of length %zu) is identical.", PSA_UID_ARGS(uid), data_length);
		*ret = PSA_SUCCESS;
		return true;
	}
	return false;
}

static psa_status_t store_entry(psa_storage_uid_t uid, size_t data_length, const void *p_data,
				struct its_stored_data *its_data_rollback,
				struct its_stored_data *its_data_new)
{
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;
	bool is_new_entry = (its_data_rollback == NULL);
	psa_status_t rollback_ret;
	psa_status_t ret;

	ret = secure_storage_ps_transform_to_store(uid, data_length, p_data,
						   its_data_new->replay_protection,
						   stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		log_failed_operation("transform", "for", uid, ret);
		return PSA_ERROR_GENERIC_ERROR;
	}

	if (is_new_entry) {
		/* In case of a new entry write PS first and then ITS. This
		 * way if PS write fails and the entry was meant to be WRITE_ONCE
		 * we're not stuck with an unremovable entry.
		 */
		ret = secure_storage_ps_store_set(uid, stored_data_len, stored_data);
		if (ret != PSA_SUCCESS) {
			log_failed_operation("write", "to", uid, ret);
			return ret;
		}

		ret = store_its_data(uid, its_data_new);
		if (ret != PSA_SUCCESS) {
			return ret;
		}
	} else {
		/* Since overwriting an existing entry might add the WRITE_ONCE
		 * flag, the behavior is as follows:
		 * 1. write ITS without WRITE_ONCE first
		 * 2. write PS
		 * 3. if (2) is fine then update the ITS entry adding WRITE_ONCE.
		 * This prevents from having "blocked" and corrupted entries in case of
		 * power-loss between ITS and PS writes.
		 */
		bool is_write_once = ((its_data_new->flags & PSA_STORAGE_FLAG_WRITE_ONCE) != 0);

		/* Create a copy of 'its_data_new' so that if this function returns
		 * before WRITE_ONCE is set back again, the caller doesn't see its
		 * struct changed.
		 */
		struct its_stored_data its_data_new_copy = *its_data_new;
		its_data_new_copy.flags &= ~PSA_STORAGE_FLAG_WRITE_ONCE;

		ret = store_its_data(uid, &its_data_new_copy);
		if (ret != PSA_SUCCESS) {
			return ret;
		}

		ret = secure_storage_ps_store_set(uid, stored_data_len, stored_data);
		if (ret != PSA_SUCCESS) {
			log_failed_operation("write", "to", uid, ret);
			/* Try to rollback. */
			rollback_ret = secure_storage_its_set(SECURE_STORAGE_ITS_CALLER_PSA_PS,
							      uid,
							      sizeof(struct its_stored_data),
							      its_data_rollback,
							      PSA_STORAGE_FLAG_NONE);
			if (rollback_ret != PSA_SUCCESS) {
				LOG_ERR("Failed to rollback ITS data for entry " PSA_UID_FMT
					" (%d)", PSA_UID_ARGS(uid), rollback_ret);
			}
			/* Return the error from ps_store_set(). */
			return ret;
		}

		if (is_write_once) {
			its_data_new_copy.flags |= PSA_STORAGE_FLAG_WRITE_ONCE;
			ret = store_its_data(uid, &its_data_new_copy);
			if (ret != PSA_SUCCESS) {
				return ret;
			}
		}
	}

	return PSA_SUCCESS;
}

static psa_status_t ps_set(psa_storage_uid_t uid,
			   size_t data_length, const void *p_data,
			   psa_storage_create_flags_t create_flags)
{
	psa_status_t ret;
	struct its_stored_data its_data_curr, its_data_new;
	struct its_stored_data *its_data_rollback = NULL;
	const uint8_t *curr_replay_protection = NULL;

	if (check_uid(uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if ((create_flags & ~SECURE_STORAGE_ALL_CREATE_FLAGS) != 0U) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
	if (data_length > CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE) {
		LOG_DBG("Passed data length (%zu) exceeds maximum allowed (%u).",
			data_length, CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE);
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_its_data(uid, &its_data_curr);
	if (ret == PSA_ERROR_GENERIC_ERROR) {
		return ret;
	}

	if (ret == PSA_SUCCESS) {
		/* The create flags can only be checked if the ITS data is readable. If it is
		 * corrupted, there is no way to tell if the entry was WRITE_ONCE, so overwriting
		 * it is allowed.
		 */
		if ((its_data_curr.flags & PSA_STORAGE_FLAG_WRITE_ONCE) != 0U) {
			LOG_ERR("Cannot write WRITE_ONCE entry " PSA_UID_FMT, PSA_UID_ARGS(uid));
			return PSA_ERROR_NOT_PERMITTED;
		}

		if (keep_stored_entry(uid, data_length, p_data, create_flags,
				      &its_data_curr, &ret)) {
			return ret;
		}

		its_data_rollback = &its_data_curr;
		curr_replay_protection = its_data_curr.replay_protection;
	}

	its_data_new.flags = create_flags;
	ret = secure_storage_ps_get_replay_protection(p_data, data_length,
						      curr_replay_protection,
						      its_data_new.replay_protection);
	if (ret != PSA_SUCCESS) {
		LOG_ERR("Error generating replay protection value for entry " PSA_UID_FMT,
			PSA_UID_ARGS(uid));
		return PSA_ERROR_GENERIC_ERROR;
	}

	return store_entry(uid, data_length, p_data, its_data_rollback, &its_data_new);
}

static psa_status_t ps_get(psa_storage_uid_t uid, size_t data_offset, size_t data_size,
			   void *p_data, size_t *p_data_length)
{
	uint8_t stored_data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
	size_t stored_data_len;
	struct its_stored_data its_data;
	psa_status_t ret;

	if (check_uid(uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_its_data(uid, &its_data);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = get_stored_data(uid, stored_data, &stored_data_len);
	if (ret != PSA_SUCCESS) {
		if (ret == PSA_ERROR_DOES_NOT_EXIST) {
			/* ITS exists, but PS not: data is corrupted/tampered. */
			return PSA_ERROR_DATA_CORRUPT;
		}
		return PSA_ERROR_STORAGE_FAILURE;
	}

	if (data_offset == 0 &&
	    data_size >= SECURE_STORAGE_PS_TRANSFORM_DATA_SIZE(stored_data_len)) {
		/* All the data fits directly in the provided buffer. */
		return transform_stored_data(uid, stored_data_len, stored_data, &its_data,
					     data_size, p_data, p_data_length);
	}
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t data_len;

	ret = transform_stored_data(uid, stored_data_len, stored_data, &its_data,
				    sizeof(data), data, &data_len);
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
	uint8_t data[CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE];
	size_t data_len;
	struct its_stored_data its_data;
	psa_status_t ret;

	if (check_uid(uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_its_data(uid, &its_data);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = get_entry(uid, &its_data, sizeof(data), data, &data_len);
	if (ret == PSA_ERROR_DOES_NOT_EXIST) {
		/* ITS exists, but PS not: data is corrupted/tampered. */
		return PSA_ERROR_DATA_CORRUPT;
	} else if (ret != PSA_SUCCESS) {
		return ret;
	}

	p_info->flags = its_data.flags;
	p_info->size = data_len;
	p_info->capacity = data_len;

	return PSA_SUCCESS;
}

static psa_status_t ps_remove(psa_storage_uid_t uid)
{
	struct its_stored_data its_data;
	psa_status_t ret;
	bool its_exist;

	if (check_uid(uid) != PSA_SUCCESS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = get_its_data(uid, &its_data);
	if (ret == PSA_ERROR_GENERIC_ERROR) {
		return PSA_ERROR_GENERIC_ERROR;
	} else if ((ret == PSA_SUCCESS) &&
		   ((its_data.flags & PSA_STORAGE_FLAG_WRITE_ONCE) != 0U)) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	if (ret != PSA_ERROR_DOES_NOT_EXIST) {
		its_exist = true;
		ret = secure_storage_its_remove(SECURE_STORAGE_ITS_CALLER_PSA_PS, uid);
		if (ret != PSA_SUCCESS) {
			LOG_ERR("Failed to remove ITS data for entry " PSA_UID_FMT " (%d)",
				PSA_UID_ARGS(uid), ret);
			return PSA_ERROR_GENERIC_ERROR;
		}
	} else {
		its_exist = false;
	}

	/* The ZMS and settings stores return success even if the UID does not exist, while
	 * other (custom) stores may return PSA_ERROR_DOES_NOT_EXIST.
	 */
	ret = secure_storage_ps_store_remove(uid);
	if ((ret != PSA_SUCCESS) && (ret != PSA_ERROR_DOES_NOT_EXIST)) {
		LOG_ERR("Failed to remove entry " PSA_UID_FMT " (%d)", PSA_UID_ARGS(uid), ret);
		return PSA_ERROR_STORAGE_FAILURE;
	}

	if (!its_exist) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}
	return PSA_SUCCESS;
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
