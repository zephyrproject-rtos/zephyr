/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <psa/crypto.h>
#include <zephyr/secure_storage/ps/replay_protection.h>

psa_status_t secure_storage_ps_get_replay_protection(
		const uint8_t *data,
		size_t data_len,
		const uint8_t *curr_rp,
		uint8_t output[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE])
{
	uint32_t sum = 0;

	ARG_UNUSED(curr_rp);

	for (int i = 0; i < data_len; i++) {
		sum += data[i];
	}

	memcpy(output, &sum, sizeof(sum));

	return PSA_SUCCESS;
}
