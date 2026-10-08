/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/ps/transform.h>
#include <zephyr/sys/__assert.h>
#include <string.h>

#define RP_SIZE     CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE

psa_status_t secure_storage_ps_transform_to_store(
		psa_storage_uid_t uid, size_t data_len, const void *data,
		const uint8_t
		replay_protection[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len)
{
	*stored_data_len = RP_SIZE + data_len;
	__ASSERT_NO_MSG(data_len <= CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE);
	__ASSERT_NO_MSG(*stored_data_len <= SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE);
	memcpy(&stored_data[0], replay_protection, RP_SIZE);
	memcpy(&stored_data[RP_SIZE], data, data_len);
	return PSA_SUCCESS;
}

psa_status_t secure_storage_ps_transform_from_store(
		psa_storage_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		const uint8_t
		replay_protection[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		size_t data_size, void *data, size_t *data_len)
{
	const uint8_t *stored_rp_ptr = &stored_data[0];
	const uint8_t *stored_data_ptr = &stored_data[RP_SIZE];

	if (stored_data_len < RP_SIZE) {
		return PSA_ERROR_DATA_CORRUPT;
	}
	if (memcmp(stored_rp_ptr, replay_protection, RP_SIZE) != 0) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}
	if (data_size < stored_data_len - RP_SIZE) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	*data_len = stored_data_len - RP_SIZE;
	memcpy(data, stored_data_ptr, *data_len);
	return PSA_SUCCESS;
}
