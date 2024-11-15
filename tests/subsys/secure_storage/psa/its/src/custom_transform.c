/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/transform.h>
#include <zephyr/sys/__assert.h>
#include <string.h>

psa_status_t secure_storage_its_transform_to_store(
		secure_storage_its_uid_t uid, size_t data_len, const void *data,
		secure_storage_packed_create_flags_t create_flags,
		uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len)
{
	*stored_data_len = data_len + sizeof(secure_storage_packed_create_flags_t);
	__ASSERT_NO_MSG(data_len <= CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE);
	__ASSERT_NO_MSG(*stored_data_len <= SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE);
	memcpy(stored_data, data, data_len);
	*(secure_storage_packed_create_flags_t *)(&stored_data[data_len]) = create_flags;
	return PSA_SUCCESS;
}

psa_status_t secure_storage_its_transform_from_store(
		secure_storage_its_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t data_size, void *data, size_t *data_len,
		psa_storage_create_flags_t *create_flags)
{
	*data_len = stored_data_len - sizeof(secure_storage_packed_create_flags_t);
	__ASSERT_NO_MSG(data_size >= *data_len);
	memcpy(data, stored_data, *data_len);
	*create_flags = *(secure_storage_packed_create_flags_t *)(&stored_data[*data_len]);
	return PSA_SUCCESS;
}
