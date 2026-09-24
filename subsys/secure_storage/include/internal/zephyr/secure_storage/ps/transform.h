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
 * @param[in]  uid             The entry's UID.
 * @param[in]  data_len        The number of bytes in `data`.
 * @param[in]  data            The data to transform for storage.
 * @param[in]  create_flags    The entry's create flags. It must contain only valid
 *                             `PSA_STORAGE_FLAG_*` values. It gets stored as part of `stored_data`.
 * @param[out] stored_data     The buffer to which the transformed data is written.
 * @param[out] stored_data_len On success, the number of bytes written to `stored_data`.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_ps_transform_to_store(
		psa_storage_uid_t uid, size_t data_len, const void *data,
		secure_storage_packed_create_flags_t create_flags,
		uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len);

/** @brief Transforms and validates the stored data of a PS entry for use.
 *
 * @param[in]  uid             The entry's UID.
 * @param[in]  stored_data_len The number of bytes in `stored_data`.
 * @param[in]  stored_data     The stored data to transform for use.
 * @param[in]  data_size       The size of `data` in bytes.
 * @param[out] data            The buffer to which the transformed data is written.
 * @param[out] data_len        On success, the number of bytes written to `stored_data`.
 * @param[out] create_flags    On success, the entry's create flags.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_ps_transform_from_store(
		psa_storage_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t data_size, void *data, size_t *data_len,
		psa_storage_create_flags_t *create_flags);

#endif
