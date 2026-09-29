/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <psa/crypto.h>
#include <zephyr/secure_storage/ps/replay_protection.h>

psa_status_t secure_storage_ps_get_replay_protection(
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t stored_data_len,
		uint8_t output[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		size_t *output_len)
{
	uint32_t sum = 0;

	for (int i = 0; i < stored_data_len; i++) {
		sum += stored_data[i];
	}

	memcpy(output, &sum, sizeof(sum));
	*output_len = sizeof(sum);

	return PSA_SUCCESS;
}
