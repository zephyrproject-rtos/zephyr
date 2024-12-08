/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ITS_STORE_H
#define SECURE_STORAGE_ITS_STORE_H

/** @file zephyr/secure_storage/its/store.h The secure storage ITS store module.
 *
 * The functions declared in this header implement the ITS store module.
 * They are meant to be called only by the ITS implementation.
 * This header may be included when providing a custom implementation of the
 * ITS store module (@kconfig{CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_CUSTOM}).
 */
#include <zephyr/secure_storage/its/common.h>

/** @brief Writes the data of an ITS entry to the storage medium.
 *
 * @param uid         The entry's UID.
 * @param data_length The number of bytes in `data`.
 * @param data        The data to store.
 *
 * @retval `PSA_SUCCESS` on success.
 * @retval `PSA_ERROR_INSUFFICIENT_STORAGE` if there is insufficient storage space.
 * @retval `PSA_ERROR_STORAGE_FAILURE` on any other failure.
 */
psa_status_t secure_storage_its_store_set(secure_storage_its_uid_t uid,
					  size_t data_length, const void *data);

/** @brief Retrieves the data of an ITS entry from the storage medium.
 *
 * @param[in]  uid         The entry's UID.
 * @param[in]  data_size   The size of `data` in bytes.
 * @param[out] data        The buffer to which the entry's stored data is written.
 * @param[out] data_length On success, the number of bytes written to `data`.
 *                         May be less than `data_size`.
 *
 * @retval `PSA_SUCCESS` on success.
 * @retval `PSA_ERROR_DOES_NOT_EXIST` if no entry with the given UID exists.
 * @retval `PSA_ERROR_STORAGE_FAILURE` on any other failure.
 */
psa_status_t secure_storage_its_store_get(secure_storage_its_uid_t uid, size_t data_size,
					  void *data, size_t *data_length);

/** @brief Removes an ITS entry from the storage medium.
 *
 * @param uid The entry's UID.
 *
 * @return `PSA_SUCCESS` on success, anything else on failure.
 */
psa_status_t secure_storage_its_store_remove(secure_storage_its_uid_t uid);

#endif
