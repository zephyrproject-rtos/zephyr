/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_PS_TRANSFORM_H
#define SECURE_STORAGE_PS_TRANSFORM_H

/** @file zephyr/secure_storage/ps/transform.h The secure storage PS transform module.
 *
 * The functions declared in this header implement the PS transform module.
 * They are meant to be called only by the PS implementation.
 * This header may be included when providing a custom implementation of the
 * PS transform module (@kconfig{CONFIG_SECURE_STORAGE_PS_TRANSFORM_IMPLEMENTATION_CUSTOM}).
 */
#include <zephyr/secure_storage/ps/common.h>

/** @brief Transforms the data of a PS entry for storage.
 *
 * @param[in]  uid               The entry's UID.
 * @param[in]  data_len          The number of bytes in `data`.
 * @param[in]  data              The data to transform for storage.
 * @param[in]  replay_protection The entry's replay protection value, which gets stored in ITS.
 *                               It must be bound to `stored_data` so that
 *                               secure_storage_ps_transform_from_store() fails when it is
 *                               called with a different replay protection value.
 * @param[out] stored_data       The buffer to which the transformed data is written.
 * @param[out] stored_data_len   On success, the number of bytes written to `stored_data`.
 *                               It must be at least 1, even for empty data.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_ps_transform_to_store(
		psa_storage_uid_t uid, size_t data_len, const void *data,
		const uint8_t
		replay_protection[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len);

/** @brief Transforms and validates the stored data of a PS entry for use.
 *
 * @param[in]  uid               The entry's UID.
 * @param[in]  stored_data_len   The number of bytes in `stored_data`.
 * @param[in]  stored_data       The stored data to transform for use.
 * @param[in]  replay_protection The entry's replay protection value, as retrieved from ITS.
 *                               It must match the one that was passed to
 *                               secure_storage_ps_transform_to_store() when `stored_data` was
 *                               generated.
 * @param[in]  data_size         The size of `data` in bytes.
 * @param[out] data              The buffer to which the transformed data is written.
 * @param[out] data_len          On success, the number of bytes written to `data`.
 *
 * @return `PSA_SUCCESS` on success, `PSA_ERROR_INVALID_SIGNATURE` or `PSA_ERROR_DATA_CORRUPT`
 *         if the stored data is invalid or does not match `replay_protection`, anything else on
 *         other failures.
 */
psa_status_t secure_storage_ps_transform_from_store(
		psa_storage_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		const uint8_t
		replay_protection[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		size_t data_size, void *data, size_t *data_len);

#endif
