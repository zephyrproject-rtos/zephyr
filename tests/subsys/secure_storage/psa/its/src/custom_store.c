/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/store.h>
#include <zephyr/secure_storage/its/transform.h>
#include <zephyr/sys/util.h>
#include <string.h>

static struct {
	secure_storage_its_uid_t uid;
	size_t data_length;
	uint8_t data[SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE];
} s_its_entries[100];

static int get_existing_entry_index(secure_storage_its_uid_t uid)
{
	__ASSERT_NO_MSG(uid.uid != 0);

	for (unsigned int i = 0; i != ARRAY_SIZE(s_its_entries); ++i) {
		if (!memcmp(&uid, &s_its_entries[i].uid, sizeof(uid))) {
			return i;
		}
	}
	return -1;
}

psa_status_t secure_storage_its_store_set(secure_storage_its_uid_t uid,
					  size_t data_length, const void *data)
{
	__ASSERT_NO_MSG(data_length <= sizeof(s_its_entries[0].data));
	int index = get_existing_entry_index(uid);

	if (index == -1) {
		for (unsigned int i = 0; i != ARRAY_SIZE(s_its_entries); ++i) {
			if (s_its_entries[i].uid.uid == 0) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			return PSA_ERROR_INSUFFICIENT_STORAGE;
		}
		s_its_entries[index].uid = uid;
	}

	s_its_entries[index].data_length = data_length;
	memcpy(s_its_entries[index].data, data, data_length);
	return PSA_SUCCESS;
}

psa_status_t secure_storage_its_store_get(secure_storage_its_uid_t uid, size_t data_size,
					  void *data, size_t *data_length)
{
	const int index = get_existing_entry_index(uid);

	if (index == -1) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}
	*data_length = MIN(data_size, s_its_entries[index].data_length);
	memcpy(data, s_its_entries[index].data, *data_length);
	return PSA_SUCCESS;
}

psa_status_t secure_storage_its_store_remove(secure_storage_its_uid_t uid)
{
	const int index = get_existing_entry_index(uid);

	if (index == -1) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}
	s_its_entries[index].uid.uid = 0;
	return PSA_SUCCESS;
}
