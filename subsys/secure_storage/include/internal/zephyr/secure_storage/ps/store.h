/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_PS_STORE_H
#define SECURE_STORAGE_PS_STORE_H

/** @file zephyr/secure_storage/ps/store.h The secure storage PS store module.
 *
 * The functions declared in this header implement the PS store module.
 * They are meant to be called only by the PS implementation.
 * This header may be included when providing a custom implementation of the
 * PS store module (@kconfig{CONFIG_SECURE_STORAGE_PS_STORE_IMPLEMENTATION_CUSTOM}).
 *
 * The PS implementation serializes all the operations on entries, so the functions of
 * this module are never called concurrently.
 */
#include <zephyr/secure_storage/ps/common.h>

/** @brief Writes the data of a PS entry to the storage medium.
 *
 * @param uid         The entry's UID.
 * @param data_length The number of bytes in `data`.
 * @param data        The data to store.
 *
 * @return One of the return values of `psa_ps_set()`.
 */
psa_status_t secure_storage_ps_store_set(psa_storage_uid_t uid,
					  size_t data_length, const void *data);

/** @brief Retrieves the data of a PS entry from the storage medium.
 *
 * @param[in]  uid         The entry's UID.
 * @param[in]  data_size   The size of `data` in bytes.
 * @param[out] data        The buffer to which the entry's stored data is written.
 * @param[out] data_length On success, the number of bytes written to `data`.
 *                         May be less than `data_size`.
 *
 * @retval PSA_SUCCESS               The read succeeded.
 * @retval PSA_ERROR_DOES_NOT_EXIST  The entry was not found from the storage.
 * @retval PSA_ERROR_DATA_CORRUPT    The stored entry is larger than data_size.
 * @retval PSA_ERROR_STORAGE_FAILURE Some storage failure happened.
 */
psa_status_t secure_storage_ps_store_get(psa_storage_uid_t uid, size_t data_size,
					  void *data, size_t *data_length);

/** @brief Removes a PS entry from the storage medium.
 *
 * @param uid The entry's UID.
 *
 * @retval PSA_SUCCESS              The removal succeeded, or the entry did not exist.
 * @retval PSA_ERROR_DOES_NOT_EXIST The entry did not exist. Treated like `PSA_SUCCESS`.
 * @return Anything else on failure. `psa_ps_remove()` then returns
 *         `PSA_ERROR_STORAGE_FAILURE`.
 */
psa_status_t secure_storage_ps_store_remove(psa_storage_uid_t uid);

#endif
