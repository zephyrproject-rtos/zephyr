/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef PSA_PROTECTED_STORAGE_H
#define PSA_PROTECTED_STORAGE_H

/** @file psa/protected_storage.h The PSA Protected Storage (PS) API.
 * @ingroup psa_secure_storage
 * For more information on the PS, see [The Protected Storage API](https://arm-software.github.io/psa-api/storage/1.0/overview/architecture.html#the-protected-storage-api).
 */

/** @cond INTERNAL_HIDDEN */
#ifdef CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS
#include "../internal/zephyr/secure_storage/its.h"
#define ITS_UID (secure_storage_its_uid_t){.uid = uid, \
					   .caller_id = SECURE_STORAGE_ITS_CALLER_PSA_PS}
#else
#include "../internal/zephyr/secure_storage/ps.h"
#endif
/** @endcond */

#include <psa/storage_common.h>

#define PSA_PS_API_VERSION_MAJOR 1
#define PSA_PS_API_VERSION_MINOR 0

/**
 * @brief Creates a new or modifies an existing entry.
 *
 * @param uid          The identifier of the data. Must be nonzero.
 * @param data_length  The size in bytes of the data in `p_data` to store.
 * @param p_data       A buffer containing the data to store.
 * @param create_flags Flags indicating the properties of the entry.
 *
 * @retval PSA_SUCCESS                    The operation completed successfully.
 * @retval PSA_ERROR_GENERIC_ERROR        An unspecified internal failure happened.
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
psa_status_t psa_ps_set(psa_storage_uid_t uid, size_t data_length,
			const void *p_data, psa_storage_create_flags_t create_flags)
{
#ifdef CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS
	return secure_storage_its_set(ITS_UID, data_length, p_data, create_flags);
#else
	return secure_storage_ps_set(uid, data_length, p_data, create_flags);
#endif
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
 * @retval PSA_SUCCESS                 The operation completed successfully.
 * @retval PSA_ERROR_GENERIC_ERROR     An unspecified internal failure happened.
 * @retval PSA_ERROR_INVALID_ARGUMENT  One or more of the arguments are invalid. This can also
 *                                     happen if `data_offset` is larger than the size of the data
 *                                     associated with `uid`.
 * @retval PSA_ERROR_DOES_NOT_EXIST    The provided `uid` was not found in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE   The physical storage has failed (fatal error).
 * @retval PSA_ERROR_INVALID_SIGNATURE The data associated with `uid` failed authentication.
 * @retval PSA_ERROR_DATA_CORRUPT      The data associated with `uid` is corrupt.
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_ps_get(psa_storage_uid_t uid, size_t data_offset,
			size_t data_size, void *p_data, size_t *p_data_length)
{
#ifdef CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS
	return secure_storage_its_get(ITS_UID, data_offset, data_size, p_data, p_data_length);
#else
	return secure_storage_ps_get(uid, data_offset, data_size, p_data, p_data_length);
#endif
}

/**
 * @brief Retrieves the metadata of a given entry.
 *
 * @param[in]  uid    The identifier of the entry.
 * @param[out] p_info A pointer to a `psa_storage_info_t` struct that will
 *                    be populated with the metadata on success.
 *
 * @retval PSA_SUCCESS                 The operation completed successfully.
 * @retval PSA_ERROR_GENERIC_ERROR     An unspecified internal failure happened.
 * @retval PSA_ERROR_INVALID_ARGUMENT  One or more of the arguments are invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST    The provided `uid` was not found in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE   The physical storage has failed (fatal error).
 * @retval PSA_ERROR_INVALID_SIGNATURE The data associated with `uid` failed authentication.
 * @retval PSA_ERROR_DATA_CORRUPT      The data associated with `uid` is corrupt.
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_ps_get_info(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
#ifdef CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS
	return secure_storage_its_get_info(ITS_UID, p_info);
#else
	return secure_storage_ps_get_info(uid, p_info);
#endif
}

/**
 * @brief Removes the provided `uid` and its associated data.
 *
 * Deletes previously stored data and any associated metadata, including rollback protection data.
 *
 * @param uid The identifier of the entry to remove.
 *
 * @return A status indicating the success/failure of the operation
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_GENERIC_ERROR    An unspecified internal failure happened.
 * @retval PSA_ERROR_NOT_PERMITTED    The entry was created with `PSA_STORAGE_FLAG_WRITE_ONCE`.
 * @retval PSA_ERROR_INVALID_ARGUMENT `uid` is invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST   The provided `uid` was not found in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE  The physical storage has failed (fatal error).
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_ps_remove(psa_storage_uid_t uid)
{
#ifdef CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS
	return secure_storage_its_remove(ITS_UID);
#else
	return secure_storage_ps_remove(uid);
#endif
}

/**
 * @brief Reserves storage for the provided `uid`.
 *
 * Upon success, the capacity of the storage for `uid` will be `capacity`, and the size will be 0.
 * It is only necessary to call this function for data that will be written with the
 * @ref psa_ps_set_extended() function. If only the @ref psa_ps_set() function is used, calls to
 * this function are redundant. This function cannot be used to replace or resize an existing entry.
 *
 * @param uid          The identifier of the entry to reserve storage for.
 * @param capacity     The capacity, in bytes, to allocate.
 * @param create_flags Flags indicating the properties of the entry.
 *
 * @retval PSA_SUCCESS                    The operation completed successfully.
 * @retval PSA_ERROR_GENERIC_ERROR        An unspecified internal failure happened.
 * @retval PSA_ERROR_NOT_SUPPORTED        The implementation doesn't support this function or one
 *                                        or more of the flags provided in `create_flags` are not
 *                                        supported or invalid.
 * @retval PSA_ERROR_INVALID_ARGUMENT     `uid` is invalid.
 * @retval PSA_ERROR_ALREADY_EXISTS       An entry with the provided `uid` already exists.
 * @retval PSA_ERROR_INSUFFICIENT_STORAGE There is insufficient space on the storage medium.
 * @retval PSA_ERROR_STORAGE_FAILURE      The physical storage has failed (fatal error).
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_ps_create(psa_storage_uid_t uid, size_t capacity,
			   psa_storage_create_flags_t create_flags)
{
#ifdef CONFIG_SECURE_STORAGE_PS_SUPPORTS_SET_EXTENDED
	return secure_storage_ps_create(uid, capacity, create_flags);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Writes part of the data associated with the provided `uid`.
 *
 * Before calling this function, storage must have been reserved with a call to
 * @ref psa_ps_create(). This function can also be used to overwrite data that was
 * written with @ref psa_ps_set(). This function can overwrite existing data and/or extend
 * it up to the capacity of the entry specified in @ref psa_ps_create(), but cannot create gaps.
 *
 * @param uid         The identifier of the entry to write.
 * @param data_offset The offset, in bytes, from which to start writing the data.
 *                    Can be at most the current size of the data.
 * @param data_length The size in bytes of the data in `p_data` to write. `data_offset`
 *                    + `data_length` can be at most the capacity of the entry.
 * @param p_data      A buffer containing the data to write.
 *
 * @retval PSA_SUCCESS                 The operation completed successfully.
 * @retval PSA_ERROR_GENERIC_ERROR     An unspecified internal failure happened.
 * @retval PSA_ERROR_NOT_PERMITTED     The entry was created with `PSA_STORAGE_FLAG_WRITE_ONCE`.
 * @retval PSA_ERROR_NOT_SUPPORTED     The implementation doesn't support this function.
 * @retval PSA_ERROR_INVALID_ARGUMENT  One or more of the arguments are invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST    The provided `uid` was not found in the storage.
 * @retval PSA_ERROR_STORAGE_FAILURE   The physical storage has failed (fatal error).
 * @retval PSA_ERROR_INVALID_SIGNATURE The data associated with `uid` failed authentication.
 * @retval PSA_ERROR_DATA_CORRUPT      The data associated with `uid` is corrupt.
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
psa_status_t psa_ps_set_extended(psa_storage_uid_t uid, size_t data_offset,
				 size_t data_length, const void *p_data)
{
#ifdef CONFIG_SECURE_STORAGE_PS_SUPPORTS_SET_EXTENDED
	return secure_storage_ps_set_extended(uid, data_offset, data_length, p_data);
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Lists optional features.
 *
 * @return A bitmask with flags set for the optional features supported by the implementation.
 *         Currently defined flags are limited to `PSA_STORAGE_SUPPORT_SET_EXTENDED`.
 */
/** @cond INTERNAL_HIDDEN */
static ALWAYS_INLINE
/** @endcond  */
uint32_t psa_ps_get_support(void)
{
	uint32_t flags = 0;

#ifdef CONFIG_SECURE_STORAGE_PS_SUPPORTS_SET_EXTENDED
	flags |= PSA_STORAGE_SUPPORT_SET_EXTENDED;
#endif
	return flags;
}

#undef ITS_UID

#endif
