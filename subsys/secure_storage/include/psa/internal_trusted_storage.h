/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef PSA_INTERNAL_TRUSTED_STORAGE_H
#define PSA_INTERNAL_TRUSTED_STORAGE_H

/** @file psa/internal_trusted_storage.h The PSA Internal Trusted Storage (ITS) API.
 * @ingroup psa_secure_storage
 * For more information on the ITS, see [The Internal Trusted Storage API](https://arm-software.github.io/psa-api/storage/1.0/overview/architecture.html#the-internal-trusted-storage-api).
 */

/** @cond INTERNAL_HIDDEN */
#include "../internal/zephyr/secure_storage/its.h"
#ifdef BUILDING_MBEDTLS_CRYPTO
#define ITS_CALLER_ID SECURE_STORAGE_ITS_CALLER_MBEDTLS
#else
#define ITS_CALLER_ID SECURE_STORAGE_ITS_CALLER_PSA_ITS
#endif
#define ITS_UID (secure_storage_its_uid_t){.uid = uid, .caller_id = ITS_CALLER_ID}
/** @endcond */

#include <psa/storage_common.h>

#define PSA_ITS_API_VERSION_MAJOR 1
#define PSA_ITS_API_VERSION_MINOR 0

/**
 * @brief Creates a new or modifies an existing entry.
 *
 * Stores data in the internal storage.
 *
 * @param uid          The identifier of the data. Must be nonzero.
 * @param data_length  The size in bytes of the data in `p_data` to store.
 * @param p_data       A buffer containing the data to store.
 * @param create_flags Flags indicating the properties of the entry.
 *
 * @retval PSA_SUCCESS                    The operation completed successfully.
 * @retval PSA_ERROR_NOT_PERMITTED        An entry associated with the provided `uid` already
 *                                        exists and was created with `PSA_STORAGE_FLAG_WRITE_ONCE`.
 * @retval PSA_ERROR_NOT_SUPPORTED        One or more of the flags provided in `create_flags`
 *                                        are not supported or invalid.
 * @retval PSA_ERROR_INVALID_ARGUMENT     One or more arguments other than `create_flags` are
 *                                        invalid.
 * @retval PSA_ERROR_INSUFFICIENT_STORAGE There is insufficient space on the storage medium.
 * @retval PSA_ERROR_STORAGE_FAILURE      The physical storage has failed (fatal error).
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_its_set(psa_storage_uid_t uid, size_t data_length,
			 const void *p_data, psa_storage_create_flags_t create_flags)
{
	return secure_storage_its_set(ITS_UID, data_length, p_data, create_flags);
}

/**
 * @brief Retrieves data associated with the provided `uid`.
 *
 * @param[in]  uid           The identifier of the data.
 * @param[in]  data_offset   The offset, in bytes, from which to start reading the data.
 * @param[in]  data_size     The number of bytes to read.
 * @param[out] p_data        The buffer where the data will be placed on success.
 *                           Must be at least `data_size` bytes long.
 * @param[out] p_data_length On success, the number of bytes placed in `p_data`.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT One or more of the arguments are invalid. This can also
 *                                    happen if `data_offset` is larger than the size of the data
 *                                    associated with `uid`.
 * @retval PSA_ERROR_DOES_NOT_EXIST   The provided `uid` was not found in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE  The physical storage has failed (fatal error).
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_its_get(psa_storage_uid_t uid, size_t data_offset,
			 size_t data_size, void *p_data, size_t *p_data_length)
{
	return secure_storage_its_get(ITS_UID, data_offset, data_size, p_data, p_data_length);
}

/**
 * @brief Retrieves the metadata of a given entry.
 *
 * @param[in]  uid    The identifier of the entry.
 * @param[out] p_info A pointer to a `psa_storage_info_t` struct that will
 *                    be populated with the metadata on success.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT One or more of the arguments are invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST   The provided `uid` was not found in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE  The physical storage has failed (fatal error).
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_its_get_info(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
	return secure_storage_its_get_info(ITS_UID, p_info);
}

/**
 * @brief Removes the provided `uid` and its associated data.
 *
 * Deletes all the data associated with the entry from internal storage.
 *
 * @param uid The identifier of the entry to remove.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_PERMITTED    The entry was created with `PSA_STORAGE_FLAG_WRITE_ONCE`.
 * @retval PSA_ERROR_INVALID_ARGUMENT `uid` is invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST   The provided `uid` was not found in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE  The physical storage has failed (fatal error).
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_its_remove(psa_storage_uid_t uid)
{
	return secure_storage_its_remove(ITS_UID);
}

#undef ITS_UID
#undef ITS_CALLER_ID

#endif
