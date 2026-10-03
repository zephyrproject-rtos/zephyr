/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/uid.h>
#include <zephyr/secure_storage/ps/common.h>
#include <zephyr/secure_storage/ps/replay_protection.h>
#include <psa/crypto.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(secure_storage_ps, CONFIG_SECURE_STORAGE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE >= PSA_HASH_LENGTH(PSA_ALG_SHA_256));

psa_status_t secure_storage_ps_get_replay_protection(
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t stored_data_len,
		uint8_t output[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		size_t *output_len)
{
	return psa_hash_compute(PSA_ALG_SHA_256, stored_data, stored_data_len, output,
				CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE, output_len);
}
