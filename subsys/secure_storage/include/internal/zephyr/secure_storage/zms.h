/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ZMS_H
#define SECURE_STORAGE_ZMS_H

/** @file zephyr/secure_storage/zms.h ZMS helpers shared by the ITS and PS store modules. */
#include <stdint.h>
#include <zephyr/kvss/zms.h>
#include <zephyr/secure_storage/uid.h>
#include <psa/error.h>

/** @brief Writes the data of an ITS/PS entry to a ZMS instance.
 *
 * @param zms         The ZMS instance.
 * @param uid         The entry's UID.
 * @param data_length The number of bytes in `data`.
 * @param data        The data to store.
 *
 * @retval PSA_SUCCESS                    The write succeeded.
 * @retval PSA_ERROR_INSUFFICIENT_STORAGE There is not enough space in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE      Some storage failure happened.
 */
psa_status_t secure_storage_store_set(struct zms_fs *zms, secure_storage_uid_t uid,
				      size_t data_length, const void *data);

/** @brief Retrieves the data of an ITS/PS entry from a ZMS instance.
 *
 * @param[in]  zms         The ZMS instance.
 * @param[in]  uid         The entry's UID.
 * @param[in]  data_size   The size of `data` in bytes.
 * @param[out] data        The buffer to which the entry's stored data is written.
 * @param[out] data_length On success, the number of bytes written to `data`.
 *                         May be less than `data_size`.
 *
 * @retval PSA_SUCCESS               The read succeeded.
 * @retval PSA_ERROR_DOES_NOT_EXIST  The entry was not found from the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE Some storage failure happened.
 */
psa_status_t secure_storage_store_get(struct zms_fs *zms, secure_storage_uid_t uid,
				      size_t data_size, void *data, size_t *data_length);

/** @brief Removes an ITS/PS entry from a ZMS instance.
 *
 * @param zms The ZMS instance.
 * @param uid The entry's UID.
 *
 * @retval PSA_SUCCESS               The removal succeeded.
 * @retval PSA_ERROR_STORAGE_FAILURE Some storage failure happened.
 */
psa_status_t secure_storage_store_remove(struct zms_fs *zms, secure_storage_uid_t uid);

#endif /* SECURE_STORAGE_ZMS_H */
