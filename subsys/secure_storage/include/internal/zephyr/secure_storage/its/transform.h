/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ITS_TRANSFORM_H
#define SECURE_STORAGE_ITS_TRANSFORM_H

/** @file zephyr/secure_storage/its/transform.h The secure storage ITS transform module.
 *
 * The functions declared in this header implement the ITS transform module.
 * They are meant to be called only by the ITS implementation.
 * This header may be included when providing a custom implementation of the
 * ITS transform module (@kconfig{CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_CUSTOM}).
 */
#include <zephyr/secure_storage/its/common.h>

/** The maximum size, in bytes, of an entry's data after it has been transformed for storage. */
enum { SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE
	= CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE
	  + sizeof(secure_storage_packed_create_flags_t)
	  + CONFIG_SECURE_STORAGE_ITS_TRANSFORM_OUTPUT_OVERHEAD };

#define SECURE_STORAGE_ITS_TRANSFORM_DATA_SIZE(stored_data_len) \
	(stored_data_len - (SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE \
			    - CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE))

/** @brief Transforms the data of an ITS entry for storage.
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
psa_status_t secure_storage_its_transform_to_store(
		secure_storage_its_uid_t uid, size_t data_len, const void *data,
		secure_storage_packed_create_flags_t create_flags,
		uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len);

/** @brief Transforms and validates the stored data of an ITS entry for use.
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
psa_status_t secure_storage_its_transform_from_store(
		secure_storage_its_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t data_size, void *data, size_t *data_len,
		psa_storage_create_flags_t *create_flags);

#endif
