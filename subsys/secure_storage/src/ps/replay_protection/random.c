/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/ps/common.h>
#include <zephyr/secure_storage/ps/replay_protection.h>
#include <psa/crypto.h>

psa_status_t secure_storage_ps_get_replay_protection(
		const uint8_t *data,
		size_t data_len,
		const uint8_t *curr_rp,
		uint8_t output[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE])
{
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(curr_rp);

	return psa_generate_random(output, CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE);
}
