/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/ps/store.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <string.h>

static struct {
	psa_storage_uid_t uid;
	size_t data_length;
	uint8_t data[SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE];
} s_ps_entries[100];

static K_MUTEX_DEFINE(s_ps_entries_mutex);

static int get_existing_entry_index(psa_storage_uid_t uid)
{
	__ASSERT_NO_MSG(uid != 0);

	for (unsigned int i = 0; i != ARRAY_SIZE(s_ps_entries); ++i) {
		if (s_ps_entries[i].uid == uid) {
			return i;
		}
	}
	return -1;
}

psa_status_t secure_storage_ps_store_set(psa_storage_uid_t uid,
					  size_t data_length, const void *data)
{
	k_mutex_lock(&s_ps_entries_mutex, K_FOREVER);

	__ASSERT_NO_MSG(data_length <= sizeof(s_ps_entries[0].data));
	int index = get_existing_entry_index(uid);

	if (index == -1) {
		for (unsigned int i = 0; i != ARRAY_SIZE(s_ps_entries); ++i) {
			if (s_ps_entries[i].uid == 0) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			k_mutex_unlock(&s_ps_entries_mutex);
			return PSA_ERROR_INSUFFICIENT_STORAGE;
		}
		s_ps_entries[index].uid = uid;
	}

	s_ps_entries[index].data_length = data_length;
	memcpy(s_ps_entries[index].data, data, data_length);

	k_mutex_unlock(&s_ps_entries_mutex);
	return PSA_SUCCESS;
}

psa_status_t secure_storage_ps_store_get(psa_storage_uid_t uid, size_t data_size,
					  void *data, size_t *data_length)
{
	k_mutex_lock(&s_ps_entries_mutex, K_FOREVER);

	const int index = get_existing_entry_index(uid);

	if (index == -1) {
		k_mutex_unlock(&s_ps_entries_mutex);
		return PSA_ERROR_DOES_NOT_EXIST;
	}
	*data_length = MIN(data_size, s_ps_entries[index].data_length);
	memcpy(data, s_ps_entries[index].data, *data_length);

	k_mutex_unlock(&s_ps_entries_mutex);
	return PSA_SUCCESS;
}

psa_status_t secure_storage_ps_store_remove(psa_storage_uid_t uid)
{
	k_mutex_lock(&s_ps_entries_mutex, K_FOREVER);

	const int index = get_existing_entry_index(uid);

	if (index == -1) {
		k_mutex_unlock(&s_ps_entries_mutex);
		return PSA_ERROR_DOES_NOT_EXIST;
	}
	s_ps_entries[index].uid = 0;

	k_mutex_unlock(&s_ps_entries_mutex);
	return PSA_SUCCESS;
}
